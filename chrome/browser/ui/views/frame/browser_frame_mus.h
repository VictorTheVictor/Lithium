// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_MUS_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_MUS_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/frame/native_browser_frame.h"
#include "ui/views/mus/native_widget_mus.h"

class BrowserFrameMus : public NativeBrowserFrame,
                        public views::NativeWidgetMus {
 public:
  BrowserFrameMus(BrowserFrame* browser_frame, BrowserView* browser_view);
  ~BrowserFrameMus() override;

 private:
  // Overridden from NativeBrowserFrame:
  views::Widget::InitParams GetWidgetParams() override;
  bool UseCustomFrame() const override;
  bool UsesNativeSystemMenu() const override;
  bool ShouldSaveWindowPlacement() const override;
  void GetWindowPlacement(gfx::Rect* bounds,
                          ui::WindowShowState* show_state) const override;
  int GetMinimizeButtonOffset() const override;

  BrowserView* browser_view_;

  DISALLOW_COPY_AND_ASSIGN(BrowserFrameMus);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_MUS_H_
