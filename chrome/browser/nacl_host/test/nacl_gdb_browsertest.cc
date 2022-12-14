// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/win/windows_version.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/ppapi/ppapi_test.h"
#include "components/nacl/common/nacl_switches.h"

static const base::FilePath::CharType kMockNaClGdb[] =
#if defined(OS_WIN)
    FILE_PATH_LITERAL("mock_nacl_gdb.exe");
#else
    FILE_PATH_LITERAL("mock_nacl_gdb");
#endif

class NaClGdbTest : public PPAPINaClNewlibTest {
 public:
  NaClGdbTest() {
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PPAPINaClNewlibTest::SetUpCommandLine(command_line);

    base::FilePath mock_nacl_gdb;
    EXPECT_TRUE(PathService::Get(base::DIR_EXE, &mock_nacl_gdb));
    mock_nacl_gdb = mock_nacl_gdb.Append(kMockNaClGdb);
    command_line->AppendSwitchPath(switches::kNaClGdb, mock_nacl_gdb);
    EXPECT_TRUE(base::CreateTemporaryFile(&script_));
    command_line->AppendSwitchPath(switches::kNaClGdbScript, script_);
  }

  void RunWithNaClGdb(const std::string& test_name) {
    base::FilePath mock_nacl_gdb_file;
    scoped_ptr<base::Environment> env(base::Environment::Create());
    std::string content;
    // TODO(halyavin): Make this test work on Windows 32-bit. Currently this
    // is not possible because NaCl doesn't work without sandbox since 1Gb of
    // space is not reserved. We can't reserve 1Gb of space because
    // base::LaunchProcess doesn't support creating suspended processes. We need
    // to either add suspended process support to base::LaunchProcess or use
    // Win API.
#if defined(OS_WIN)
    if (base::win::OSInfo::GetInstance()->wow64_status() ==
      base::win::OSInfo::WOW64_DISABLED) {
        return;
    }
#endif
    EXPECT_TRUE(base::CreateTemporaryFile(&mock_nacl_gdb_file));
    env->SetVar("MOCK_NACL_GDB", mock_nacl_gdb_file.AsUTF8Unsafe());
    RunTestViaHTTP(test_name);
    env->UnSetVar("MOCK_NACL_GDB");

    EXPECT_TRUE(base::ReadFileToString(mock_nacl_gdb_file, &content));
    EXPECT_STREQ("PASS", content.c_str());
    EXPECT_TRUE(base::DeleteFile(mock_nacl_gdb_file, false));

    content.clear();
    EXPECT_TRUE(base::ReadFileToString(script_, &content));
    EXPECT_STREQ("PASS", content.c_str());
    EXPECT_TRUE(base::DeleteFile(script_, false));
  }

 private:
  base::FilePath script_;
};

// Fails on the ASAN test bot. See http://crbug.com/122219
#if defined(ADDRESS_SANITIZER)
#define MAYBE_Empty DISABLED_Empty
#else
#define MAYBE_Empty Empty
#endif
IN_PROC_BROWSER_TEST_F(NaClGdbTest, MAYBE_Empty) {
  RunWithNaClGdb("Empty");
}
