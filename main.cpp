#include <iostream>
#include <clocale>

#include "utils.h"
#include "logger.h"
#include "crash_report_data.h"
#include "crash_report_data_builder.h"
#include "http_client.h"
#include "main.h"

namespace CrashSender {

int RunApplication(int argc, wchar_t* argv[]) noexcept {
    try {
        // Set locale for proper character handling
        std::setlocale(LC_ALL, "");

        // Parse command line arguments
        std::string parse_error;
        auto crash_data = CrashReportDataBuilder::ParseCommandLine(argc, argv, parse_error);
        if (!crash_data) {
            Logger::LogError("Command line parsing failed: " + parse_error);
            return 1;
        }

        CrashReportDataBuilder::ProcessServerUrl(crash_data.value());
        CrashReportDataBuilder::ProcessLogFiles(crash_data.value());

        if (!CrashReportDataBuilder::ProcessErrorContent(crash_data.value())) {
            Logger::LogError("Failed to process error file content");
            return 1;
        }

        // Log parsed data for debugging
        Logger::LogDebug(L"Version: " + crash_data->version);
        Logger::LogDebug(L"Error file path: " + crash_data->temp_path);
        Logger::LogDebug(L"Dump path: " + crash_data->dump_path);
        Logger::LogDebug(L"Game log path: " + crash_data->game_log_path);
        Logger::LogDebug(L"Network log path: " + crash_data->network_log_path);
        Logger::LogDebug(L"URL: " + crash_data->url);
        Logger::LogDebug(L"Server: " + crash_data->full_url);
        Logger::LogDebug(L"Path: " + crash_data->server_path);

        // Validate crash data
        if (!crash_data->IsValid()) {
            Logger::LogError("Invalid crash report data");
            return 1;
        }

        // Send crash report
        Logger::LogInfo(L"Sending crash report to " + crash_data->full_url);
        std::string send_error;
        if (HttpClient::SendCrashReport(*crash_data, send_error)) {
            // Clean up temporary files on success
            FileUtils::CleanupTempFiles(*crash_data);
            Logger::LogInfo("Temporary files cleaned up");
            return 0;
        } else {
            Logger::LogError("Failed to send crash report: " + send_error);
            return 1;
        }
    }
    catch (const std::exception& e) {
        Logger::LogError("Unhandled exception: " + std::string(e.what()));
        return 1;
    }
    catch (...) {
        Logger::LogError("Unknown unhandled exception occurred");
        return 1;
    }
}

} // namespace CrashSender

/**
 * @brief Main entry point for the application
 */
int wmain(int argc, wchar_t* argv[]) {
    return CrashSender::RunApplication(argc, argv);
}
