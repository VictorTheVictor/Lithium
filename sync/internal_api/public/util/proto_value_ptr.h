// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_SYNCABLE_ENTRY_PROTO_FIELD_PTR_H_
#define SYNC_SYNCABLE_ENTRY_PROTO_FIELD_PTR_H_

#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"

namespace syncer_v2 {
FORWARD_DECLARE_TEST(EntityDataTest, Swap);
}  // namespace syncable

namespace syncer {

namespace syncable {
struct EntryKernel;
}  // namespace syncable

// Default traits struct for ProtoValuePtr - adapts a
// ::google::protobuf::MessageLite derived type to be used with ProtoValuePtr.
template <typename T>
struct DefaultProtoValuePtrTraits {
  // Deep copy the value from |src| to |dest|.
  static void CopyValue(T* dest, const T& src) { dest->CopyFrom(src); }
  // Swap the value with the source (to avoid deep copying).
  static void SwapValue(T* dest, T* src) { dest->Swap(src); }
  // Parse the value from BLOB.
  static void ParseFromBlob(T* dest, const void* blob, int length) {
    dest->ParseFromArray(blob, length);
  }
  // True if the |value| is a non-default value.
  static bool HasValue(const T& value) { return value.ByteSize() > 0; }
  // Default value for the type.
  static const T& DefaultValue() { return T::default_instance(); }
};

// This is a smart pointer to a ::google::protobuf::MessageLite derived type
// that implements immutable, shareable, copy-on-write semantics.
//
// Additionally this class helps to avoid storing multiple copies of default
// instances of the wrapped type.
//
// Copying ProtoValuePtr results in ref-counted sharing of the
// underlying wrapper and the value contained in the wrapper.
//
// The public interface includes only immutable access to the wrapped value.
// The only way to assign a value to ProtoValuePtr is through a
// private SetValue function which is called from EntryKernel. That results
// in stopping sharing the previous value and creating a wrapper to the new
// value.
template <typename T, typename Traits = DefaultProtoValuePtrTraits<T>>
class ProtoValuePtr {
 private:
  // Immutable shareable ref-counted wrapper that embeds the value.
  class Wrapper : public base::RefCountedThreadSafe<Wrapper> {
   public:
    Wrapper(const T& value) { Traits::CopyValue(&value_, value); }
    Wrapper(T* value) { Traits::SwapValue(&value_, value); }

    const T& value() const { return value_; }
    // Create wrapper by deserializing a BLOB.
    static Wrapper* ParseFromBlob(const void* blob, int length) {
      Wrapper* wrapper = new Wrapper;
      Traits::ParseFromBlob(&wrapper->value_, blob, length);
      return wrapper;
    }

   private:
    friend class base::RefCountedThreadSafe<Wrapper>;
    Wrapper() {}
    ~Wrapper() {}

    T value_;
  };

  ProtoValuePtr() {}
  ~ProtoValuePtr() {}

 public:
  const T& value() const {
    return wrapper_ ? wrapper_->value() : Traits::DefaultValue();
  }

  const T* operator->() const {
    const T& wrapped_instance = value();
    return &wrapped_instance;
  }

 private:
  friend struct syncable::EntryKernel;
  FRIEND_TEST_ALL_PREFIXES(ProtoValuePtrTest, ValueAssignment);
  FRIEND_TEST_ALL_PREFIXES(ProtoValuePtrTest, ValueSwap);
  FRIEND_TEST_ALL_PREFIXES(ProtoValuePtrTest, SharingTest);
  FRIEND_TEST_ALL_PREFIXES(ProtoValuePtrTest, ParsingTest);
  FRIEND_TEST_ALL_PREFIXES(syncer_v2::EntityDataTest, Swap);

  // set the value to copy of |new_value|.
  void set_value(const T& new_value) {
    if (Traits::HasValue(new_value)) {
      wrapper_ = new Wrapper(new_value);
    } else {
      // Don't store default value.
      wrapper_ = nullptr;
    }
  }

  // Take over |src| value (swap).
  void swap_value(T* src) {
    if (Traits::HasValue(*src)) {
      wrapper_ = new Wrapper(src);
    } else {
      // Don't store default value.
      wrapper_ = nullptr;
    }
  }

  void load(const void* blob, int length) {
    wrapper_ = Wrapper::ParseFromBlob(blob, length);
  }

  scoped_refptr<Wrapper> wrapper_;
};

}  // namespace syncer

#endif  // SYNC_SYNCABLE_ENTRY_PROTO_FIELD_PTR_H_
