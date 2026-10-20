#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ENGINE_CORE {
// Game time is advanced explicitly by the runtime. It never samples wall time.
class GameClock {
    double m_Time = 0, m_Delta = 0, m_Accumulator = 0;
public:
    static constexpr double FixedStep = 1.0 / 60.0;
    static constexpr double MaxDelta = 0.25;
    void Reset() { m_Time = m_Delta = m_Accumulator = 0; }
    void Advance(double seconds) {
        m_Delta = std::isfinite(seconds) ? std::clamp(seconds, 0.0, MaxDelta) : 0;
        m_Time += m_Delta; m_Accumulator += m_Delta;
    }
    void Freeze() { m_Delta = 0; }
    bool ConsumeStep() {
        if (m_Accumulator + 1e-12 < FixedStep) return false;
        m_Accumulator = (std::max)(0.0, m_Accumulator - FixedStep); return true;
    }
    double Seconds() const { return m_Time; }
    double Delta() const { return m_Delta; }
    uint64_t Milliseconds() const { return static_cast<uint64_t>(m_Time * 1000.0 + 1e-7); }
};
}
