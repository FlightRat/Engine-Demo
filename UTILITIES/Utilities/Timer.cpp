#include "Timer.h"

namespace ENGINE_UTIL {
    void Timer::Start()
    {
        if (!m_bIsRunning)
        {
            m_bIsRunning = true;
            m_bIsPaused = false;
            m_StartPoint = Now();
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
            m_PausePoint = Now();
        }
    }

    void Timer::Resume()
    {
        if (m_bIsRunning && m_bIsPaused)
        {
            m_bIsPaused = false;
            m_StartPoint += (Now() - m_PausePoint);
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
                return (m_PausePoint - m_StartPoint);
            else
                return (Now() - m_StartPoint);
        }
        return 0;
    }

    const int64_t Timer::ElapsedSec()
    {
        return ElapsedMS() / 1000;
    }

    void Timer::CreateLuaTimerBind(sol::state& lua, std::function<int64_t()> clock)
    {
        lua.new_usertype<Timer>(
            "Timer",
            sol::call_constructor,
            sol::factories([clock]() {return Timer{clock}; }),
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

