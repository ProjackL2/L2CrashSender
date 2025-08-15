#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

#include "common.h"
#include "logger.h"
#include "main.h"

class InternetHandle {
public:
    InternetHandle() = default;

    explicit InternetHandle(HINTERNET handle) noexcept
        : handle_(handle) {
    }

    ~InternetHandle() noexcept {
        if (handle_) {
            InternetCloseHandle(handle_);
        }
    }

    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;

    [[nodiscard]]
    HINTERNET get() const noexcept {
        return handle_;
    }

    [[nodiscard]]
    explicit operator bool() const noexcept {
        return handle_ != nullptr;
    }

private:
    HINTERNET handle_ = nullptr;
};

void CrashReportData::clear() noexcept {
    url.clear();
    version.clear();
    temp_path.clear();
    error.clear();
    dump_path.clear();
    full_url.clear();
    server_path.clear();
}

void ParseCommandLineArguments(int argc, wchar_t* argv[], CrashReportData& crashData) {
    if (argc < 5) {
        throw std::exception("Insufficient command line arguments");
    }

    // Parse parameters
    ParseCommandLineParameter(argc, argv, L"-url=", crashData.url);
    ParseCommandLineParameter(argc, argv, L"-version=", crashData.version);
    ParseCommandLineParameter(argc, argv, L"-error=", crashData.temp_path);
    ParseCommandLineParameter(argc, argv, L"-dump=", crashData.dump_path);

    ProcessErrorContent(crashData);
    ProcessServerUrl(crashData);
}

void ParseCommandLineParameter(int argc, wchar_t* const argv[], std::wstring_view parameter, std::wstring& output) noexcept {
    try {
        for (int i = 0; i < argc; ++i) {
            if (!argv[i]) continue;
            
            const std::wstring_view arg(argv[i]);
            if (arg.starts_with(parameter)) {
                output = arg.substr(parameter.length());
                break;
            }
        }
    } catch (...) {
        // Ignore parsing errors
    }
}

void ProcessErrorContent(CrashReportData& crashData) noexcept {
    try {
        std::vector<char> errorFileData;
        if (!ReadFileToBuffer(crashData.temp_path, errorFileData)) {
            Logger::Error("Failed to read error file");
            return;
        }
        crashData.error = std::wstring((wchar_t*)(errorFileData.data()), errorFileData.size() / 2);
    }
    catch (...) {
        crashData.error = L"Not parsed";
    }
}

void ProcessServerUrl(CrashReportData& crashData) noexcept {
    try {
        constexpr std::wstring_view httpPrefix = L"http://";

        size_t startPos = 0;
        if (const auto httpPos = crashData.url.find(httpPrefix); httpPos != std::wstring::npos) {
            startPos = httpPos + httpPrefix.length();
        }

        const std::wstring urlPart = crashData.url.substr(startPos);
        if (const auto slashPos = urlPart.find(L'/'); slashPos != std::wstring::npos) {
            crashData.full_url = urlPart.substr(0, slashPos);
            crashData.server_path = urlPart.substr(slashPos);
        } else {
            crashData.full_url = urlPart;
            crashData.server_path = L"/";
        }
    } catch (...) {
        crashData.full_url = crashData.url;
        crashData.server_path = L"/";
    }
}

void CreateMultiPartFormData(const CrashReportData& crashData, std::vector<char>& formData) {
    try {
        constexpr std::string_view boundary = "--MULTIPART-DATA-BOUNDARY";
        constexpr std::string_view crlf = "\r\n";

        formData.clear();

        auto appendString = [&formData](std::string_view str) {
            formData.insert(formData.end(), str.begin(), str.end());
        };

        auto appendWideString = [&formData](const std::wstring& wstr) {
            const std::string utf8Str = WideToUtf8(wstr);
            formData.insert(formData.end(), utf8Str.begin(), utf8Str.end());
        };

        // Add version field
        appendString(boundary);
        appendString(crlf);
        appendString("Content-Disposition: form-data; name=\"CRVersion\"");
        appendString(crlf);
        appendString(crlf);
        appendWideString(crashData.version);
        appendString(crlf);

        // Add error field
        appendString(boundary);
        appendString(crlf);
        appendString("Content-Disposition: form-data; name=\"error\"");
        appendString(crlf);
        appendString(crlf);
        appendWideString(crashData.error);
        appendString(crlf);

        // Add dump file field
        appendString(boundary);
        appendString(crlf);
        appendString("Content-Disposition: form-data; name=\"dumpfile\"; filename=\"");

        // Extract filename from path using std::filesystem
        try {
            const std::filesystem::path path(crashData.dump_path);
            const std::wstring filename = path.filename().wstring();
            appendWideString(filename);
        } catch (...) {
            // Fallback: use the whole path as filename
            appendWideString(crashData.dump_path);
        }

        appendString("\"");
        appendString(crlf);
        appendString("Content-Type: application/octet-stream");
        appendString(crlf);
        appendString(crlf);
    } catch (...) {
        formData.clear();
        Logger::Error("Failed to create multipart form data");
    }
}

