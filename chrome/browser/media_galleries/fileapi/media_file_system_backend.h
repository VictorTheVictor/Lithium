// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_GALLERIES_FILEAPI_MEDIA_FILE_SYSTEM_BACKEND_H_
#define CHROME_BROWSER_MEDIA_GALLERIES_FILEAPI_MEDIA_FILE_SYSTEM_BACKEND_H_

#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/media_galleries/media_galleries_preferences.h"
#include "storage/browser/fileapi/file_system_backend.h"
#include "storage/browser/fileapi/task_runner_bound_observer_list.h"

namespace base {
class SequencedTaskRunner;
}

namespace storage {
class FileSystemURL;
}

namespace net {
class URLRequest;
}

class MediaPathFilter;
class DeviceMediaAsyncFileUtil;

class MediaFileSystemBackend : public storage::FileSystemBackend {
 public:
  static const char kMediaTaskRunnerName[];

  MediaFileSystemBackend(
      const base::FilePath& profile_path,
      base::SequencedTaskRunner* media_task_runner);
  ~MediaFileSystemBackend() override;

  static bool CurrentlyOnMediaTaskRunnerThread();
  static scoped_refptr<base::SequencedTaskRunner> MediaTaskRunner();

  // Construct the mount point for the gallery specified by |pref_id| in
  // the profile located in |profile_path|.
  static std::string ConstructMountName(const base::FilePath& profile_path,
                                        const std::string& extension_id,
                                        MediaGalleryPrefId pref_id);

  static bool AttemptAutoMountForURLRequest(
      const net::URLRequest* url_request,
      const storage::FileSystemURL& filesystem_url,
      const std::string& storage_domain,
      const base::Callback<void(base::File::Error result)>& callback);

  // FileSystemBackend implementation.
  bool CanHandleType(storage::FileSystemType type) const override;
  void Initialize(storage::FileSystemContext* context) override;
  void ResolveURL(const storage::FileSystemURL& url,
                  storage::OpenFileSystemMode mode,
                  const OpenFileSystemCallback& callback) override;
  storage::AsyncFileUtil* GetAsyncFileUtil(
      storage::FileSystemType type) override;
  storage::WatcherManager* GetWatcherManager(
      storage::FileSystemType type) override;
  storage::CopyOrMoveFileValidatorFactory* GetCopyOrMoveFileValidatorFactory(
      storage::FileSystemType type,
      base::File::Error* error_code) override;
  storage::FileSystemOperation* CreateFileSystemOperation(
      const storage::FileSystemURL& url,
      storage::FileSystemContext* context,
      base::File::Error* error_code) const override;
  bool SupportsStreaming(const storage::FileSystemURL& url) const override;
  bool HasInplaceCopyImplementation(
      storage::FileSystemType type) const override;
  scoped_ptr<storage::FileStreamReader> CreateFileStreamReader(
      const storage::FileSystemURL& url,
      int64 offset,
      int64 max_bytes_to_read,
      const base::Time& expected_modification_time,
      storage::FileSystemContext* context) const override;
  scoped_ptr<storage::FileStreamWriter> CreateFileStreamWriter(
      const storage::FileSystemURL& url,
      int64 offset,
      storage::FileSystemContext* context) const override;
  storage::FileSystemQuotaUtil* GetQuotaUtil() override;
  const storage::UpdateObserverList* GetUpdateObservers(
      storage::FileSystemType type) const override;
  const storage::ChangeObserverList* GetChangeObservers(
      storage::FileSystemType type) const override;
  const storage::AccessObserverList* GetAccessObservers(
      storage::FileSystemType type) const override;

 private:
  // Store the profile path. We need this to create temporary snapshot files.
  const base::FilePath profile_path_;

  scoped_refptr<base::SequencedTaskRunner> media_task_runner_;

  scoped_ptr<MediaPathFilter> media_path_filter_;
  scoped_ptr<storage::CopyOrMoveFileValidatorFactory>
      media_copy_or_move_file_validator_factory_;

  scoped_ptr<storage::AsyncFileUtil> native_media_file_util_;
  scoped_ptr<DeviceMediaAsyncFileUtil> device_media_async_file_util_;
#if defined(OS_WIN) || defined(OS_MACOSX)
  scoped_ptr<storage::AsyncFileUtil> picasa_file_util_;
  scoped_ptr<storage::AsyncFileUtil> itunes_file_util_;

  // Used for usage UMA tracking.
  bool picasa_file_util_used_;
  bool itunes_file_util_used_;
#endif  // defined(OS_WIN) || defined(OS_MACOSX)
#if defined(OS_MACOSX)
  scoped_ptr<storage::AsyncFileUtil> iphoto_file_util_;

  // Used for usage UMA tracking.
  bool iphoto_file_util_used_;
#endif  // defined(OS_MACOSX)

  DISALLOW_COPY_AND_ASSIGN(MediaFileSystemBackend);
};

#endif  // CHROME_BROWSER_MEDIA_GALLERIES_FILEAPI_MEDIA_FILE_SYSTEM_BACKEND_H_
