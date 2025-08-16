#include <filesystem>

#include <windows.h>
#include <wininet.h>

#include "utils.h"
#include "logger.h"
#include "http_client.h"

#pragma comment(lib, "wininet.lib")

namespace CrashSender {

namespace {

    constexpr std::string_view BOUNDARY = "--MULTIPART-DATA-BOUNDARY";
    constexpr std::string_view CRLF = "\r\n";

    /**
     * @brief RAII wrapper for WinINet handles
     */
    class InternetHandle {
    public:
        InternetHandle() = default;
        
        explicit InternetHandle(HINTERNET handle) noexcept : handle_(handle) {}
        
        ~InternetHandle() noexcept {
            if (handle_) {
                InternetCloseHandle(handle_);
            }
        }

        // Non-copyable, movable
        InternetHandle(const InternetHandle&) = delete;
        InternetHandle& operator=(const InternetHandle&) = delete;
        
        InternetHandle(InternetHandle&& other) noexcept : handle_(other.handle_) {
            other.handle_ = nullptr;
        }
        
        InternetHandle& operator=(InternetHandle&& other) noexcept {
            if (this != &other) {
                if (handle_) {
                    InternetCloseHandle(handle_);
                }
                handle_ = other.handle_;
                other.handle_ = nullptr;
            }
            return *this;
        }

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
} // anonymous namespace

bool HttpClient::SendCrashReport(const CrashReportData& data, std::string& error_message) noexcept {
    try {
        Logger::LogInfo(L"Attempting to send crash report to " + data.full_url);
        
        // Initialize WinINet
        InternetHandle internet(InternetOpenW(L"L2CrashSender/1.0", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0));
        if (!internet) {
            error_message = "Failed to initialize WinINet";
            return false;
        }

        // Connect to server
        Logger::LogDebug(L"Connecting to server: " + data.full_url);
        InternetHandle connect(InternetConnectW(internet.get(), data.full_url.c_str(),
                                               INTERNET_DEFAULT_HTTP_PORT, nullptr, nullptr,
                                               INTERNET_SERVICE_HTTP, 0, 0));
        if (!connect) {
            error_message = "Failed to connect to server: " + TextUtils::WideToUtf8(data.full_url);
            return false;
        }

        // Create HTTP request
        Logger::LogDebug(L"Creating HTTP POST request to: " + data.server_path);
        InternetHandle request(HttpOpenRequestW(connect.get(), L"POST", data.server_path.c_str(),
                                               L"HTTP/1.1", nullptr, nullptr,
                                               INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0));
        if (!request) {
            error_message = "Failed to create HTTP request";
            return false;
        }

        // Set HTTP headers
        const std::wstring headers = 
            L"Content-Type: multipart/form-data; boundary=MULTIPART-DATA-BOUNDARY\r\n"
            L"Content-Transfer-Encoding: binary\r\n";

        if (!HttpAddRequestHeadersW(request.get(), headers.c_str(), 
                                   static_cast<DWORD>(-1), 
                                   HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE)) {
            error_message = "Failed to add HTTP headers";
            return false;
        }

        // Prepare multipart form data
        std::vector<char> form_data;
        if (!CreateMultipartFormData(data, form_data, error_message)) {
            return false;
        }

        // Calculate total content length
        const auto total_length = static_cast<DWORD>(form_data.size());
        Logger::LogDebug("Total upload size: " + std::to_string(total_length) + " bytes");

        // Prepare request
        INTERNET_BUFFERSW buffers{};
        buffers.dwStructSize = sizeof(INTERNET_BUFFERSW);
        buffers.dwBufferTotal = total_length;

        if (!HttpSendRequestExW(request.get(), &buffers, nullptr, 0, 0)) {
            error_message = "Failed to prepare HTTP request";
            return false;
        }

        // Send data
        Logger::LogDebug("Uploading crash report data");
        DWORD bytes_written = 0;
        
        // Send form header
        if (!InternetWriteFile(request.get(), form_data.data(), static_cast<DWORD>(form_data.size()), &bytes_written)) {
            error_message = "Failed to upload form data";
            return false;
        }

        // Complete the request
        Logger::LogDebug("Finalizing HTTP request: body=" + std::to_string(bytes_written));
        if (!HttpEndRequestW(request.get(), nullptr, 0, 0)) {
            error_message = "Failed to finalize HTTP request";
            return false;
        }

        // Check HTTP status code
        DWORD status_code = 0;
        DWORD status_size = sizeof(status_code);
        if (!HttpQueryInfoW(request.get(), HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                           &status_code, &status_size, nullptr)) {
            error_message = "Failed to query HTTP status";
            return false;
        }

        Logger::LogDebug("Server responded with status: " + std::to_string(status_code));

        // Read response body
        std::string response_body;
        char buffer[4096] = {};
        DWORD bytes_read = 0;
        while (InternetReadFile(request.get(), buffer, sizeof(buffer), &bytes_read) && bytes_read > 0) {
            response_body.append(buffer, bytes_read);
        }

        // Check for success status (2xx)
        if (status_code < 200 || status_code >= 300) {
            error_message = "Server rejected crash report (HTTP " + std::to_string(status_code) + ")";
            if (!response_body.empty()) {
                error_message += ": " + response_body;
            }
            return false;
        }

        Logger::LogInfo("Crash report sent successfully");
        return true;
    }
    catch (const std::exception& e) {
        error_message = "Exception during HTTP request: " + std::string(e.what());
        return false;
    }
    catch (...) {
        error_message = "Unknown exception during HTTP request";
        return false;
    }
}

bool HttpClient::CreateMultipartFormData(const CrashReportData& data, std::vector<char>& output, std::string& error_message) noexcept {
    try {
        output.clear();

        // Add version field
        TextUtils::AppendString(output, BOUNDARY);
        TextUtils::AppendString(output, CRLF);
        TextUtils::AppendString(output, "Content-Disposition: form-data; name=\"CRVersion\"");
        TextUtils::AppendString(output, CRLF);
        TextUtils::AppendString(output, CRLF);
        TextUtils::AppendString(output, data.version);
        TextUtils::AppendString(output, CRLF);

        // Add error field
        TextUtils::AppendString(output, BOUNDARY);
        TextUtils::AppendString(output, CRLF);
        TextUtils::AppendString(output, "Content-Disposition: form-data; name=\"error\"");
        TextUtils::AppendString(output, CRLF);
        TextUtils::AppendString(output, CRLF);
        TextUtils::AppendString(output, data.error);
        TextUtils::AppendString(output, CRLF);

        if (!data.dump_path.empty() && !AddFileToMultipartData("dumpfile", data.dump_path, output, error_message)) {
            return false;
        }

        if (!data.game_log_path.empty() && !AddFileToMultipartData("gamelog", data.game_log_path, output, error_message)) {
            // Not-crtitical failure
            Logger::LogError(error_message);
            error_message = "";
        }
        
        if (!data.network_log_path.empty() && !AddFileToMultipartData("networklog", data.network_log_path, output, error_message)) {
            // Not-crtitical failure
            Logger::LogError(error_message);
            error_message = "";
        }

        // Create form footer
        const std::string footer = std::string(CRLF) + std::string(BOUNDARY) + "--";
        output.insert(output.end(), footer.begin(), footer.end());
        return true;
    }
    catch (const std::exception& e) {
        error_message = "Failed to create multipart form data: " + std::string(e.what());
        output.clear();
        return false;
    }
    catch (...) {
        error_message = "Unknown exception while creating multipart form data";
        output.clear();
        return false;
    }
}

bool HttpClient::AddFileToMultipartData(std::string_view name, std::wstring_view filepath, std::vector<char>& output, std::string& error_message) noexcept {
    Logger::LogDebug(L"Try to add multipart data file: " + std::wstring(filepath));

    // Add file field
    TextUtils::AppendString(output, BOUNDARY);
    TextUtils::AppendString(output, CRLF);
    TextUtils::AppendString(output, "Content-Disposition: form-data; name=\"");
    TextUtils::AppendString(output, name);
    TextUtils::AppendString(output, "\"; filename=\"");

    try {
        // Extract filename from path
        const std::filesystem::path path(filepath);
        const std::wstring filename = path.filename().wstring();
        TextUtils::AppendString(output, filename);
    }
    catch (...) {
        // Fallback: use the whole path as filename
        TextUtils::AppendString(output, filepath);
    }

    TextUtils::AppendString(output, "\"");
    TextUtils::AppendString(output, CRLF);
    TextUtils::AppendString(output, "Content-Type: application/octet-stream");
    TextUtils::AppendString(output, CRLF);
    TextUtils::AppendString(output, CRLF);

    if (!FileUtils::AppendToBuffer(filepath, output, error_message)) {
        return false;
    }
    return true;
}

} // namespace CrashSender

