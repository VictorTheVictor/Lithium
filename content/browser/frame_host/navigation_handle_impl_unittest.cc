// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/test/test_render_frame_host.h"

namespace content {

// Test version of a NavigationThrottle. It will always return the same
// NavigationThrottle::ThrottleCheckResult |result_|, It also monitors the
// number of times WillStartRequest and WillRedirectRequest were called.
class TestNavigationThrottle : public NavigationThrottle {
 public:
  TestNavigationThrottle(NavigationHandle* handle,
                         NavigationThrottle::ThrottleCheckResult result)
      : NavigationThrottle(handle),
        result_(result),
        will_start_calls_(0),
        will_redirect_calls_(0) {}

  ~TestNavigationThrottle() override {}

  NavigationThrottle::ThrottleCheckResult WillStartRequest() override {
    ++will_start_calls_;
    return result_;
  }

  NavigationThrottle::ThrottleCheckResult WillRedirectRequest() override {
    ++will_redirect_calls_;
    return result_;
  }

  int will_start_calls() const { return will_start_calls_; }
  int will_redirect_calls() const { return will_redirect_calls_; }

 private:
  // The result returned by the TestNavigationThrottle.
  NavigationThrottle::ThrottleCheckResult result_;

  // The number of times WillStartRequest and WillRedirectRequest were called.
  int will_start_calls_;
  int will_redirect_calls_;
};

class NavigationHandleImplTest : public RenderViewHostImplTestHarness {
 public:
  NavigationHandleImplTest()
      : was_callback_called_(false),
        callback_result_(NavigationThrottle::DEFER) {}

  void SetUp() override {
    RenderViewHostImplTestHarness::SetUp();
    test_handle_ = NavigationHandleImpl::Create(
        GURL(), main_test_rfh()->frame_tree_node());
  }

  void TearDown() override {
    // Release the |test_handle_| before destroying the WebContents, to match
    // the WebContentsObserverSanityChecker expectations.
    test_handle_.reset();
    RenderViewHostImplTestHarness::TearDown();
  }

  bool IsDeferringStart() {
    return test_handle_->state() == NavigationHandleImpl::DEFERRING_START;
  }

  bool IsDeferringRedirect() {
    return test_handle_->state() == NavigationHandleImpl::DEFERRING_REDIRECT;
  }

  // Helper function to call WillStartRequest on |handle|. If this function
  // returns DEFER, |callback_result_| will be set to the actual result of
  // the throttle checks when they are finished.
  void SimulateWillStartRequest() {
    was_callback_called_ = false;
    callback_result_ = NavigationThrottle::DEFER;

    // It's safe to use base::Unretained since the NavigationHandle is owned by
    // the NavigationHandleImplTest.
    test_handle_->WillStartRequest(
        false, Referrer(), false, ui::PAGE_TRANSITION_LINK, false,
        base::Bind(&NavigationHandleImplTest::UpdateThrottleCheckResult,
                   base::Unretained(this)));
  }

  // Helper function to call WillRedirectRequest on |handle|. If this function
  // returns DEFER, |callback_result_| will be set to the actual result of the
  // throttle checks when they are finished.
  void SimulateWillRedirectRequest() {
    was_callback_called_ = false;
    callback_result_ = NavigationThrottle::DEFER;

    // It's safe to use base::Unretained since the NavigationHandle is owned by
    // the NavigationHandleImplTest.
    test_handle_->WillRedirectRequest(
        GURL(), false, GURL(), false,
        base::Bind(&NavigationHandleImplTest::UpdateThrottleCheckResult,
                   base::Unretained(this)));
  }

  // Returns the handle used in tests.
  NavigationHandleImpl* test_handle() const { return test_handle_.get(); }

  // Whether the callback was called.
  bool was_callback_called() const { return was_callback_called_; }

  // Returns the callback_result.
  NavigationThrottle::ThrottleCheckResult callback_result() const {
    return callback_result_;
  }

