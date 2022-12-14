// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/mojo/src/mojo/edk/system/ipc_support.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/test_io_thread.h"
#include "base/test/test_timeouts.h"
#include "mojo/public/cpp/system/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/mojo/src/mojo/edk/embedder/master_process_delegate.h"
#include "third_party/mojo/src/mojo/edk/embedder/platform_channel_pair.h"
#include "third_party/mojo/src/mojo/edk/embedder/simple_platform_support.h"
#include "third_party/mojo/src/mojo/edk/embedder/slave_process_delegate.h"
#include "third_party/mojo/src/mojo/edk/system/channel_manager.h"
#include "third_party/mojo/src/mojo/edk/system/connection_identifier.h"
#include "third_party/mojo/src/mojo/edk/system/dispatcher.h"
#include "third_party/mojo/src/mojo/edk/system/message_pipe.h"
#include "third_party/mojo/src/mojo/edk/system/message_pipe_dispatcher.h"
#include "third_party/mojo/src/mojo/edk/system/process_identifier.h"
#include "third_party/mojo/src/mojo/edk/system/test_utils.h"
#include "third_party/mojo/src/mojo/edk/system/waiter.h"
#include "third_party/mojo/src/mojo/edk/test/multiprocess_test_helper.h"
#include "third_party/mojo/src/mojo/edk/test/test_utils.h"

namespace mojo {
namespace system {
namespace {

const char kConnectionIdFlag[] = "test-connection-id";

// Tests writing a message (containing just data) to |write_mp| and then reading
// it from |read_mp| (it should be the next message, i.e., there should be no
// other messages already enqueued in that direction).
void TestWriteReadMessage(scoped_refptr<MessagePipeDispatcher> write_mp,
                          scoped_refptr<MessagePipeDispatcher> read_mp) {
  // Set up waiting on the read end first (to avoid racing).
  Waiter waiter;
  waiter.Init();
  ASSERT_EQ(
      MOJO_RESULT_OK,
      read_mp->AddAwakable(&waiter, MOJO_HANDLE_SIGNAL_READABLE, 0, nullptr));

  // Write a message with just 'x' through the write end.
  EXPECT_EQ(MOJO_RESULT_OK,
            write_mp->WriteMessage(UserPointer<const void>("x"), 1, nullptr,
                                   MOJO_WRITE_MESSAGE_FLAG_NONE));

  // Wait for it to arrive.
  EXPECT_EQ(MOJO_RESULT_OK, waiter.Wait(test::ActionDeadline(), nullptr));
  read_mp->RemoveAwakable(&waiter, nullptr);

  // Read the message from the read end.
  char buffer[10] = {};
  uint32_t buffer_size = static_cast<uint32_t>(sizeof(buffer));
  EXPECT_EQ(MOJO_RESULT_OK,
            read_mp->ReadMessage(UserPointer<void>(buffer),
                                 MakeUserPointer(&buffer_size), 0, nullptr,
                                 MOJO_READ_MESSAGE_FLAG_NONE));
  EXPECT_EQ(1u, buffer_size);
  EXPECT_EQ('x', buffer[0]);
}

// Writes a message pipe dispatcher (in a message) to |write_mp| and reads it
// from |read_mp| (it should be the next message, i.e., there should be no other
// other messages already enqueued in that direction).
scoped_refptr<MessagePipeDispatcher> SendMessagePipeDispatcher(
    scoped_refptr<MessagePipeDispatcher> write_mp,
    scoped_refptr<MessagePipeDispatcher> read_mp,
    scoped_refptr<MessagePipeDispatcher> mp_to_send) {
  CHECK_NE(mp_to_send, write_mp);
  CHECK_NE(mp_to_send, read_mp);

  // Set up waiting on the read end first (to avoid racing).
  Waiter waiter;
  waiter.Init();
  CHECK_EQ(
      read_mp->AddAwakable(&waiter, MOJO_HANDLE_SIGNAL_READABLE, 0, nullptr),
      MOJO_RESULT_OK);

  // Write a message with just |mp_to_send| through the write end.
  DispatcherTransport transport(
      test::DispatcherTryStartTransport(mp_to_send.get()));
  CHECK(transport.is_valid());
  std::vector<DispatcherTransport> transports;
  transports.push_back(transport);
  CHECK_EQ(write_mp->WriteMessage(NullUserPointer(), 0, &transports,
                                  MOJO_WRITE_MESSAGE_FLAG_NONE),
           MOJO_RESULT_OK);
  transport.End();

  // Wait for it to arrive.
  CHECK_EQ(waiter.Wait(test::ActionDeadline(), nullptr), MOJO_RESULT_OK);
  read_mp->RemoveAwakable(&waiter, nullptr);

  // Read the message from the read end.
  DispatcherVector dispatchers;
  uint32_t num_dispatchers = 10;
  CHECK_EQ(
      read_mp->ReadMessage(NullUserPointer(), NullUserPointer(), &dispatchers,
                           &num_dispatchers, MOJO_READ_MESSAGE_FLAG_NONE),
      MOJO_RESULT_OK);
  CHECK_EQ(dispatchers.size(), 1u);
  CHECK_EQ(num_dispatchers, 1u);
  CHECK_EQ(dispatchers[0]->GetType(), Dispatcher::Type::MESSAGE_PIPE);
  return scoped_refptr<MessagePipeDispatcher>(
      static_cast<MessagePipeDispatcher*>(dispatchers[0].get()));
}

class TestMasterProcessDelegate : public embedder::MasterProcessDelegate {
 public:
  TestMasterProcessDelegate() : slave_disconnected_(false) {}
  ~TestMasterProcessDelegate() override {}

