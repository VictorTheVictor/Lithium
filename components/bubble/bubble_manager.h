// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BUBBLE_BUBBLE_MANAGER_H_
#define COMPONENTS_BUBBLE_BUBBLE_MANAGER_H_

#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "components/bubble/bubble_close_reason.h"
#include "components/bubble/bubble_reference.h"

class BubbleDelegate;

// Inherit from BubbleManager to show, update, and close bubbles.
// Any class that inherits from BubbleManager should capture any events that
// should dismiss a bubble or update its anchor point.
// This class assumes that we won't be showing a lot of bubbles simultaneously.
// TODO(hcarmona): Handle simultaneous bubbles. http://crbug.com/366937
class BubbleManager {
 public:
  // This interface should be used to observe the manager. This is useful when
  // collecting metrics.
  class BubbleManagerObserver {
   public:
    BubbleManagerObserver() {}
    virtual ~BubbleManagerObserver() {}

    // Called when a bubble is asked to be displayed but never shown.
    // ex: a bubble chained on destruction will not be shown.
    virtual void OnBubbleNeverShown(BubbleReference bubble) = 0;

    // Called when a bubble is closed. The reason for closing is provided.
    virtual void OnBubbleClosed(BubbleReference bubble,
                                BubbleCloseReason reason) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(BubbleManagerObserver);
  };

  // Should be instantiated on the UI thread.
  BubbleManager();
  virtual ~BubbleManager();

  // Shows a specific bubble and returns a reference to it.
  // This reference should be used through the BubbleManager.
  BubbleReference ShowBubble(scoped_ptr<BubbleDelegate> bubble);

  // Notify a bubble of an event that might trigger close.
  // Returns true if the bubble was actually closed.
  bool CloseBubble(BubbleReference bubble, BubbleCloseReason reason);

  // Notify all bubbles of an event that might trigger close.
  void CloseAllBubbles(BubbleCloseReason reason);

  // Notify all bubbles that their anchor or parent may have changed.
  void UpdateAllBubbleAnchors();

  // Add an observer for this BubbleManager.
  void AddBubbleManagerObserver(BubbleManagerObserver* observer);

  // Remove an observer from this BubbleManager.
  void RemoveBubbleManagerObserver(BubbleManagerObserver* observer);

 protected:
  // Will close any open bubbles and prevent new ones from being shown.
  void FinalizePendingRequests();

 private:
  enum ManagerState {
    SHOW_BUBBLES,
    NO_MORE_BUBBLES,
    ITERATING_BUBBLES,
  };

  // All bubbles will get a close event for the specified |reason| if |match| is
  // nullptr, otherwise only the bubble held by |match| will get a close event.
  // Any bubble that is closed will also be deleted.
  bool CloseAllMatchingBubbles(BubbleController* match,
                               BubbleCloseReason reason);

  base::ObserverList<BubbleManagerObserver> observers_;

  // Verify that functions that affect the UI are done on the same thread.
  base::ThreadChecker thread_checker_;

  // Determines what happens to a bubble when |ShowBubble| is called.
  ManagerState manager_state_;

  // The bubbles that are being managed.
  ScopedVector<BubbleController> controllers_;

  DISALLOW_COPY_AND_ASSIGN(BubbleManager);
};

#endif  // COMPONENTS_BUBBLE_BUBBLE_MANAGER_H_