  // Creates, register and returns a TestNavigationThrottle that will return
  // |result| on checks.
  TestNavigationThrottle* CreateTestNavigationThrottle(
      NavigationThrottle::ThrottleCheckResult result) {
    TestNavigationThrottle* test_throttle =
        new TestNavigationThrottle(test_handle(), result);
    test_handle()->RegisterThrottleForTesting(
        scoped_ptr<TestNavigationThrottle>(test_throttle));
    return test_throttle;
  }

 private:
  // The callback provided to NavigationHandleImpl::WillStartRequest and
  // NavigationHandleImpl::WillRedirectRequest during the tests.
  void UpdateThrottleCheckResult(
      NavigationThrottle::ThrottleCheckResult result) {
    callback_result_ = result;
    was_callback_called_ = true;
  }

  scoped_ptr<NavigationHandleImpl> test_handle_;
  bool was_callback_called_;
  NavigationThrottle::ThrottleCheckResult callback_result_;
};

// Checks that a deferred navigation can be properly resumed.
TEST_F(NavigationHandleImplTest, ResumeDeferred) {
  TestNavigationThrottle* test_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::DEFER);
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_EQ(0, test_throttle->will_start_calls());
  EXPECT_EQ(0, test_throttle->will_redirect_calls());

  // Simulate WillStartRequest. The request should be deferred. The callback
  // should not have been called.
  SimulateWillStartRequest();
  EXPECT_TRUE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_FALSE(was_callback_called());
  EXPECT_EQ(1, test_throttle->will_start_calls());
  EXPECT_EQ(0, test_throttle->will_redirect_calls());

  // Resume the request. It should no longer be deferred and the callback
  // should have been called.
  test_handle()->Resume();
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_TRUE(was_callback_called());
  EXPECT_EQ(NavigationThrottle::PROCEED, callback_result());
  EXPECT_EQ(1, test_throttle->will_start_calls());
  EXPECT_EQ(0, test_throttle->will_redirect_calls());

  // Simulate WillRedirectRequest. The request should be deferred. The callback
  // should not have been called.
  SimulateWillRedirectRequest();
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_TRUE(IsDeferringRedirect());
  EXPECT_FALSE(was_callback_called());
  EXPECT_EQ(1, test_throttle->will_start_calls());
  EXPECT_EQ(1, test_throttle->will_redirect_calls());

  // Resume the request. It should no longer be deferred and the callback
  // should have been called.
  test_handle()->Resume();
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_TRUE(was_callback_called());
  EXPECT_EQ(NavigationThrottle::PROCEED, callback_result());
  EXPECT_EQ(1, test_throttle->will_start_calls());
  EXPECT_EQ(1, test_throttle->will_redirect_calls());
}