  // Warning: There's only one slave disconnect event (which resets
  // automatically).
  void TryWaitForOnSlaveDisconnect() {
    if (!slave_disconnected_)
      run_loop_.Run();
  }

 private:
  // |embedder::MasterProcessDelegate| methods:
  void OnShutdownComplete() override { NOTREACHED(); }

  void OnSlaveDisconnect(embedder::SlaveInfo /*slave_info*/) override {
    slave_disconnected_ = true;
    if (run_loop_.running())
      run_loop_.Quit();
  }

  bool slave_disconnected_;
  base::RunLoop run_loop_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(TestMasterProcessDelegate);
};

class TestSlaveProcessDelegate : public embedder::SlaveProcessDelegate {
 public:
  TestSlaveProcessDelegate() {}
  ~TestSlaveProcessDelegate() override {}

 private:
  // |embedder::SlaveProcessDelegate| methods:
  void OnShutdownComplete() override { NOTREACHED(); }

  void OnMasterDisconnect() override { NOTREACHED(); }

  MOJO_DISALLOW_COPY_AND_ASSIGN(TestSlaveProcessDelegate);
};

// Represents the master's side of its connection to a slave.
class TestSlaveConnection {
 public:
  TestSlaveConnection(base::TestIOThread* test_io_thread,
                      IPCSupport* master_ipc_support)
      : test_io_thread_(test_io_thread),
        master_ipc_support_(master_ipc_support),
        connection_id_(master_ipc_support_->GenerateConnectionIdentifier()),
        slave_id_(kInvalidProcessIdentifier),
        event_(true, false) {}
  ~TestSlaveConnection() {}

  // After this is called, |ShutdownChannelToSlave()| must be called (possibly
  // after |WaitForChannelToSlave()|) before destruction.
  scoped_refptr<MessagePipeDispatcher> ConnectToSlave() {
    embedder::PlatformChannelPair channel_pair;
    // Note: |ChannelId|s and |ProcessIdentifier|s are interchangeable.
    scoped_refptr<MessagePipeDispatcher> mp =
        master_ipc_support_->ConnectToSlave(
            connection_id_, nullptr, channel_pair.PassServerHandle(),
            base::Bind(&base::WaitableEvent::Signal, base::Unretained(&event_)),
            nullptr, &slave_id_);
    EXPECT_TRUE(mp);
    EXPECT_NE(slave_id_, kInvalidProcessIdentifier);
    EXPECT_NE(slave_id_, kMasterProcessIdentifier);
    slave_platform_handle_ = channel_pair.PassClientHandle();
    return mp;
  }

  void WaitForChannelToSlave() {
    EXPECT_TRUE(event_.TimedWait(TestTimeouts::action_timeout()));
  }

  void ShutdownChannelToSlave() {
    // Since |event_| is manual-reset, calling this multiple times is OK.
    WaitForChannelToSlave();

    test_io_thread_->PostTaskAndWait(
        FROM_HERE,
        base::Bind(&ChannelManager::ShutdownChannelOnIOThread,
                   base::Unretained(master_ipc_support_->channel_manager()),
                   slave_id_));
  }

