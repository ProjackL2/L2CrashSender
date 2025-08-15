#pragma once

#include <optional>
#include <string>

#include "crash_report_data.h"

namespace CrashSender {

/**
 * @brief Command line argument parser for crash reports
 */
class CommandLineParser {
public:
    /**
     * @brief Parse command line arguments into crash report data
     * @param argc Number of arguments
     * @param argv Argument values
     * @param error_message
     * @return Crash report data on success, error message on failure
     */
    [[nodiscard]]
    static std::optional<CrashReportData> Parse(int argc, wchar_t* argv[], std::string& error_message) noexcept;

private:
    static bool ParseParameter(int argc, wchar_t* const argv[], std::wstring_view parameter, std::wstring& output) noexcept;
    static bool ProcessErrorContent(CrashReportData& data) noexcept;
    static void ProcessServerUrl(CrashReportData& data) noexcept;
};

} // namespace CrashSender
