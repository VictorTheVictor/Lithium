// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/feedback/screenshot_task.h"

#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/thread_task_runner_handle.h"
#include "jni/ScreenshotTask_jni.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/android/window_android.h"
#include "ui/gfx/display.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/screen.h"
#include "ui/snapshot/snapshot.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaGlobalRef;
using ui::WindowAndroid;

namespace chrome {
namespace android {

bool RegisterScreenshotTask(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void SnapshotCallback(JNIEnv* env,
                      base::android::ScopedJavaGlobalRef<jobject>* callback,
                      scoped_refptr<base::RefCountedBytes> png_data) {
  jbyteArray jbytes = nullptr;
  if (png_data.get()) {
    size_t size = png_data->size();
    jbytes = env->NewByteArray(size);
    env->SetByteArrayRegion(jbytes, 0, size, (jbyte*) png_data->front());
  }
  Java_ScreenshotTask_notifySnapshotFinished(env,
                                             callback->obj(),
                                             jbytes);
}

void GrabWindowSnapshotAsync(JNIEnv* env,
                             const JavaParamRef<jclass>& clazz,
                             const JavaParamRef<jobject>& jcallback,
                             jlong native_window_android,
                             jint window_width,
                             jint window_height) {
  WindowAndroid* window_android = reinterpret_cast<WindowAndroid*>(
      native_window_android);
  gfx::Rect window_bounds(window_width, window_height);
  ui::GrabWindowSnapshotAsync(
      window_android,
      window_bounds,
      base::ThreadTaskRunnerHandle::Get(),
      base::Bind(&SnapshotCallback,
                 env,
                 base::Owned(new ScopedJavaGlobalRef<jobject>(env,
                                                              jcallback))));
}

}  // namespace android
}  // namespace chrome