  embedder::ScopedPlatformHandle PassSlavePlatformHandle() {
    return slave_platform_handle_.Pass();
  }

  const ConnectionIdentifier& connection_id() const { return connection_id_; }

 private:
  base::TestIOThread* const test_io_thread_;
  IPCSupport* const master_ipc_support_;
  const ConnectionIdentifier connection_id_;
  // The master's message pipe dispatcher.
  scoped_refptr<MessagePipeDispatcher> message_pipe_;
  ProcessIdentifier slave_id_;
  base::WaitableEvent event_;
  embedder::ScopedPlatformHandle slave_platform_handle_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(TestSlaveConnection);
};

// Encapsulates the state of a slave. (Note, however, that we share a
// |PlatformSupport| and an I/O thread.)
class TestSlave {
 public:
  // Note: Before destruction, |ShutdownIPCSupport()| must be called.
  TestSlave(embedder::PlatformSupport* platform_support,
            base::TestIOThread* test_io_thread,
            embedder::ScopedPlatformHandle platform_handle)
      : test_io_thread_(test_io_thread),
        slave_ipc_support_(platform_support,
                           embedder::ProcessType::SLAVE,
                           &slave_process_delegate_,
                           test_io_thread->task_runner(),
                           platform_handle.Pass()),
        event_(true, false) {}
  ~TestSlave() {}

  // After this is called, |ShutdownChannelToMaster()| must be called (possibly
  // after |WaitForChannelToMaster()|) before destruction.
  scoped_refptr<MessagePipeDispatcher> ConnectToMaster(
      const ConnectionIdentifier& connection_id) {
    ProcessIdentifier master_id = kInvalidProcessIdentifier;
    scoped_refptr<MessagePipeDispatcher> mp =
        slave_ipc_support_.ConnectToMaster(
            connection_id,
            base::Bind(&base::WaitableEvent::Signal, base::Unretained(&event_)),
            nullptr, &master_id);
    EXPECT_TRUE(mp);
    EXPECT_EQ(kMasterProcessIdentifier, master_id);
    return mp;
  }

  void WaitForChannelToMaster() {
    EXPECT_TRUE(event_.TimedWait(TestTimeouts::action_timeout()));
  }

  void ShutdownChannelToMaster() {
    // Since |event_| is manual-reset, calling this multiple times is OK.
    WaitForChannelToMaster();

    test_io_thread_->PostTaskAndWait(
        FROM_HERE,
        base::Bind(&ChannelManager::ShutdownChannelOnIOThread,
                   base::Unretained(slave_ipc_support_.channel_manager()),
                   kMasterProcessIdentifier));
  }

  // No other methods may be called after this.
  void ShutdownIPCSupport() {
    test_io_thread_->PostTaskAndWait(
        FROM_HERE, base::Bind(&IPCSupport::ShutdownOnIOThread,
                              base::Unretained(&slave_ipc_support_)));
  }

 private:
  base::TestIOThread* const test_io_thread_;
  TestSlaveProcessDelegate slave_process_delegate_;
  IPCSupport slave_ipc_support_;
  base::WaitableEvent event_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(TestSlave);
};

// Encapsulates both the master and slave sides for each slave.
class TestSlaveSetup {
 public:
  TestSlaveSetup(embedder::SimplePlatformSupport* platform_support,
                 base::TestIOThread* test_io_thread,
                 TestMasterProcessDelegate* master_process_delegate,
                 IPCSupport* master_ipc_support)
      : platform_support_(platform_support),
        test_io_thread_(test_io_thread),
        master_process_delegate_(master_process_delegate),
        master_ipc_support_(master_ipc_support) {}
  ~TestSlaveSetup() {
    CHECK(!slave_connection_);
    CHECK(!slave_);
  }

  void Init() {
    // Set up the master side entirely before the slave side, since this
    // simulates what's likely to happen "in reality" more closely.
    slave_connection_.reset(
        new TestSlaveConnection(test_io_thread_, master_ipc_support_));
    master_mp_ = slave_connection_->ConnectToSlave();

    slave_.reset(new TestSlave(platform_support_, test_io_thread_,
                               slave_connection_->PassSlavePlatformHandle()));
    slave_mp_ = slave_->ConnectToMaster(slave_connection_->connection_id());
  }

