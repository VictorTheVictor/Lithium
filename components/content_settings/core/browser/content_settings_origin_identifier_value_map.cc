// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/content_settings/core/browser/content_settings_origin_identifier_value_map.h"

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "base/values.h"
#include "components/content_settings/core/browser/content_settings_rule.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"

namespace content_settings {

namespace {

// This iterator is used for iterating the rules for |content_type| and
// |resource_identifier| in the precedence order of the rules.
class RuleIteratorImpl : public RuleIterator {
 public:
  // |RuleIteratorImpl| takes the ownership of |auto_lock|.
  RuleIteratorImpl(
      const OriginIdentifierValueMap::Rules::const_iterator& current_rule,
      const OriginIdentifierValueMap::Rules::const_iterator& rule_end,
      base::AutoLock* auto_lock)
      : current_rule_(current_rule),
        rule_end_(rule_end),
        auto_lock_(auto_lock) {
  }
  ~RuleIteratorImpl() override {}

  bool HasNext() const override { return (current_rule_ != rule_end_); }

  Rule Next() override {
    DCHECK(current_rule_ != rule_end_);
    DCHECK(current_rule_->second.get());
    Rule to_return(current_rule_->first.primary_pattern,
                   current_rule_->first.secondary_pattern,
                   current_rule_->second.get()->DeepCopy());
    ++current_rule_;
    return to_return;
  }

 private:
  OriginIdentifierValueMap::Rules::const_iterator current_rule_;
  OriginIdentifierValueMap::Rules::const_iterator rule_end_;
  scoped_ptr<base::AutoLock> auto_lock_;
};

}  // namespace

OriginIdentifierValueMap::EntryMapKey::EntryMapKey(
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier)
    : content_type(content_type),
      resource_identifier(resource_identifier) {
}

bool OriginIdentifierValueMap::EntryMapKey::operator<(
    const OriginIdentifierValueMap::EntryMapKey& other) const {
  if (content_type != other.content_type)
    return content_type < other.content_type;
  return (resource_identifier < other.resource_identifier);
}

OriginIdentifierValueMap::PatternPair::PatternPair(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern)
    : primary_pattern(primary_pattern),
      secondary_pattern(secondary_pattern) {
}

bool OriginIdentifierValueMap::PatternPair::operator<(
    const OriginIdentifierValueMap::PatternPair& other) const {
  // Note that this operator is the other way around than
  // |ContentSettingsPattern::operator<|. It sorts patterns with higher
  // precedence first.
  if (primary_pattern > other.primary_pattern)
    return true;
  else if (other.primary_pattern > primary_pattern)
    return false;
  return (secondary_pattern > other.secondary_pattern);
}

RuleIterator* OriginIdentifierValueMap::GetRuleIterator(
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    base::Lock* lock) const {
  EntryMapKey key(content_type, resource_identifier);
  // We access |entries_| here, so we need to lock |lock_| first. The lock must
  // be passed to the |RuleIteratorImpl| in a locked state, so that nobody can
  // access |entries_| after |find()| but before the |RuleIteratorImpl| is
  // created.
  scoped_ptr<base::AutoLock> auto_lock;
  if (lock)
    auto_lock.reset(new base::AutoLock(*lock));
  EntryMap::const_iterator it = entries_.find(key);
  if (it == entries_.end())
    return new EmptyRuleIterator();
  return new RuleIteratorImpl(it->second.begin(),
                              it->second.end(),
                              auto_lock.release());
}

size_t OriginIdentifierValueMap::size() const {
  size_t size = 0;
  EntryMap::const_iterator it;
  for (it = entries_.begin(); it != entries_.end(); ++it)
    size += it->second.size();
  return size;
}

OriginIdentifierValueMap::OriginIdentifierValueMap() {}

OriginIdentifierValueMap::~OriginIdentifierValueMap() {}

base::Value* OriginIdentifierValueMap::GetValue(
    const GURL& primary_url,
    const GURL& secondary_url,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier) const {
  EntryMapKey key(content_type, resource_identifier);
  EntryMap::const_iterator it = entries_.find(key);
  if (it == entries_.end())
      return NULL;

  // Iterate the entries in until a match is found. Since the rules are stored
  // in the order of decreasing precedence, the most specific match is found
  // first.
  Rules::const_iterator entry;
  for (entry = it->second.begin(); entry != it->second.end(); ++entry) {
    if (entry->first.primary_pattern.Matches(primary_url) &&
        entry->first.secondary_pattern.Matches(secondary_url)) {
      return entry->second.get();
    }
  }
  return NULL;
}

void OriginIdentifierValueMap::SetValue(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const ResourceIdentifier& resource_identifier,
    base::Value* value) {
  DCHECK(primary_pattern.IsValid());
  DCHECK(secondary_pattern.IsValid());
  DCHECK(value);
  // TODO(raymes): Remove this after we track down the cause of
  // crbug.com/531548.
  CHECK_NE(CONTENT_SETTINGS_TYPE_DEFAULT, content_type);
  EntryMapKey key(content_type, resource_identifier);
  PatternPair patterns(primary_pattern, secondary_pattern);
  // This will create the entry and the linked_ptr if needed.
  entries_[key][patterns].reset(value);
}

void OriginIdentifierValueMap::DeleteValue(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier) {
  EntryMapKey key(content_type, resource_identifier);
  PatternPair patterns(primary_pattern, secondary_pattern);
  EntryMap::iterator it = entries_.find(key);
  if (it == entries_.end())
    return;
  it->second.erase(patterns);
  if (it->second.empty())
    entries_.erase(it);
}

void OriginIdentifierValueMap::DeleteValues(
      ContentSettingsType content_type,
      const ResourceIdentifier& resource_identifier) {
  EntryMapKey key(content_type, resource_identifier);
  entries_.erase(key);
}

void OriginIdentifierValueMap::clear() {
  // Delete all owned value objects.
  entries_.clear();
}

}  // namespace content_settings
