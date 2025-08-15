#include <fstream>
#include <string_view>
#include <vector>

#include <windows.h>

#include "logger.h"
#include "utils.h"
#include "command_line_parser.h"

namespace CrashSender {

std::optional<CrashReportData> CommandLineParser::Parse(int argc, wchar_t* argv[], 
                                                        std::string& error_message) noexcept {
    try {
        if (argc < 5) {
            error_message = "Insufficient command line arguments (minimum 4 required)";
            return std::nullopt;
        }

        CrashReportData data;

        // Parse required parameters
        if (!ParseParameter(argc, argv, L"-url=", data.url) || data.url.empty()) {
            error_message = "Missing or empty -url parameter";
            return std::nullopt;
        }

        if (!ParseParameter(argc, argv, L"-version=", data.version) || data.version.empty()) {
            error_message = "Missing or empty -version parameter";
            return std::nullopt;
        }

        if (!ParseParameter(argc, argv, L"-error=", data.temp_path) || data.temp_path.empty()) {
            error_message = "Missing or empty -error parameter";
            return std::nullopt;
        }

        if (!ParseParameter(argc, argv, L"-dump=", data.dump_path) || data.dump_path.empty()) {
            error_message = "Missing or empty -dump parameter";
            return std::nullopt;
        }

        // Process error content and server URL
        if (!ProcessErrorContent(data)) {
            error_message = "Failed to process error file content";
            return std::nullopt;
        }

        ProcessServerUrl(data);

        if (!data.IsValid()) {
            error_message = "Parsed data is invalid";
            return std::nullopt;
        }

        return data;
    }
    catch (const std::exception& e) {
        error_message = "Exception during parsing: " + std::string(e.what());
        return std::nullopt;
    }
    catch (...) {
        error_message = "Unknown exception during parsing";
        return std::nullopt;
    }
}

bool CommandLineParser::ParseParameter(int argc, wchar_t* const argv[], 
                                      std::wstring_view parameter, 
                                      std::wstring& output) noexcept {
    try {
        for (int i = 0; i < argc; ++i) {
            if (!argv[i]) {
                continue;
            }
            
            const std::wstring_view arg(argv[i]);
            if (arg.starts_with(parameter)) {
                output = arg.substr(parameter.length());
                return true;
            }
        }
        return false;
    } 
    catch (...) {
        return false;
    }
}

bool CommandLineParser::ProcessErrorContent(CrashReportData& data) noexcept {
    try {
        // Check if file exists using Windows API
        const DWORD attributes = GetFileAttributesW(data.temp_path.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            Logger::LogError("Error file does not exist: " + TextUtils::WideToUtf8(data.temp_path));
            return false;
        }

        // Read file content
        std::vector<char> buffer;
        const HANDLE file_handle = CreateFileW(data.temp_path.c_str(), GENERIC_READ, 
                                             FILE_SHARE_READ, nullptr, OPEN_EXISTING, 
                                             FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_handle == INVALID_HANDLE_VALUE) {
            Logger::LogError("Failed to open error file: " + TextUtils::WideToUtf8(data.temp_path));
            data.error = L"Failed to read error content";
            return true;
        }

        FileGuard guard{ file_handle };

        LARGE_INTEGER file_size;
        if (!GetFileSizeEx(file_handle, &file_size) || file_size.QuadPart > SIZE_MAX) {
            Logger::LogError("Failed to get error file size");
            data.error = L"Failed to read error content";
            return true;
        }

        buffer.resize(static_cast<size_t>(file_size.QuadPart));
        DWORD bytes_read = 0;
        if (!ReadFile(file_handle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read, nullptr) ||
            bytes_read != buffer.size()) {
            Logger::LogError("Failed to read error file content");
            data.error = L"Failed to read error content";
            return true;
        }

        if (buffer.size() % 2 != 0) {
            Logger::LogError("Error file has invalid size for wide characters");
            data.error = L"Invalid error file format";
            return true;
        }

        // Convert buffer to wide string (assuming UTF-16)
        const size_t char_count = buffer.size() / 2;
        data.error = std::wstring(reinterpret_cast<const wchar_t*>(buffer.data()), char_count);
        
        return true;
    }
    catch (...) {
        data.error = L"Exception occurred while processing error content";
        return true; // Non-critical failure
    }
}

void CommandLineParser::ProcessServerUrl(CrashReportData& data) noexcept {
    try {
        constexpr std::wstring_view http_prefix = L"http://";

        size_t start_pos = 0;
        
        if (const auto http_pos = data.url.find(http_prefix); http_pos != std::wstring::npos) {
            start_pos = http_pos + http_prefix.length();
        }

        const std::wstring url_part = data.url.substr(start_pos);
        if (const auto slash_pos = url_part.find(L'/'); slash_pos != std::wstring::npos) {
            data.full_url = url_part.substr(0, slash_pos);
            data.server_path = url_part.substr(slash_pos);
        } else {
            data.full_url = url_part;
            data.server_path = L"/";
        }
    } 
    catch (...) {
        // Fallback to original URL parsing
        data.full_url = data.url;
        data.server_path = L"/";
    }
}

} // namespace CrashSender
