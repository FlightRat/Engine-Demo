#pragma once
#include<string>
#include<string_view>
#include<source_location>
#include<string_view>
#include<string>
#include<vector>
#include<cassert>

#define ENGINE_LOG(x, ...) ENGINE_LOGGER::Logger::GetInstance().Log(x, __VA_ARGS__);
#define ENGINE_WARN(x, ...) ENGINE_LOGGER::Logger::GetInstance().Warn(x, __VA_ARGS__);
#define ENGINE_ERROR(x, ...) ENGINE_LOGGER::Logger::GetInstance().Error(std::source_location::current(), x, __VA_ARGS__)
#define ENGINE_INIT_LOGS(console, retain) ENGINE_LOGGER::Logger::GetInstance().Init(console, retain)
#define ENGINE_GET_LOGS() ENGINE_LOGGER::Logger::GetInstance().GetLogs()
#define ENGINE_CLEAR_LOGS() ENGINE_LOGGER::Logger::GetInstance().ClearLogs()
#define ENGINE_LOG_ADDED() ENGINE_LOGGER::Logger::GetInstance().LogAdded()
#define ENGINE_RESET_ADDED() ENGINE_LOGGER::Logger::GetInstance().ResetLogAdded()

namespace ENGINE_LOGGER {
	struct LogEntry
	{
		// Log info struct£ºinclude an enum type and a string log
		enum class LogType { INFO, WARN, ERR, NONE };
		LogType type{ LogType::INFO };
		std::string log{ "" };
	};

	class Logger
	{
	private:
		std::vector<LogEntry> m_LogEntries;
		bool m_bLogAdded{ false }, m_bInitialized{ false }, m_bConsoleLog{ true }, m_bRetainLogs{ true };
		Logger() = default;	// set constructor private
		struct LogTime
		{
			// Log time struct
			std::string day, dayNumber, month, year, time;
			LogTime(const std::string& date);
		};
		std::string CurrentDateTime();
	public:
		// only way to get logger instance, "static" prove there is only one logger during the whole life
		static Logger& GetInstance();				
		~Logger() = default;
		Logger(const Logger&) = delete;					// prevent create another logger with logger
		Logger& operator = (const Logger&) = delete;	// prevent () operator

		void Init(bool consoleLog = true, bool retainLogs = true);

		template <typename... Args>
		void Log(const std::string& message, Args&&... args);

		template <typename... Args>
		void Warn(const std::string& message, Args&&... args);

		template <typename... Args>
		void Error(std::source_location location, const std::string& message, Args&&... args);

		void LuaLog(const std::string_view message);
		void LuaWarn(const std::string_view message);
		void LuaError(const std::string_view message);

		inline const std::vector<LogEntry>& GetLogs() { return m_LogEntries; }
		inline void ClearLogs() { m_LogEntries.clear(); }
		inline const bool LogAdded() { return m_bLogAdded; }
		inline void ResetLogAdded() { m_bLogAdded = false; }
	};
}

#include"Logger.inl"