// Checks that a NavigationThrottle asking to defer followed by a
// NavigationThrottle asking to proceed behave correctly.
TEST_F(NavigationHandleImplTest, DeferThenProceed) {
  TestNavigationThrottle* defer_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::DEFER);
  TestNavigationThrottle* proceed_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::PROCEED);
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_EQ(0, defer_throttle->will_start_calls());
  EXPECT_EQ(0, defer_throttle->will_redirect_calls());
  EXPECT_EQ(0, proceed_throttle->will_start_calls());
  EXPECT_EQ(0, proceed_throttle->will_redirect_calls());

  // Simulate WillStartRequest. The request should be deferred. The callback
  // should not have been called. The second throttle should not have been
  // notified.
  SimulateWillStartRequest();
  EXPECT_TRUE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_FALSE(was_callback_called());
  EXPECT_EQ(1, defer_throttle->will_start_calls());
  EXPECT_EQ(0, defer_throttle->will_redirect_calls());
  EXPECT_EQ(0, proceed_throttle->will_start_calls());
  EXPECT_EQ(0, proceed_throttle->will_redirect_calls());

  // Resume the request. It should no longer be deferred and the callback
  // should have been called. The second throttle should have been notified.
  test_handle()->Resume();
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_TRUE(was_callback_called());
  EXPECT_EQ(NavigationThrottle::PROCEED, callback_result());
  EXPECT_EQ(1, defer_throttle->will_start_calls());
  EXPECT_EQ(0, defer_throttle->will_redirect_calls());
  EXPECT_EQ(1, proceed_throttle->will_start_calls());
  EXPECT_EQ(0, proceed_throttle->will_redirect_calls());

  // Simulate WillRedirectRequest. The request should be deferred. The callback
  // should not have been called. The second throttle should not have been
  // notified.
  SimulateWillRedirectRequest();
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_TRUE(IsDeferringRedirect());
  EXPECT_FALSE(was_callback_called());
  EXPECT_EQ(1, defer_throttle->will_start_calls());
  EXPECT_EQ(1, defer_throttle->will_redirect_calls());
  EXPECT_EQ(1, proceed_throttle->will_start_calls());
  EXPECT_EQ(0, proceed_throttle->will_redirect_calls());

  // Resume the request. It should no longer be deferred and the callback
  // should have been called. The second throttle should have been notified.
  test_handle()->Resume();
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_TRUE(was_callback_called());
  EXPECT_EQ(NavigationThrottle::PROCEED, callback_result());
  EXPECT_EQ(1, defer_throttle->will_start_calls());
  EXPECT_EQ(1, defer_throttle->will_redirect_calls());
  EXPECT_EQ(1, proceed_throttle->will_start_calls());
  EXPECT_EQ(1, proceed_throttle->will_redirect_calls());
}

// Checks that a NavigationThrottle asking to defer followed by a
// NavigationThrottle asking to cancel behave correctly in WillStartRequest.
TEST_F(NavigationHandleImplTest, DeferThenCancelWillStartRequest) {
  TestNavigationThrottle* defer_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::DEFER);
  TestNavigationThrottle* cancel_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::CANCEL_AND_IGNORE);
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_EQ(0, defer_throttle->will_start_calls());
  EXPECT_EQ(0, defer_throttle->will_redirect_calls());
  EXPECT_EQ(0, cancel_throttle->will_start_calls());
  EXPECT_EQ(0, cancel_throttle->will_redirect_calls());

  // Simulate WillStartRequest. The request should be deferred. The callback
  // should not have been called. The second throttle should not have been
  // notified.
  SimulateWillStartRequest();
  EXPECT_TRUE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_FALSE(was_callback_called());
  EXPECT_EQ(1, defer_throttle->will_start_calls());
  EXPECT_EQ(0, defer_throttle->will_redirect_calls());
  EXPECT_EQ(0, cancel_throttle->will_start_calls());
  EXPECT_EQ(0, cancel_throttle->will_redirect_calls());

  // Resume the request. The callback should have been called. The second
  // throttle should have been notified.
  test_handle()->Resume();
  EXPECT_TRUE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_TRUE(was_callback_called());
  EXPECT_EQ(NavigationThrottle::CANCEL_AND_IGNORE, callback_result());
  EXPECT_EQ(1, defer_throttle->will_start_calls());
  EXPECT_EQ(0, defer_throttle->will_redirect_calls());
  EXPECT_EQ(1, cancel_throttle->will_start_calls());
  EXPECT_EQ(0, cancel_throttle->will_redirect_calls());
}