  void TestConnection() {
    TestWriteReadMessage(master_mp_, slave_mp_);
    TestWriteReadMessage(slave_mp_, master_mp_);
  }

  scoped_refptr<MessagePipeDispatcher> PassMasterMessagePipe() {
    return master_mp_.Pass();
  }

  scoped_refptr<MessagePipeDispatcher> PassSlaveMessagePipe() {
    return slave_mp_.Pass();
  }

  void Shutdown() {
    if (master_mp_) {
      master_mp_->Close();
      master_mp_ = nullptr;
    }
    if (slave_mp_) {
      slave_mp_->Close();
      slave_mp_ = nullptr;
    }

    slave_->ShutdownChannelToMaster();
    slave_->ShutdownIPCSupport();
    master_process_delegate_->TryWaitForOnSlaveDisconnect();
    slave_connection_->ShutdownChannelToSlave();

    slave_.reset();
    slave_connection_.reset();
  }

  TestSlaveConnection* slave_connection() { return slave_connection_.get(); }
  // Note: To close the master message pipe, use |PassMasterMessagePipe()|.
  MessagePipeDispatcher* master_mp() { return master_mp_.get(); }

  TestSlave* slave() { return slave_.get(); }
  // Note: To close the slave message pipe, use |PassSlaveMessagePipe()|.
  MessagePipeDispatcher* slave_mp() { return slave_mp_.get(); }

 private:
  embedder::SimplePlatformSupport* const platform_support_;
  base::TestIOThread* const test_io_thread_;
  TestMasterProcessDelegate* const master_process_delegate_;
  IPCSupport* const master_ipc_support_;

  scoped_ptr<TestSlaveConnection> slave_connection_;
  scoped_refptr<MessagePipeDispatcher> master_mp_;

  scoped_ptr<TestSlave> slave_;
  scoped_refptr<MessagePipeDispatcher> slave_mp_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(TestSlaveSetup);
};

class IPCSupportTest : public testing::Test {
 public:
  // Note: Run master process delegate methods on the I/O thread.
  IPCSupportTest()
      : test_io_thread_(base::TestIOThread::kAutoStart),
        master_ipc_support_(&platform_support_,
                            embedder::ProcessType::MASTER,
                            &master_process_delegate_,
                            test_io_thread_.task_runner(),
                            embedder::ScopedPlatformHandle()) {}
  ~IPCSupportTest() override {}

  scoped_ptr<TestSlaveSetup> SetupSlave() {
    scoped_ptr<TestSlaveSetup> s(
        new TestSlaveSetup(&platform_support_, &test_io_thread_,
                           &master_process_delegate_, &master_ipc_support_));
    s->Init();
    return s;
  }

  void ShutdownMasterIPCSupport() {
    test_io_thread_.PostTaskAndWait(
        FROM_HERE, base::Bind(&IPCSupport::ShutdownOnIOThread,
                              base::Unretained(&master_ipc_support_)));
  }

  embedder::SimplePlatformSupport& platform_support() {
    return platform_support_;
  }
  base::TestIOThread& test_io_thread() { return test_io_thread_; }
  TestMasterProcessDelegate& master_process_delegate() {
    return master_process_delegate_;
  }
  IPCSupport& master_ipc_support() { return master_ipc_support_; }

 private:
  base::MessageLoop message_loop_;
  embedder::SimplePlatformSupport platform_support_;
  base::TestIOThread test_io_thread_;

