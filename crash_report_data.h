#pragma once

#include <string>

namespace CrashSender {

/**
 * @brief Data structure containing crash report information
 */
struct CrashReportData {
    std::wstring url{};              ///< Server URL
    std::wstring version{};          ///< Application version
    std::wstring error{};            ///< Error description  
    std::wstring dump_path{};        ///< Path to dump file
    std::wstring temp_path{};        ///< Temporary file path
    std::wstring full_url{};         ///< Complete URL with path
    std::wstring server_path{};      ///< Server path component
    std::wstring game_log_path{};    ///< Game log path
    std::wstring network_log_path{}; ///< Network log path

    /**
     * @brief Clear all data fields
     */
    void Clear() noexcept;

    /**
     * @brief Check if required data is present
     * @return true if data is valid for sending
     */
    [[nodiscard]]
    bool IsValid() const noexcept;
};

} // namespace CrashSender

