// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.content.Context;
import android.view.MotionEvent;

import org.chromium.chrome.browser.compositor.bottombar.OverlayPanel;
import org.chromium.chrome.browser.compositor.bottombar.OverlayPanelManager;
import org.chromium.chrome.browser.compositor.layouts.eventfilter.EdgeSwipeHandler;
import org.chromium.chrome.browser.compositor.layouts.eventfilter.EventFilter;
import org.chromium.chrome.browser.compositor.layouts.eventfilter.EventFilterHost;

/**
 * A {@link EventFilter} used to filter events in the Contextual Search Bar, when displayed
 * in the StaticLayout.
 */
public class ContextualSearchStaticEventFilter extends EventFilter {
    /**
     * The @{link OverlayPanelManager} that controls panel's UI.
     */
    private final OverlayPanelManager mPanelManager;

    /**
     * The @{link SwipeRecognizer} that recognizes directional swipe gestures.
     */
    private final SwipeRecognizer mSwipeRecognizer;

    private final ContextualSearchTapHandler mTapHandler;

    /**
     * Interface to handle taps on the contextual search bar..
     */
    public interface ContextualSearchTapHandler {
        /**
         * Handle a tap event on the contextual seach bar.
         * @param time The time of the tap event.
         * @param x The x position of the tap event.
         * @param y The y position of the tap event.
         */
        void handleTapContextualSearchBar(long time, float x, float y);
    }

    /**
     * Constructs a {@link ContextualSearchStaticEventFilter}.
     *
     * @param context The current Android {@link Context}.
     * @param host The @{link EventFilterHost} associated to this filter.
     * @param panelManager The @{link OverlayPanelManager} responsible for showing different panels.
     * @param swipeHandler The @{link EdgeSwipeHandler} for Contextual Search events.
     */
    public ContextualSearchStaticEventFilter(Context context, EventFilterHost host,
            OverlayPanelManager panelManager, EdgeSwipeHandler swipeHandler,
            ContextualSearchTapHandler tapHandler) {
        super(context, host);

        mPanelManager = panelManager;
        mSwipeRecognizer = new SwipeRecognizerImpl(context);
        mSwipeRecognizer.setSwipeHandler(swipeHandler);
        mTapHandler = tapHandler;
    }

    @Override
    protected boolean onInterceptTouchEventInternal(MotionEvent event, boolean isKeyboardShowing) {
        // TODO(pedrosimonetti): isKeyboardShowing has the wrong value after
        // rotating the device. We don't really need to check whether the
        // keyboard is showing here because Contextual Search's Panel will
        // be closed, if opened, when the keyboard shows up. Even so,
        // it would be nice fixing this problem in Chrome-Android.
        OverlayPanel activePanel = mPanelManager.getActivePanel();
        return activePanel != null && activePanel.isPeeking()
                && activePanel.isCoordinateInsideBar(event.getX() * mPxToDp,
                        activePanel.getFullscreenY(event.getY()) * mPxToDp);
    }

    @Override
    protected boolean onTouchEventInternal(MotionEvent event) {
        OverlayPanel activePanel = mPanelManager.getActivePanel();
        if (activePanel == null) return false;

        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            // To avoid a gray flash of empty content, show the search content
            // view immediately on tap rather than waiting for panel expansion.
            // TODO(pedrosimonetti): Once we implement "side-swipe to dismiss"
            // we'll have to revisit this because we don't want to set the
            // Content View visibility to true when the side-swipe is detected.
            activePanel.notifyPanelTouched();
        }

        mSwipeRecognizer.onTouchEvent(event);
        return true;
    }

    private class SwipeRecognizerImpl extends SwipeRecognizer {
        public SwipeRecognizerImpl(Context context) {
            super(context);
        }

        @Override
        public boolean onSingleTapUp(MotionEvent event) {
            if (mTapHandler == null) return true;
            mTapHandler.handleTapContextualSearchBar(event.getEventTime(),
                    event.getX() * mPxToDp,
                    mPanelManager.getActivePanel().getFullscreenY(event.getY()) * mPxToDp);
            return true;
        }
    }
}
