// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_RENDER_HOST_RENDER_VIEW_HOST_EXT_H_
#define ANDROID_WEBVIEW_BROWSER_RENDER_HOST_RENDER_VIEW_HOST_EXT_H_

#include "content/public/browser/web_contents_observer.h"

#include "android_webview/common/aw_hit_test_data.h"
#include "base/callback_forward.h"
#include "base/threading/non_thread_safe.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/size_f.h"

class GURL;

namespace content {
struct FrameNavigateParams;
struct LoadCommittedDetails;
}  // namespace content

namespace android_webview {

class AwRenderViewHostExtClient {
 public:
  // Called when the RenderView page scale changes.
  virtual void OnWebLayoutPageScaleFactorChanged(float page_scale_factor) = 0;
  virtual void OnWebLayoutContentsSizeChanged(
      const gfx::Size& contents_size) = 0;

 protected:
  virtual ~AwRenderViewHostExtClient() {}
};

// Provides RenderViewHost wrapper functionality for sending WebView-specific
// IPC messages to the renderer and from there to WebKit.
class AwRenderViewHostExt : public content::WebContentsObserver,
                            public base::NonThreadSafe {
 public:

  // To send receive messages to a RenderView we take the WebContents instance,
  // as it internally handles RenderViewHost instances changing underneath us.
  AwRenderViewHostExt(
      AwRenderViewHostExtClient* client, content::WebContents* contents);
  ~AwRenderViewHostExt() override;

  // |result| will be invoked with the outcome of the request.
  typedef base::Callback<void(bool)> DocumentHasImagesResult;
  void DocumentHasImages(DocumentHasImagesResult result);

  // Clear all WebCore memory cache (not only for this view).
  void ClearCache();

  // Do a hit test at the view port coordinates and asynchronously update
  // |last_hit_test_data_|. Width and height in |touch_area| are in density
  // independent pixels used by blink::WebView.
  void RequestNewHitTestDataAt(const gfx::PointF& touch_center,
                               const gfx::SizeF& touch_area);

  // Optimization to avoid unnecessary Java object creation on hit test.
  bool HasNewHitTestData() const;
  void MarkHitTestDataRead();

  // Return |last_hit_test_data_|. Note that this is unavoidably racy;
  // the corresponding public WebView API is as well.
  const AwHitTestData& GetLastHitTestData() const;

  // Sets the zoom factor for text only. Used in layout modes other than
  // Text Autosizing.
  void SetTextZoomFactor(float factor);

  void ResetScrollAndScaleState();

  // Sets the initial page scale. This overrides initial scale set by
  // the meta viewport tag.
  void SetInitialPageScale(double page_scale_factor);
  void SetBackgroundColor(SkColor c);
  void SetJsOnlineProperty(bool network_up);

  void SmoothScroll(int target_x, int target_y, long duration_ms);

 private:
  // content::WebContentsObserver implementation.
  void RenderViewCreated(content::RenderViewHost* view_host) override;
  void RenderProcessGone(base::TerminationStatus status) override;
  void DidNavigateAnyFrame(content::RenderFrameHost* render_frame_host,
                           const content::LoadCommittedDetails& details,
                           const content::FrameNavigateParams& params) override;
  void OnPageScaleFactorChanged(float page_scale_factor) override;
  bool OnMessageReceived(const IPC::Message& message) override;

  void OnDocumentHasImagesResponse(int msg_id, bool has_images);
  void OnUpdateHitTestData(const AwHitTestData& hit_test_data);
  void OnContentsSizeChanged(const gfx::Size& contents_size);

  bool IsRenderViewReady() const;

  AwRenderViewHostExtClient* client_;

  SkColor background_color_;

  std::map<int, DocumentHasImagesResult> pending_document_has_images_requests_;

  // Master copy of hit test data on the browser side. This is updated
  // as a result of DoHitTest called explicitly or when the FocusedNodeChanged
  // is called in AwRenderViewExt.
  AwHitTestData last_hit_test_data_;

  bool has_new_hit_test_data_;

  DISALLOW_COPY_AND_ASSIGN(AwRenderViewHostExt);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_RENDER_HOST_RENDER_VIEW_HOST_EXT_H_
