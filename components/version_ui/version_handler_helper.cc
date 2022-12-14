// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/version_ui/version_handler_helper.h"

#include <vector>

#include "base/metrics/field_trial.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "components/variations/active_field_trials.h"

namespace version_ui {

scoped_ptr<base::Value> GetVariationsList() {
  std::vector<std::string> variations;
#if !defined(NDEBUG)
  base::FieldTrial::ActiveGroups active_groups;
  base::FieldTrialList::GetActiveFieldTrialGroups(&active_groups);

  const unsigned char kNonBreakingHyphenUTF8[] = {0xE2, 0x80, 0x91, '\0'};
  const std::string kNonBreakingHyphenUTF8String(
      reinterpret_cast<const char*>(kNonBreakingHyphenUTF8));
  for (const auto& group : active_groups) {
    std::string line = group.trial_name + ":" + group.group_name;
    base::ReplaceChars(line, "-", kNonBreakingHyphenUTF8String, &line);
    variations.push_back(line);
  }
#else
  // In release mode, display the hashes only.
  variations::GetFieldTrialActiveGroupIdsAsStrings(&variations);
#endif

  scoped_ptr<base::ListValue> variations_list(new base::ListValue);
  for (std::vector<std::string>::const_iterator it = variations.begin();
       it != variations.end(); ++it) {
    variations_list->Append(new base::StringValue(*it));
  }

  return variations_list.Pass();
}

}  // namespace version_ui
