// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MUS_EXAMPLE_WM_NON_CLIENT_FRAME_CONTROLLER_H_
#define COMPONENTS_MUS_EXAMPLE_WM_NON_CLIENT_FRAME_CONTROLLER_H_

#include "base/macros.h"
#include "components/mus/public/cpp/window_observer.h"
#include "ui/views/widget/widget_delegate.h"

namespace mojo {
class Shell;
}

namespace mus {
class Window;
}

// Provides the non-client frame for mus Windows.
class NonClientFrameController : public views::WidgetDelegateView,
                                 public mus::WindowObserver {
 public:
  // NonClientFrameController deletes itself when |window| is destroyed.
  NonClientFrameController(mojo::Shell* shell, mus::Window* window);

 private:
  ~NonClientFrameController() override;

  // views::WidgetDelegateView:
  views::View* GetContentsView() override;
  bool CanResize() const override;
  bool CanMaximize() const override;
  bool CanMinimize() const override;

  // mus::WindowObserver:
  void OnWindowSharedPropertyChanged(
      mus::Window* window,
      const std::string& name,
      const std::vector<uint8_t>* old_data,
      const std::vector<uint8_t>* new_data) override;
  void OnWindowDestroyed(mus::Window* window) override;

  views::Widget* widget_;

  // WARNING: as widget delays destruction there is a portion of time when this
  // is null.
  mus::Window* window_;

  DISALLOW_COPY_AND_ASSIGN(NonClientFrameController);
};

#endif  // COMPONENTS_MUS_EXAMPLE_WM_NON_CLIENT_FRAME_CONTROLLER_H_
