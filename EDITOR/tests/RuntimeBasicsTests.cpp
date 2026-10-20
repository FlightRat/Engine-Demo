#include "../src/editor/scene/PlaybackState.h"
#include <Core/CoreUtilities/GameClock.h>
#include <cmath>
#include <stdexcept>
using namespace ENGINE_EDITOR;
static void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void RuntimeBasicsTests() {
    auto edit = PlaybackControls(PlaybackState::Editing);
    auto play = PlaybackControls(PlaybackState::Playing);
    auto pause = PlaybackControls(PlaybackState::Paused);
    Check(edit.playEnabled && !edit.pauseEnabled && !edit.stopEnabled, "Editing controls");
    Check(!play.playEnabled && play.pauseEnabled && play.stopEnabled && play.playActive, "Playing controls");
    Check(pause.playEnabled && !pause.pauseEnabled && pause.stopEnabled && pause.pauseActive, "Paused controls");
    Check(CanEdit(PlaybackState::Editing) && !CanEdit(PlaybackState::Playing) && !CanEdit(PlaybackState::Paused), "Playback permissions");
    for (int fps : {30, 60, 144}) {
        ENGINE_CORE::GameClock clock;
        int steps = 0;
        for (int i = 0; i < fps * 4; ++i) { clock.Advance(1.0 / fps); while (clock.ConsumeStep()) ++steps; }
        Check(steps == 240 && std::abs(clock.Seconds() - 4.0) < 1e-9, "Frame independent game clock");
        const auto ticks = clock.Milliseconds(); clock.Freeze();
        Check(clock.Milliseconds() == ticks && clock.Delta() == 0, "Frozen time advanced");
    }
    ENGINE_CORE::GameClock clock;
    clock.Advance(ENGINE_CORE::GameClock::FixedStep / 2); clock.Freeze();
    Check(!clock.ConsumeStep(), "Fractional step advanced");
    clock.Advance(ENGINE_CORE::GameClock::FixedStep / 2);
    Check(clock.ConsumeStep() && !clock.ConsumeStep(), "Pause lost fractional time");
    clock.Advance(100); Check(clock.Delta() == .25, "Unbounded frame delta");
    clock.Reset(); Check(clock.Seconds() == 0 && !clock.ConsumeStep(), "New session inherited time");
}