  // All tests require a master.
  TestMasterProcessDelegate master_process_delegate_;
  IPCSupport master_ipc_support_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(IPCSupportTest);
};

using MessagePipeDispatcherPair =
    std::pair<scoped_refptr<MessagePipeDispatcher>,
              scoped_refptr<MessagePipeDispatcher>>;
MessagePipeDispatcherPair CreateMessagePipe() {
  MessagePipeDispatcherPair rv;
  rv.first = MessagePipeDispatcher::Create(
      MessagePipeDispatcher::kDefaultCreateOptions);
  rv.second = MessagePipeDispatcher::Create(
      MessagePipeDispatcher::kDefaultCreateOptions);
  scoped_refptr<MessagePipe> mp(MessagePipe::CreateLocalLocal());
  rv.first->Init(mp, 0);
  rv.second->Init(mp, 1);
  return rv;
}

TEST_F(IPCSupportTest, MasterSlave) {
  scoped_ptr<TestSlaveSetup> s(SetupSlave());

  s->TestConnection();

  // Don't need the message pipe anymore.
  s->PassMasterMessagePipe()->Close();
  s->PassSlaveMessagePipe()->Close();

  // A message was sent through the message pipe, |Channel|s must have been
  // established on both sides. The events have thus almost certainly been
  // signalled, but we'll wait just to be sure.
  s->slave_connection()->WaitForChannelToSlave();
  s->slave()->WaitForChannelToMaster();

  s->Shutdown();

  ShutdownMasterIPCSupport();
}

// Simulates a master and two slaves. Initially, there are just message pipes
// from the master to the slaves. This tests the master creating a message pipe
// and sending an end to each slave, which should result in a direct connection
// between the two slaves (TODO(vtl): this part doesn't happen yet).
// TODO(vtl): There are various other similar scenarios we'll need to test, so
// we'll need to factor out some of the code.
// TODO(vtl): In this scenario, we can't test the intermediary (the master)
// going away.
TEST_F(IPCSupportTest, ConnectTwoSlaves) {
  scoped_ptr<TestSlaveSetup> s1(SetupSlave());
  scoped_ptr<TestSlaveSetup> s2(SetupSlave());
  s1->TestConnection();
  s2->TestConnection();

  // Make a message pipe (logically "in" the master) and send one end to each
  // slave.
  MessagePipeDispatcherPair send_mp = CreateMessagePipe();
  scoped_refptr<MessagePipeDispatcher> slave1_received_mp =
      SendMessagePipeDispatcher(s1->master_mp(), s1->slave_mp(), send_mp.first);
  scoped_refptr<MessagePipeDispatcher> slave2_received_mp =
      SendMessagePipeDispatcher(s2->master_mp(), s2->slave_mp(),
                                send_mp.second);

  // These should be connected.
  TestWriteReadMessage(slave1_received_mp, slave2_received_mp);
  TestWriteReadMessage(slave2_received_mp, slave1_received_mp);

  s1->PassMasterMessagePipe()->Close();
  s2->PassMasterMessagePipe()->Close();
  s1->PassSlaveMessagePipe()->Close();
  s2->PassSlaveMessagePipe()->Close();

  // They should still be connected.
  TestWriteReadMessage(slave1_received_mp, slave2_received_mp);
  TestWriteReadMessage(slave2_received_mp, slave1_received_mp);

  slave1_received_mp->Close();
  slave2_received_mp->Close();

  s1->Shutdown();
  s2->Shutdown();

  ShutdownMasterIPCSupport();
}

// Like |ConnectTwoSlaves|, but does it twice, to test reusing a connection.
TEST_F(IPCSupportTest, ConnectTwoSlavesTwice) {
  scoped_ptr<TestSlaveSetup> s1(SetupSlave());
  scoped_ptr<TestSlaveSetup> s2(SetupSlave());
  s1->TestConnection();
  s2->TestConnection();

  MessagePipeDispatcherPair send_mp1 = CreateMessagePipe();
  scoped_refptr<MessagePipeDispatcher> slave1_received_mp1 =
      SendMessagePipeDispatcher(s1->master_mp(), s1->slave_mp(),
                                send_mp1.first);
  scoped_refptr<MessagePipeDispatcher> slave2_received_mp1 =
      SendMessagePipeDispatcher(s2->master_mp(), s2->slave_mp(),
                                send_mp1.second);

  MessagePipeDispatcherPair send_mp2 = CreateMessagePipe();
  scoped_refptr<MessagePipeDispatcher> slave1_received_mp2 =
      SendMessagePipeDispatcher(s1->master_mp(), s1->slave_mp(),
                                send_mp2.first);
  scoped_refptr<MessagePipeDispatcher> slave2_received_mp2 =
      SendMessagePipeDispatcher(s2->master_mp(), s2->slave_mp(),
                                send_mp2.second);

  s1->PassMasterMessagePipe()->Close();
  s2->PassMasterMessagePipe()->Close();
  s1->PassSlaveMessagePipe()->Close();
  s2->PassSlaveMessagePipe()->Close();

  TestWriteReadMessage(slave1_received_mp1, slave2_received_mp1);
  TestWriteReadMessage(slave2_received_mp1, slave1_received_mp1);

  TestWriteReadMessage(slave1_received_mp2, slave2_received_mp2);
  TestWriteReadMessage(slave2_received_mp2, slave1_received_mp2);

  slave1_received_mp1->Close();
  slave2_received_mp1->Close();

  TestWriteReadMessage(slave1_received_mp2, slave2_received_mp2);
  TestWriteReadMessage(slave2_received_mp2, slave1_received_mp2);

  slave1_received_mp2->Close();
  slave2_received_mp2->Close();

  s1->Shutdown();
  s2->Shutdown();

  ShutdownMasterIPCSupport();
}

// Creates a message pipe in the slave, which sends both ends (in separate
// messages) to the master.
TEST_F(IPCSupportTest, SlavePassBackToMaster) {
  scoped_ptr<TestSlaveSetup> s(SetupSlave());

  s->TestConnection();

  // Make a message pipe (logically "in" the slave) and send both ends
  // (separately) to the master.
  MessagePipeDispatcherPair send_mp = CreateMessagePipe();
  scoped_refptr<MessagePipeDispatcher> received_mp1 =
      SendMessagePipeDispatcher(s->slave_mp(), s->master_mp(), send_mp.first);

  TestWriteReadMessage(received_mp1, send_mp.second);
  TestWriteReadMessage(send_mp.second, received_mp1);

  scoped_refptr<MessagePipeDispatcher> received_mp2 =
      SendMessagePipeDispatcher(s->slave_mp(), s->master_mp(), send_mp.second);

  s->PassMasterMessagePipe()->Close();
  s->PassSlaveMessagePipe()->Close();

  TestWriteReadMessage(received_mp1, received_mp2);
  TestWriteReadMessage(received_mp2, received_mp1);

  s->Shutdown();

  // These should still be connected.
  // TODO(vtl): This is not yet implemented, thus will fail here!
  // TestWriteReadMessage(received_mp1, received_mp2);
  // TestWriteReadMessage(received_mp2, received_mp1);

  received_mp1->Close();
  received_mp2->Close();

  ShutdownMasterIPCSupport();
}

}  // namespace

// Note: This test isn't in an anonymous namespace, since it needs to be
// friended by |IPCSupport|.
TEST_F(IPCSupportTest, MasterSlaveInternal) {
  ConnectionIdentifier connection_id =
      master_ipc_support().GenerateConnectionIdentifier();

  embedder::PlatformChannelPair channel_pair;
  ProcessIdentifier slave_id = kInvalidProcessIdentifier;
  embedder::ScopedPlatformHandle master_second_platform_handle =
      master_ipc_support().ConnectToSlaveInternal(
          connection_id, nullptr, channel_pair.PassServerHandle(), &slave_id);
  ASSERT_TRUE(master_second_platform_handle.is_valid());
  EXPECT_NE(slave_id, kInvalidProcessIdentifier);
  EXPECT_NE(slave_id, kMasterProcessIdentifier);

  TestSlaveProcessDelegate slave_process_delegate;
  // Note: Run process delegate methods on the I/O thread.
  IPCSupport slave_ipc_support(
      &platform_support(), embedder::ProcessType::SLAVE,
      &slave_process_delegate, test_io_thread().task_runner(),
      channel_pair.PassClientHandle());

  embedder::ScopedPlatformHandle slave_second_platform_handle =
      slave_ipc_support.ConnectToMasterInternal(connection_id);
  ASSERT_TRUE(slave_second_platform_handle.is_valid());

  // Write an 'x' through the master's end.
  size_t n = 0;
  EXPECT_TRUE(mojo::test::BlockingWrite(master_second_platform_handle.get(),
                                        "x", 1, &n));
  EXPECT_EQ(1u, n);

  // Read it from the slave's end.
  char c = '\0';
  n = 0;
  EXPECT_TRUE(
      mojo::test::BlockingRead(slave_second_platform_handle.get(), &c, 1, &n));
  EXPECT_EQ(1u, n);
  EXPECT_EQ('x', c);

  test_io_thread().PostTaskAndWait(
      FROM_HERE, base::Bind(&IPCSupport::ShutdownOnIOThread,
                            base::Unretained(&slave_ipc_support)));

  master_process_delegate().TryWaitForOnSlaveDisconnect();

  ShutdownMasterIPCSupport();
}

// This is a true multiprocess version of IPCSupportTest.MasterSlaveInternal.
// Note: This test isn't in an anonymous namespace, since it needs to be
// friended by |IPCSupport|.
#if defined(OS_ANDROID)
// Android multi-process tests are not executing the new process. This is flaky.
// TODO(vtl): I'm guessing this is true of this test too?
#define MAYBE_MultiprocessMasterSlaveInternal \
  DISABLED_MultiprocessMasterSlaveInternal
#else
#define MAYBE_MultiprocessMasterSlaveInternal MultiprocessMasterSlaveInternal
#endif  // defined(OS_ANDROID)
TEST_F(IPCSupportTest, MAYBE_MultiprocessMasterSlaveInternal) {
  ConnectionIdentifier connection_id =
      master_ipc_support().GenerateConnectionIdentifier();
  mojo::test::MultiprocessTestHelper multiprocess_test_helper;
  ProcessIdentifier slave_id = kInvalidProcessIdentifier;
  embedder::ScopedPlatformHandle second_platform_handle =
      master_ipc_support().ConnectToSlaveInternal(
          connection_id, nullptr,
          multiprocess_test_helper.server_platform_handle.Pass(), &slave_id);
  ASSERT_TRUE(second_platform_handle.is_valid());
  EXPECT_NE(slave_id, kInvalidProcessIdentifier);
  EXPECT_NE(slave_id, kMasterProcessIdentifier);

  multiprocess_test_helper.StartChildWithExtraSwitch(
      "MultiprocessMasterSlaveInternal", kConnectionIdFlag,
      connection_id.ToString());

  // We write a '?'. The slave should write a '!' in response.
  size_t n = 0;
  EXPECT_TRUE(
      mojo::test::BlockingWrite(second_platform_handle.get(), "?", 1, &n));
  EXPECT_EQ(1u, n);

  char c = '\0';
  n = 0;
  EXPECT_TRUE(
      mojo::test::BlockingRead(second_platform_handle.get(), &c, 1, &n));
  EXPECT_EQ(1u, n);
  EXPECT_EQ('!', c);

  master_process_delegate().TryWaitForOnSlaveDisconnect();
  EXPECT_TRUE(multiprocess_test_helper.WaitForChildTestShutdown());

  ShutdownMasterIPCSupport();
}

MOJO_MULTIPROCESS_TEST_CHILD_TEST(MultiprocessMasterSlaveInternal) {
  embedder::ScopedPlatformHandle client_platform_handle =
      mojo::test::MultiprocessTestHelper::client_platform_handle.Pass();
  ASSERT_TRUE(client_platform_handle.is_valid());

  embedder::SimplePlatformSupport platform_support;
  base::MessageLoop message_loop;
  base::TestIOThread test_io_thread(base::TestIOThread::kAutoStart);
  TestSlaveProcessDelegate slave_process_delegate;
  // Note: Run process delegate methods on the I/O thread.
  IPCSupport ipc_support(&platform_support, embedder::ProcessType::SLAVE,
                         &slave_process_delegate, test_io_thread.task_runner(),
                         client_platform_handle.Pass());

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  ASSERT_TRUE(command_line.HasSwitch(kConnectionIdFlag));
  bool ok = false;
  ConnectionIdentifier connection_id = ConnectionIdentifier::FromString(
      command_line.GetSwitchValueASCII(kConnectionIdFlag), &ok);
  ASSERT_TRUE(ok);

  embedder::ScopedPlatformHandle second_platform_handle =
      ipc_support.ConnectToMasterInternal(connection_id);
  ASSERT_TRUE(second_platform_handle.is_valid());

  // The master should write a '?'. We'll write a '!' in response.
  char c = '\0';
  size_t n = 0;
  EXPECT_TRUE(
      mojo::test::BlockingRead(second_platform_handle.get(), &c, 1, &n));
  EXPECT_EQ(1u, n);
  EXPECT_EQ('?', c);

  n = 0;
  EXPECT_TRUE(
      mojo::test::BlockingWrite(second_platform_handle.get(), "!", 1, &n));
  EXPECT_EQ(1u, n);

  test_io_thread.PostTaskAndWait(FROM_HERE,
                                 base::Bind(&IPCSupport::ShutdownOnIOThread,
                                            base::Unretained(&ipc_support)));
}

// TODO(vtl): Also test the case of the master "dying" before the slave. (The
// slave should get OnMasterDisconnect(), which we currently don't test.)

}  // namespace system
}  // namespace mojo
