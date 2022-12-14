// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/mus/example/wm/shelf_layout.h"

#include "components/mus/example/wm/property_util.h"
#include "components/mus/public/cpp/property_type_converters.h"
#include "components/mus/public/cpp/window.h"
#include "components/mus/public/cpp/window_property.h"
#include "ui/gfx/geometry/rect.h"

ShelfLayout::ShelfLayout(mus::Window* owner) : LayoutManager(owner) {
  AddLayoutProperty(mus::mojom::WindowManager::kPreferredSize_Property);
}
ShelfLayout::~ShelfLayout() {}

void ShelfLayout::WindowAdded(mus::Window* window) {
   DCHECK_EQ(owner()->children().size(), 1U);
}

void ShelfLayout::LayoutWindow(mus::Window* window) {
  gfx::Size preferred_size = GetWindowPreferredSize(window);

  gfx::Rect container_bounds = owner()->bounds();
  container_bounds.set_origin(
      gfx::Point(0, container_bounds.height() - preferred_size.height()));
  window->SetBounds(container_bounds);
}