// Read file contents into buffer
[[nodiscard]]
bool ReadFileToBuffer(std::wstring_view filename, std::vector<char>& buffer) noexcept {
    try {
        Logger::Debug(std::wstring(L"Reading file: ") + std::wstring(filename));
        
        // Use Windows API for better control
        const HANDLE hFile = CreateFileW(filename.data(), GENERIC_READ, FILE_SHARE_READ, 
                                        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            Logger::Error(std::wstring(L"Failed to open file: ") + std::wstring(filename));
            return false;
        }

        // RAII wrapper for file handle
        struct FileHandleGuard {
            HANDLE handle;
            explicit FileHandleGuard(HANDLE h) : handle(h) {}
            ~FileHandleGuard() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
        } fileGuard(hFile);

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile, &fileSize)) {
            Logger::Error("Failed to get file size");
            return false;
        }

        if (fileSize.QuadPart > SIZE_MAX) {
            Logger::Error("File too large to read into memory");
            return false;
        }

        Logger::Debug("File size: " + std::to_string(fileSize.QuadPart) + " bytes");
        buffer.resize(static_cast<size_t>(fileSize.QuadPart));

        DWORD bytesRead = 0;
        const BOOL success = ReadFile(hFile, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr);

        if (!success || bytesRead != buffer.size()) {
            Logger::Error("Failed to read file contents");
            buffer.clear();
            return false;
        }

        Logger::Debug("File read successfully");
        return true;
    } catch (...) {
        Logger::Error("Exception occurred while reading file");
        buffer.clear();
        return false;
    }
}

