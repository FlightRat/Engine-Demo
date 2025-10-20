#include "Timer.h"

namespace ENGINE_UTIL {
    void Timer::Start()
    {
        if (!m_bIsRunning)
        {
            m_bIsRunning = true;
            m_bIsPaused = false;
            m_StartPoint = steady_clock::now();
        }
    }

    void Timer::Stop()
    {
        if (m_bIsRunning)
        {
            m_bIsRunning = false;
        }
    }

    void Timer::Pause()
    {
        if (!m_bIsPaused && m_bIsRunning)
        {
            m_bIsPaused = true;
            m_PausePoint = steady_clock::now();
        }
    }

    void Timer::Resume()
    {
        if (m_bIsRunning && m_bIsPaused)
        {
            m_bIsPaused = false;
            m_StartPoint += duration_cast<milliseconds>(steady_clock::now() - m_PausePoint);
        }
    }

    void Timer::Restart()
    {
        if (IsRunning())
        {
            Stop();
        }
        Start();
    }

    const int64_t Timer::ElapsedMS()
    {
        if (m_bIsRunning)
        {
            if (m_bIsPaused)
                return duration_cast<milliseconds>(m_PausePoint - m_StartPoint).count();
            else
                return duration_cast<milliseconds>(steady_clock::now() - m_StartPoint).count();
        }
        return 0;
    }

    const int64_t Timer::ElapsedSec()
    {
        return ElapsedMS() / 1000;
    }

    void Timer::CreateLuaTimer(sol::state& lua)
    {
        lua.new_usertype<Timer>(
            "Timer",
            sol::call_constructor,
            sol::factories([]() {return Timer{}; }),
            "start",&Timer::Start,
            "stop", &Timer::Stop,
            "pause", &Timer::Pause,
            "resume", &Timer::Resume,
            "restart",&Timer::Restart,
            "is_paused",&Timer::IsPaused,
            "is_running",&Timer::IsRunning,
            "elapsed_ms",&Timer::ElapsedMS,
            "elapsed_sec",&Timer::ElapsedSec
        );
    }

}

