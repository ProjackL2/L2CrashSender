
#pragma once

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

#include <windows.h>

#include "common.h"
#include "logger.h"

#define LOG_DEBUG_ENABLED
#define LOG_INFO_ENABLED
#define LOG_ERROR_ENABLED

static const std::string LOG_FILENAME = "L2CrashSender.log";

[[nodiscard]]
std::string GetCurrentTimestamp() noexcept {
    try {
        const auto now = std::chrono::system_clock::now();
        const auto time_t = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        tm tm;
        localtime_s(&tm, &time_t);

        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    catch (...) {
        return "TIMESTAMP_ERROR";
    }
}

Logger::Logger() {
    try {
        logFile.open(LOG_FILENAME, std::ios::out);
        if (!logFile.is_open()) {
            throw std::exception();
        }
        logFile << GetCurrentTimestamp() << " [INF] L2CrashSender started\n";
    }
    catch (...) {
        throw std::exception("Warning: Failed to initialize logging");
    }
}

Logger::~Logger() {
    try {
        if (!logFile.is_open()) {
            return;
        }

        logFile << GetCurrentTimestamp() << " [INF] L2CrashSender finished\n";
        logFile.flush();
        logFile.close();
    }
    catch (...) {
        // Ignore errors during cleanup
    }
}

void Logger::Debug(std::string_view message) noexcept {
#ifdef LOG_DEBUG_ENABLED
    Log(LogLevel::Debug, message);
#endif // LOG_DEBUG_ENABLED  
}

void Logger::Debug(std::wstring_view message) noexcept {
#ifdef LOG_DEBUG_ENABLED
    Log(LogLevel::Debug, message);
#endif  // LOG_DEBUG_ENABLED  
}

void Logger::Info(std::wstring_view message) noexcept {
#ifdef LOG_INFO_ENABLED
    Log(LogLevel::Info, message);
#endif  // LOG_INFO_ENABLED  
}

void Logger::Info(std::string_view message) noexcept {
#ifdef LOG_INFO_ENABLED
    Log(LogLevel::Info, message);
#endif  // LOG_INFO_ENABLED  
}

void Logger::Error(std::string_view message) noexcept {
#ifdef LOG_ERROR_ENABLED
    Log(LogLevel::Error, message);
#endif  // LOG_ERROR_ENABLED  
}

void Logger::Error(std::wstring_view message) noexcept {
#ifdef LOG_ERROR_ENABLED
    Log(LogLevel::Error, message);
#endif  // LOG_ERROR_ENABLED  
}

void Logger::Log(LogLevel level, std::wstring_view message) noexcept {
    Log(level, WideToUtf8(message));
}

void Logger::Log(LogLevel level, std::string_view message) noexcept {
    try {
        static const Logger guard{};
        if (!Logger::logFile.is_open()) {
            return;
        }
        
        Logger::logFile << GetCurrentTimestamp() << " [" << LogLevelToString(level) << "] " << message << '\n';
    } catch (...) {
        // Ignore logging errors
    }
}
