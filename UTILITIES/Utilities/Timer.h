#pragma once
#include<chrono>
#include<sol/sol.hpp>

using namespace std::chrono;

namespace ENGINE_UTIL {
	class Timer
	{
	private:
		time_point<steady_clock> m_StartPoint, m_PausePoint;
		bool m_bIsRunning{ false }, m_bIsPaused{ false };
	public:
		Timer() = default;
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

		static void CreateLuaTimerBind(sol::state& lua);
	};
}