#pragma once
#include<chrono>
#include<sol/sol.hpp>
#include <functional>

using namespace std::chrono;

namespace ENGINE_UTIL {
	class Timer
	{
	private:
        int64_t m_StartPoint = 0, m_PausePoint = 0;
        std::function<int64_t()> m_Clock;
        int64_t Now() const {
            return m_Clock ? m_Clock() : duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
        }
		bool m_bIsRunning{ false }, m_bIsPaused{ false };
	public:
		Timer() = default;
        explicit Timer(std::function<int64_t()> clock) : m_Clock(std::move(clock)) {}
		~Timer() = default;

		void Start();
		void Stop();
		void Pause();
		void Resume();
		void Restart();

		const int64_t ElapsedMS();
		const int64_t ElapsedSec();

		inline const bool IsRunning() const { return m_bIsRunning; }
		inline const bool IsPaused() const { return m_bIsPaused; }

		static void CreateLuaTimerBind(sol::state& lua, std::function<int64_t()> clock = {});
	};
}
