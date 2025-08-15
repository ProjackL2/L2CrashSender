#include <iostream>

#include "utils.h"
#include "logger.h"

namespace CrashSender {

Logger& Logger::GetInstance() noexcept {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    try {
        log_file_.open("L2CrashSender.log", std::ios::out);
        if (log_file_.is_open()) {
            is_initialized_ = true;
            LogImpl(LogLevel::Info, "L2CrashSender started");
        }
    }
    catch (...) {
        // Continue without logging if a file cannot be opened
        is_initialized_ = false;
    }
}

Logger::~Logger() {
    try {
        if (is_initialized_ && log_file_.is_open()) {
            LogImpl(LogLevel::Info, "L2CrashSender finished");
            log_file_.flush();
            log_file_.close();
        }
    }
    catch (...) {
        // Ignore errors during cleanup
    }
}

void Logger::Debug(std::string_view message) noexcept {
    LogImpl(LogLevel::Debug, message);
}

void Logger::Debug(std::wstring_view message) noexcept {
    LogImpl(LogLevel::Debug, TextUtils::WideToUtf8(message));
}

void Logger::Info(std::string_view message) noexcept {
    LogImpl(LogLevel::Info, message);
}

void Logger::Info(std::wstring_view message) noexcept {
    LogImpl(LogLevel::Info, TextUtils::WideToUtf8(message));
}

void Logger::Error(std::string_view message) noexcept {
    LogImpl(LogLevel::Error, message);
}

void Logger::Error(std::wstring_view message) noexcept {
    LogImpl(LogLevel::Error, TextUtils::WideToUtf8(message));
}

void Logger::LogImpl(LogLevel level, std::string_view message) noexcept {
    if (!is_initialized_) {
        return;
    }

    try {
        if (log_file_.is_open()) {
            log_file_ << TimeUtils::GetCurrentTimestamp() << " [" << LogLevelToString(level) << "] " << message << '\n';
            log_file_.flush(); // Ensure immediate write for crash reporting tool
        }
    } catch (...) {
        // Ignore logging errors to prevent cascading failures
    }
}

} // namespace CrashSender
