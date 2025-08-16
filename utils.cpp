#include <chrono>

#include "utils.h"
#include "logger.h"

namespace CrashSender {

FileGuard::FileGuard(HANDLE h) : handle(h) {
}

FileGuard::~FileGuard() {
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }
}

bool FileUtils::RemoveFile(std::wstring_view filename) noexcept {
    try {
        if (filename.empty()) {
            return true;
        }

        const BOOL result = DeleteFile(filename.data());
        if (!result) {
            const DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND) {
                return true; // File doesn't exist, consider it success
            }
            Logger::LogError("Failed to delete file: " + TextUtils::WideToUtf8(filename) + " (Error: " + std::to_string(error) + ")");
            return false;
        }
        
        Logger::LogDebug("Successfully deleted file: " + TextUtils::WideToUtf8(filename));
        return true;
    }
    catch (...) {
        Logger::LogError("Exception occurred while deleting file: " + TextUtils::WideToUtf8(filename));
        return false;
    }
}

bool FileUtils::FileExists(std::wstring_view filename) noexcept {
    try {
        if (filename.empty()) {
            return false;
        }

        const DWORD attributes = GetFileAttributesW(filename.data());
        return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }
    catch (...) {
        return false;
    }
}

int64_t FileUtils::GetFileSize(std::wstring_view filename) noexcept {
    try {
        if (filename.empty()) {
            return -1;
        }

        const HANDLE file_handle = CreateFileW(filename.data(), GENERIC_READ,
                                             FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_handle == INVALID_HANDLE_VALUE) {
            return -1;
        }

        FileGuard guard(file_handle);

        LARGE_INTEGER file_size;
        if (!GetFileSizeEx(file_handle, &file_size)) {
            return -1;
        }

        return file_size.QuadPart;
    }
    catch (...) {
        return -1;
    }
}

bool FileUtils::AppendToBuffer(std::wstring_view filepath, std::vector<char>& buffer, std::string& error_message) noexcept {
    const auto initinalSize = buffer.size();

    try {
        const HANDLE file_handle = CreateFileW(filepath.data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_handle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            if (error == ERROR_SHARING_VIOLATION) {
                error_message = "Failed to open file(" + TextUtils::WideToUtf8(filepath) + "): File is busy with other process, need to patch process";
            } else {
                error_message = "Failed to open file: " + TextUtils::WideToUtf8(filepath);
            }
            return false;
        }

        FileGuard guard{ file_handle };

        LARGE_INTEGER file_size;
        if (!GetFileSizeEx(file_handle, &file_size)) {
            error_message = "Failed to get file size";
            return false;
        }

        if (file_size.QuadPart > SIZE_MAX) {
            error_message = "File too large to read into memory";
            return false;
        }

        const auto size = static_cast<size_t>(file_size.QuadPart);
        Logger::LogDebug("File size: " + std::to_string(size) + " bytes");

        buffer.resize(initinalSize + size);

        DWORD bytes_read = 0;
        if (!ReadFile(file_handle, buffer.data() + initinalSize, static_cast<DWORD>(size), &bytes_read, nullptr) || bytes_read != size) {
            error_message = "Failed to read file contents";
            buffer.resize(initinalSize);
            return false;
        }

        Logger::LogDebug("File read successfully");
        return true;
    }
    catch (const std::exception& e) {
        error_message = "Exception while reading file: " + std::string(e.what());
        buffer.resize(initinalSize);
        return false;
    }
    catch (...) {
        error_message = "Unknown exception while reading file";
        buffer.resize(initinalSize);
        return false;
    }
}

void FileUtils::CleanupTempFiles(const CrashReportData& data) noexcept {
    try {
        bool any_deleted = false;
        
        if (!data.temp_path.empty()) {
            if (RemoveFile(data.temp_path)) {
                any_deleted = true;
            }
        }
        
        if (!data.dump_path.empty()) {
            if (RemoveFile(data.dump_path)) {
                any_deleted = true;
            }
        }
        
        if (any_deleted) {
            Logger::LogDebug("Temporary files cleaned up successfully");
        }
    }
    catch (...) {
        Logger::LogError("Exception occurred during temporary file cleanup");
    }
}

std::string TextUtils::WideToUtf8(std::wstring_view wstr) noexcept {
    if (wstr.empty()) {
        return {};
    }

    const int len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return {};
    }

    std::string str(static_cast<size_t>(len), '\0');
    const int result = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.length()), str.data(), len, nullptr, nullptr);
    return (result > 0) ? str : std::string{};
}

void TextUtils::AppendString(std::vector<char>& output, std::string_view str) noexcept {
    output.insert(output.end(), str.begin(), str.end());
};

void TextUtils::AppendString(std::vector<char>& output, std::wstring_view wstr) noexcept {
    const std::string utf8_str = WideToUtf8(wstr);
    output.insert(output.end(), utf8_str.begin(), utf8_str.end());
};

std::string TimeUtils::GetCurrentTimestamp() noexcept {
    try {
        const auto now = std::chrono::system_clock::now();
        const auto time_t = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        tm tm_buf{};
        if (localtime_s(&tm_buf, &time_t) != 0) {
            return "TIMESTAMP_ERROR";
        }

        std::ostringstream ss;
        ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    catch (...) {
        return "TIMESTAMP_ERROR";
    }
}


} // namespace CrashSender
