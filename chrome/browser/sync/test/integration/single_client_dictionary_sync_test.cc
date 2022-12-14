// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/test/integration/dictionary_helper.h"
#include "chrome/browser/sync/test/integration/sync_integration_test_util.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/browser_sync/browser/profile_sync_service.h"

using sync_integration_test_util::AwaitCommitActivityCompletion;

class SingleClientDictionarySyncTest : public SyncTest {
 public:
  SingleClientDictionarySyncTest() : SyncTest(SINGLE_CLIENT) {}
  ~SingleClientDictionarySyncTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SingleClientDictionarySyncTest);
};

IN_PROC_BROWSER_TEST_F(SingleClientDictionarySyncTest, Sanity) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  dictionary_helper::LoadDictionaries();
  ASSERT_TRUE(dictionary_helper::DictionariesMatch());

  std::string word = "foo";
  ASSERT_TRUE(dictionary_helper::AddWord(0, word));
  ASSERT_TRUE(AwaitCommitActivityCompletion(GetSyncService((0))));
  ASSERT_TRUE(dictionary_helper::DictionariesMatch());

  ASSERT_TRUE(dictionary_helper::RemoveWord(0, word));
  ASSERT_TRUE(AwaitCommitActivityCompletion(GetSyncService((0))));
  ASSERT_TRUE(dictionary_helper::DictionariesMatch());
}
