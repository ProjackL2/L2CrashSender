
#pragma once

#include <fstream>
#include <string_view>

namespace CrashSender {

/**
 * @brief Logger with RAII lifecycle management
 */
class Logger {
public:
    enum class LogLevel : int {
        Debug = 0,
        Info = 1, 
        Error = 2
    };

    /**
     * @brief Get the singleton logger instance
     * @return Reference to the logger instance
     */
    static Logger& GetInstance() noexcept;

    /**
     * @brief Log a debug message
     * @param message Message to log
     */
    void Debug(std::string_view message) noexcept;
    void Debug(std::wstring_view message) noexcept;

    /**
     * @brief Log an info message  
     * @param message Message to log
     */
    void Info(std::string_view message) noexcept;
    void Info(std::wstring_view message) noexcept;

    /**
     * @brief Log an error message
     * @param message Message to log
     */
    void Error(std::string_view message) noexcept;
    void Error(std::wstring_view message) noexcept;

    // Static convenience methods
    static void LogDebug(std::string_view message) noexcept { GetInstance().Debug(message); }
    static void LogDebug(std::wstring_view message) noexcept { GetInstance().Debug(message); }
    static void LogInfo(std::string_view message) noexcept { GetInstance().Info(message); }
    static void LogInfo(std::wstring_view message) noexcept { GetInstance().Info(message); }
    static void LogError(std::string_view message) noexcept { GetInstance().Error(message); }
    static void LogError(std::wstring_view message) noexcept { GetInstance().Error(message); }

private:
    Logger();
    ~Logger();

    // Non-copyable, non-movable
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void LogImpl(LogLevel level, std::string_view message) noexcept;

    static constexpr std::string_view LogLevelToString(LogLevel level) noexcept {
        switch (level) {
        case LogLevel::Debug: return "DBG";
        case LogLevel::Info:  return "INF";
        case LogLevel::Error: return "ERR";
        default:              return "UNK";
        }
    }

    std::ofstream log_file_;
    bool is_initialized_{false};
};

} // namespace CrashSender
