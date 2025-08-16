#pragma once

#include "crash_report_data.h"
#include <vector>
#include <string>
#include <string_view>

namespace CrashSender {

/**
 * @brief HTTP client for sending crash reports
 */
class HttpClient {
public:
    /**
     * @brief Send a crash report to server
     * @param data Crash report data to send
     * @return true on success, error message on failure
     */
    [[nodiscard]]
    static bool SendCrashReport(const CrashReportData& data, std::string& error_message) noexcept;

private:
    static bool CreateMultipartFormData(const CrashReportData& data, std::vector<char>& output, std::string& error_message) noexcept;
    static bool AddFileToMultipartData(std::string_view name, std::wstring_view filename, std::vector<char>& output, std::string& error_message) noexcept;
};

} // namespace CrashSender
