#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <windows.h>

#include "crash_report_data.h"

namespace CrashSender {

/**
 * @brief RAII for a file handle
 */
class FileGuard {
public:
    explicit FileGuard(HANDLE h);
    ~FileGuard();
private:
    HANDLE handle;
};

/**
 * @brief Utility functions for file operations
 */
struct FileUtils {
    /**
     * @brief Delete a file safely
     * @param filename Path to file to delete
     * @return true if deleted successfully or file doesn't exist
     */
    [[nodiscard]]
    static bool RemoveFile(std::wstring_view filename) noexcept;

    /**
     * @brief Check if file exists
     * @param filename Path to check
     * @return true if file exists
     */
    [[nodiscard]]
    static bool FileExists(std::wstring_view filename) noexcept;

    /**
     * @brief Get file size
     * @param filename Path to file
     * @return File size in bytes, -1 on error
     */
    [[nodiscard]]
    static int64_t GetFileSize(std::wstring_view filename) noexcept;

    /**
     * @brief Read file to buffer
     * @param filename Path to file
     * @param buffer Target buffer to load data
     * @param error_message Placeholder for error if it will occurs
     * @return File size in bytes, -1 on error
     */
    [[nodiscard]]
    static bool AppendToBuffer(std::wstring_view filename, std::vector<char>& buffer, std::string& error_message) noexcept;

    /**
     * @brief Cleanup temporary files used in crash reporting
     * @param data Crash report data containing file paths
     */
    static void CleanupTempFiles(const CrashReportData& data) noexcept;
};


struct TextUtils {
    /**
     * @brief Convert wide string to UTF-8 string
     * @param wstr Wide string to convert
     * @return UTF-8 encoded string, empty on failure
     */
    static std::string WideToUtf8(std::wstring_view wstr) noexcept;

    static void AppendString(std::vector<char>& output, std::string_view str) noexcept;
    static void AppendString(std::vector<char>& output, std::wstring_view wstr) noexcept;
};


struct TimeUtils {
    [[nodiscard]]
    static std::string GetCurrentTimestamp() noexcept;
};

} // namespace CrashSender
