#pragma once

namespace ENGINE_EDITOR {
enum class PlaybackState { Editing, Playing, Paused };
inline const char* PlaybackStateName(PlaybackState state) {
    switch (state) {
    case PlaybackState::Playing: return "playing";
    case PlaybackState::Paused: return "paused";
    default: return "editing";
    }
}
struct PlaybackToolbar {
    bool playEnabled, pauseEnabled, stopEnabled, playActive, pauseActive;
    const char* playTooltip;
};
constexpr PlaybackToolbar PlaybackControls(PlaybackState state) {
    return {state != PlaybackState::Playing, state == PlaybackState::Playing,
        state != PlaybackState::Editing, state == PlaybackState::Playing,
        state == PlaybackState::Paused, state == PlaybackState::Paused ? "Resume Game" : "Play Game"};
}
constexpr bool CanEdit(PlaybackState state) { return state == PlaybackState::Editing; }
}
