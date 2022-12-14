// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MUS_EXAMPLE_WM_PROPERTY_UTIL_H_
#define COMPONENTS_MUS_EXAMPLE_WM_PROPERTY_UTIL_H_

#include "components/mus/example/wm/public/interfaces/container.mojom.h"
#include "components/mus/public/interfaces/window_manager.mojom.h"
#include "components/mus/public/interfaces/window_manager_constants.mojom.h"

namespace gfx {
class Rect;
class Size;
}

namespace mus {
class Window;
}

// Utility functions to read values from properties & convert them to the
// appropriate types.

mus::mojom::ShowState GetWindowShowState(mus::Window* window);

void SetWindowUserSetBounds(mus::Window* window, const gfx::Rect& bounds);
gfx::Rect GetWindowUserSetBounds(mus::Window* window);

void SetWindowPreferredSize(mus::Window* window, const gfx::Size& size);
gfx::Size GetWindowPreferredSize(mus::Window* window);

ash::mojom::Container GetRequestedContainer(mus::Window* window);

mus::mojom::ResizeBehavior GetResizeBehavior(const mus::Window* window);

#endif  // COMPONENTS_MUS_EXAMPLE_WM_PROPERTY_UTIL_H_
