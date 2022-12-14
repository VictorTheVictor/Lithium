
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/input_method/mock_input_method_engine.h"

#include <map>

namespace chromeos {

MockInputMethodEngine::MockInputMethodEngine() {}

MockInputMethodEngine::~MockInputMethodEngine() {}

const std::string& MockInputMethodEngine::GetActiveComponentId() const {
  return active_component_id_;
}

bool MockInputMethodEngine::SetComposition(
    int context_id,
    const char* text,
    int selection_start,
    int selection_end,
    int cursor,
    const std::vector<SegmentInfo>& segments,
    std::string* error) {
  return true;
}

bool MockInputMethodEngine::ClearComposition(int context_id,
                                             std::string* error) {
  return true;
}

bool MockInputMethodEngine::CommitText(int context_id,
                                       const char* text,
                                       std::string* error) {
  return true;
}

bool MockInputMethodEngine::SendKeyEvents(
    int context_id,
    const std::vector<KeyboardEvent>& events) {
  return true;
}

const MockInputMethodEngine::CandidateWindowProperty&
MockInputMethodEngine::GetCandidateWindowProperty() const {
  return candidate_window_property_;
}

void MockInputMethodEngine::SetCandidateWindowProperty(
    const CandidateWindowProperty& property) {}

bool MockInputMethodEngine::SetCandidateWindowVisible(bool visible,
                                                      std::string* error) {
  return true;
}

bool MockInputMethodEngine::SetCandidates(
    int context_id,
    const std::vector<Candidate>& candidates,
    std::string* error) {
  return true;
}

bool MockInputMethodEngine::SetCursorPosition(int context_id,
                                              int candidate_id,
                                              std::string* error) {
  return true;
}

bool MockInputMethodEngine::SetMenuItems(const std::vector<MenuItem>& items) {
  return true;
}

bool MockInputMethodEngine::UpdateMenuItems(
    const std::vector<MenuItem>& items) {
  return true;
}

bool MockInputMethodEngine::IsActive() const {
  return true;
}

bool MockInputMethodEngine::DeleteSurroundingText(int context_id,
                                                  int offset,
                                                  size_t number_of_chars,
                                                  std::string* error) {
  return true;
}

void MockInputMethodEngine::HideInputView() {}

void MockInputMethodEngine::FocusIn(
    const IMEEngineHandlerInterface::InputContext& input_context) {}

void MockInputMethodEngine::FocusOut() {}

void MockInputMethodEngine::Enable(const std::string& component_id) {
  active_component_id_ = component_id;
}

void MockInputMethodEngine::Disable() {
  active_component_id_.clear();
}

void MockInputMethodEngine::PropertyActivate(const std::string& property_name) {
  last_activated_property_ = property_name;
}

void MockInputMethodEngine::Reset() {}

bool MockInputMethodEngine::IsInterestedInKeyEvent() const {
  return true;
}

void MockInputMethodEngine::ProcessKeyEvent(const ui::KeyEvent& key_event,
                                            KeyEventDoneCallback& callback) {}

void MockInputMethodEngine::CandidateClicked(uint32 index) {}

void MockInputMethodEngine::SetSurroundingText(const std::string& text,
                                               uint32 cursor_pos,
                                               uint32 anchor_pos,
                                               uint32 offset_pos) {}

void MockInputMethodEngine::SetCompositionBounds(
    const std::vector<gfx::Rect>& bounds) {}

}  // namespace chromeos