// Checks that a NavigationThrottle asking to defer followed by a
// NavigationThrottle asking to cancel behave correctly in WillRedirectRequest.
TEST_F(NavigationHandleImplTest, DeferThenCancelWillRedirectRequest) {
  TestNavigationThrottle* defer_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::DEFER);
  TestNavigationThrottle* cancel_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::CANCEL_AND_IGNORE);
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_EQ(0, defer_throttle->will_start_calls());
  EXPECT_EQ(0, defer_throttle->will_redirect_calls());
  EXPECT_EQ(0, cancel_throttle->will_start_calls());
  EXPECT_EQ(0, cancel_throttle->will_redirect_calls());

  // Simulate WillRedirectRequest. The request should be deferred. The callback
  // should not have been called. The second throttle should not have been
  // notified.
  SimulateWillRedirectRequest();
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_TRUE(IsDeferringRedirect());
  EXPECT_FALSE(was_callback_called());
  EXPECT_EQ(0, defer_throttle->will_start_calls());
  EXPECT_EQ(1, defer_throttle->will_redirect_calls());
  EXPECT_EQ(0, cancel_throttle->will_start_calls());
  EXPECT_EQ(0, cancel_throttle->will_redirect_calls());

  // Resume the request. The callback should have been called. The second
  // throttle should have been notified.
  test_handle()->Resume();
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_TRUE(IsDeferringRedirect());
  EXPECT_TRUE(was_callback_called());
  EXPECT_EQ(NavigationThrottle::CANCEL_AND_IGNORE, callback_result());
  EXPECT_EQ(0, defer_throttle->will_start_calls());
  EXPECT_EQ(1, defer_throttle->will_redirect_calls());
  EXPECT_EQ(0, cancel_throttle->will_start_calls());
  EXPECT_EQ(1, cancel_throttle->will_redirect_calls());
}

// Checks that a NavigationThrottle asking to cancel followed by a
// NavigationThrottle asking to proceed behave correctly in WillStartRequest.
// The navigation will be canceled directly, and the second throttle will not
// be called.
TEST_F(NavigationHandleImplTest, CancelThenProceedWillStartRequest) {
  TestNavigationThrottle* cancel_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::CANCEL_AND_IGNORE);
  TestNavigationThrottle* proceed_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::PROCEED);
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_EQ(0, cancel_throttle->will_start_calls());
  EXPECT_EQ(0, cancel_throttle->will_redirect_calls());
  EXPECT_EQ(0, proceed_throttle->will_start_calls());
  EXPECT_EQ(0, proceed_throttle->will_redirect_calls());

  // Simulate WillStartRequest. The request should not be deferred. The
  // callback should not have been called. The second throttle should not have
  // been notified.
  SimulateWillStartRequest();
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_TRUE(was_callback_called());
  EXPECT_EQ(NavigationThrottle::CANCEL_AND_IGNORE, callback_result());
  EXPECT_EQ(1, cancel_throttle->will_start_calls());
  EXPECT_EQ(0, cancel_throttle->will_redirect_calls());
  EXPECT_EQ(0, proceed_throttle->will_start_calls());
  EXPECT_EQ(0, proceed_throttle->will_redirect_calls());
}

// Checks that a NavigationThrottle asking to cancel followed by a
// NavigationThrottle asking to proceed behave correctly in WillRedirectRequest.
// The navigation will be canceled directly, and the second throttle will not
// be called.
TEST_F(NavigationHandleImplTest, CancelThenProceedWillRedirectRequest) {
  TestNavigationThrottle* cancel_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::CANCEL_AND_IGNORE);
  TestNavigationThrottle* proceed_throttle =
      CreateTestNavigationThrottle(NavigationThrottle::PROCEED);
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_EQ(0, cancel_throttle->will_start_calls());
  EXPECT_EQ(0, cancel_throttle->will_redirect_calls());
  EXPECT_EQ(0, proceed_throttle->will_start_calls());
  EXPECT_EQ(0, proceed_throttle->will_redirect_calls());

  // Simulate WillRedirectRequest. The request should not be deferred. The
  // callback should not have been called. The second throttle should not have
  // been notified.
  SimulateWillRedirectRequest();
  EXPECT_FALSE(IsDeferringStart());
  EXPECT_FALSE(IsDeferringRedirect());
  EXPECT_TRUE(was_callback_called());
  EXPECT_EQ(NavigationThrottle::CANCEL_AND_IGNORE, callback_result());
  EXPECT_EQ(0, cancel_throttle->will_start_calls());
  EXPECT_EQ(1, cancel_throttle->will_redirect_calls());
  EXPECT_EQ(0, proceed_throttle->will_start_calls());
  EXPECT_EQ(0, proceed_throttle->will_redirect_calls());
}

}  // namespace content