bool SendCrashReport(const CrashReportData& crashData) noexcept {
    try {
        Logger::Info(L"Try to send crash report to " + crashData.full_url);
        
        InternetHandle hInternet(InternetOpenW(L"CrashReport", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0));
        if (!hInternet) {
            Logger::Error("Failed to initialize WinINet");
            return {};
        }

        // Connect to server
        Logger::Debug(L"Connecting to server: " + crashData.full_url);
        InternetHandle hConnect(InternetConnectW(
            hInternet.get(),
            crashData.full_url.c_str(),
            INTERNET_DEFAULT_HTTP_PORT,
            nullptr, nullptr,
            INTERNET_SERVICE_HTTP,
            0, 0
        ));

        if (!hConnect) {
            Logger::Error("Failed to connect to server");
            return {};
        }

        // Create HTTP request
        Logger::Debug(L"Creating HTTP POST request to: " + crashData.server_path);
        InternetHandle hRequest(HttpOpenRequestW(
            hConnect.get(),
            L"POST",
            crashData.server_path.c_str(),
            L"HTTP/1.1",
            nullptr, nullptr,
            INTERNET_FLAG_NO_CACHE_WRITE,
            0
        ));

        if (!hRequest) {
            Logger::Error("Failed to create HTTP request");
            return {};
        }

        // Prepare multipart form data
        Logger::Debug("Creating multipart form data");
        std::vector<char> formHeader;
        CreateMultiPartFormData(crashData, formHeader);

        std::vector<char> dumpFileData;
        if (!ReadFileToBuffer(crashData.dump_path, dumpFileData)) {
            Logger::Error("Failed to read dump file");
            return {};
        }

        constexpr std::string_view boundary = "\r\n--MULTIPART-DATA-BOUNDARY--";
        std::vector<char> formFooter(boundary.begin(), boundary.end());

        // Set HTTP headers
        constexpr const wchar_t* headers =
            L"Content-Transfer-Encoding: binary\r\n"
            L"Content-Type: multipart/form-data; charset=UTF-8; boundary=MULTIPART-DATA-BOUNDARY\r\n";

        if (!HttpAddRequestHeadersW(hRequest.get(), headers, static_cast<DWORD>(-1), HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE)) {
            Logger::Error("Failed to add HTTP headers");
            return {};
        }

        // Calculate total content length
        const auto totalLength = static_cast<DWORD>(formHeader.size() + dumpFileData.size() + formFooter.size());
        Logger::Debug("Total upload size: " + std::to_string(totalLength) + " bytes");

        // Prepare request
        INTERNET_BUFFERSW buffers{};
        buffers.dwStructSize = sizeof(INTERNET_BUFFERSW);
        buffers.dwBufferTotal = totalLength;

        if (!HttpSendRequestExW(hRequest.get(), &buffers, nullptr, 0, 0)) {
            Logger::Error("Failed to prepare HTTP request");
            return {};
        }

        // Send form data
        Logger::Debug("Uploading crash report data");
        DWORD bytesWritten = 0;
        
        if (!InternetWriteFile(hRequest.get(), formHeader.data(), static_cast<DWORD>(formHeader.size()), &bytesWritten) ||
            !InternetWriteFile(hRequest.get(), dumpFileData.data(), static_cast<DWORD>(dumpFileData.size()), &bytesWritten) ||
            !InternetWriteFile(hRequest.get(), formFooter.data(), static_cast<DWORD>(formFooter.size()), &bytesWritten)) {
            Logger::Error("Failed to upload data");
            return {};
        }

        // Complete the request
        Logger::Debug("Finalizing HTTP request");
        if (!HttpEndRequestW(hRequest.get(), nullptr, 0, 0)) {
            Logger::Error("Failed to finalize HTTP request");
            return {};
        }

        // Check HTTP status code
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        if (!HttpQueryInfoW(hRequest.get(),
            HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            &statusCode,
            &statusCodeSize,
            nullptr)) {
            Logger::Error("Failed to query HTTP status");
            return false;
        }

        Logger::Debug("Server responded with status: " + std::to_string(statusCode));

        // Read response body (if any)
        std::string responseBody;
        char buffer[4096];
        DWORD bytesRead = 0;

        while (InternetReadFile(hRequest.get(), buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            responseBody.append(buffer, bytesRead);
        }
      
        // Validate success status (2xx)
        if (statusCode < 200 || statusCode >= 300) {
            Logger::Error("Server rejected crash report");
            if (!responseBody.empty()) {
                Logger::Error("Server response: " + responseBody);
            }
            return false;
        }
        return true;
    } catch (...) {
        Logger::Error("Exception occurred during HTTP request");
        return false;
    }
}

void CleanupTempFiles(const CrashReportData& crashData) noexcept {
    try {
        if (!crashData.temp_path.empty()) {
            DeleteFileW(crashData.temp_path.c_str());
        }
        if (!crashData.dump_path.empty()) {
            DeleteFileW(crashData.dump_path.c_str());
        }
        Logger::Debug("Temporary files cleaned up");
    } catch (...) {
        Logger::Error("Failed to cleanup temporary files");
    }
}

int wmain(int argc, wchar_t* argv[]) {
    try {
        std::setlocale(LC_ALL, "");

        CrashReportData crashData{};

        ParseCommandLineArguments(argc, argv, crashData);

        Logger::Debug(L"Version: " + crashData.version);
        Logger::Debug(L"Error: " + crashData.temp_path);
        Logger::Debug(L"Dump path: " + crashData.dump_path);
        Logger::Debug(L"URL: " + crashData.url);
        Logger::Debug(L"Server: " + crashData.full_url);
        Logger::Debug(L"Path: " + crashData.server_path);
        
        Logger::Info(L"Sending crash report to " + crashData.full_url);
        if (SendCrashReport(crashData)) {
            Logger::Info("Crash report sent successfully. Cleaning up temporary files");
            CleanupTempFiles(crashData);
        } else {
            Logger::Error("Failed to send crash report");
        }
        return 0;
    } catch (const std::exception& e) {
        Logger::Error("Unhandled exception: " + std::string(e.what()));
        return 1;
    } catch (...) {
        Logger::Error("Unknown unhandled exception occurred");
        return 1;
    }
}
