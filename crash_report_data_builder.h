#pragma once

#include <optional>
#include <string>

#include "crash_report_data.h"

namespace CrashSender {

/**
 * @brief CrashReportData builder
 */
class CrashReportDataBuilder {
public:
    /**
     * @brief Parse command line arguments into crash report data
     * @param argc Number of arguments
     * @param argv Argument values
     * @param error_message
     * @return Crash report data on success, error message on failure
     */
    [[nodiscard]]
    static std::optional<CrashReportData> ParseCommandLine(int argc, wchar_t* argv[], std::string& error_message) noexcept;

    static void ProcessServerUrl(CrashReportData& data) noexcept;
    static void ProcessLogFiles(CrashReportData& data) noexcept;
    static bool ProcessErrorContent(CrashReportData& data) noexcept;
private:
    static bool ParseParameter(int argc, wchar_t* const argv[], std::wstring_view parameter, std::wstring& output) noexcept;
};

} // namespace CrashSender
