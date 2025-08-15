#include <windows.h>
#include <wininet.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>

#include "log.hpp"

#pragma comment(lib, "wininet.lib")

// Structure to hold crash report data
struct CrashReportData {
    std::wstring url;        // Server URL
    std::wstring version;    // Application version
    std::wstring error;      // Error description
    std::wstring dump_path;  // Path to dump file
    std::wstring temp_path;  // Temporary file path
    std::wstring full_url;   // Complete URL with path
    std::wstring server_path; // Server path component
};

// Forward declarations
void ParseCommandLineArguments(int argc, wchar_t* argv[], CrashReportData& crashData);
void InitializeCrashReportData(CrashReportData& crashData);
void ProcessServerUrl(CrashReportData& crashData);
HINTERNET SendCrashReport(const CrashReportData& crashData);
void CreateMultiPartFormData(const CrashReportData& crashData, std::vector<char>& formData);
void ParseCommandLineParameter(int argc, wchar_t* argv[], const wchar_t* parameter, std::wstring& output);
bool ReadFileToBuffer(const wchar_t* filename, std::vector<char>& buffer);
void CleanupTempFiles(const CrashReportData& crashData);

// Parse command line arguments and extract crash report parameters
void ParseCommandLineArguments(int argc, wchar_t* argv[], CrashReportData& crashData) {
    if (argc < 4) {
        LogA(LogLevel::Warn, "Insufficient command line arguments");
        return;
    }

    // Parse -url= parameter
    ParseCommandLineParameter(argc, argv, L"-url=", crashData.url);

    // Parse -version= parameter
    ParseCommandLineParameter(argc, argv, L"-version=", crashData.version);

    // Parse -error= parameter
    ParseCommandLineParameter(argc, argv, L"-error=", crashData.error);

    // Parse -dump= parameter (path to crash dump file)
    ParseCommandLineParameter(argc, argv, L"-dump=", crashData.dump_path);
}

// Extract parameter value from command line
void ParseCommandLineParameter(int argc, wchar_t* argv[], const wchar_t* parameter, std::wstring& output) {
    for (int i = 0; i < argc; i++) {
        const wchar_t* pos = wcsstr(argv[i], parameter);
        if (pos) {
            pos += wcslen(parameter); // Move past parameter name
            output.assign(pos, wcslen(pos));
            break;
        }
    }
}

// Initialize crash report data structure
void InitializeCrashReportData(CrashReportData& crashData) {
    // Initialize all string members to empty
    crashData.url.clear();
    crashData.version.clear();
    crashData.error.clear();
    crashData.dump_path.clear();
    crashData.temp_path.clear();
    crashData.full_url.clear();
    crashData.server_path.clear();
}

// Process and validate the server URL
void ProcessServerUrl(CrashReportData& crashData) {
    const std::wstring httpPrefix = L"http://";

    size_t httpPos = crashData.url.find(httpPrefix);
    size_t startPos = (httpPos != std::wstring::npos) ? httpPos + 7 : 0;

    // Extract server part and path part
    std::wstring urlPart = crashData.url.substr(startPos);
    size_t slashPos = urlPart.find(L"/");

    if (slashPos != std::wstring::npos) {
        // URL has path component
        crashData.full_url = urlPart.substr(0, slashPos);
        crashData.server_path = urlPart.substr(slashPos);
    } else {
        // No path component, use entire URL as server
        crashData.full_url = urlPart;
        crashData.server_path = L"/";
    }
}

// Create multipart form data for HTTP POST
void CreateMultiPartFormData(const CrashReportData& crashData, std::vector<char>& formData) {
    std::string boundary = "--MULTIPART-DATA-BOUNDARY";
    std::string crlf = "\r\n";

    formData.clear();

    // Add version field
    std::string versionPart = boundary + crlf +
        "Content-Disposition: form-data; name=\"CRVersion\"" + crlf + crlf;
    formData.insert(formData.end(), versionPart.begin(), versionPart.end());

    // Convert version to UTF-8
    int versionLen = WideCharToMultiByte(CP_UTF8, 0, crashData.version.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (versionLen > 1) {
        std::vector<char> versionUtf8(versionLen);
        WideCharToMultiByte(CP_UTF8, 0, crashData.version.c_str(), -1, versionUtf8.data(), versionLen, nullptr, nullptr);
        formData.insert(formData.end(), versionUtf8.begin(), versionUtf8.end() - 1); // Exclude null terminator
    }
    formData.insert(formData.end(), crlf.begin(), crlf.end());

    // Add error field
    std::string errorPart = boundary + crlf +
        "Content-Disposition: form-data; name=\"error\"" + crlf + crlf;
    formData.insert(formData.end(), errorPart.begin(), errorPart.end());

    // Convert error to UTF-8
    int errorLen = WideCharToMultiByte(CP_UTF8, 0, crashData.error.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (errorLen > 1) {
        std::vector<char> errorUtf8(errorLen);
        WideCharToMultiByte(CP_UTF8, 0, crashData.error.c_str(), -1, errorUtf8.data(), errorLen, nullptr, nullptr);
        formData.insert(formData.end(), errorUtf8.begin(), errorUtf8.end() - 1); // Exclude null terminator
    }
    formData.insert(formData.end(), crlf.begin(), crlf.end());

    // Add dump file field
    std::string dumpPart = boundary + crlf +
        "Content-Disposition: form-data; name=\"dumpfile\"; filename=\"";
    formData.insert(formData.end(), dumpPart.begin(), dumpPart.end());

    // Extract filename from path
    const wchar_t* dumpPath = crashData.dump_path.c_str();
    const wchar_t* filename = wcsrchr(dumpPath, L'\\');
    if (filename) {
        filename++; // Skip the backslash
    } else {
        filename = dumpPath;
    }

    // Convert filename to UTF-8
    int filenameLen = WideCharToMultiByte(CP_UTF8, 0, filename, -1, nullptr, 0, nullptr, nullptr);
    if (filenameLen > 1) {
        std::vector<char> filenameUtf8(filenameLen);
        WideCharToMultiByte(CP_UTF8, 0, filename, -1, filenameUtf8.data(), filenameLen, nullptr, nullptr);
        formData.insert(formData.end(), filenameUtf8.begin(), filenameUtf8.end() - 1); // Exclude null terminator
    }

    std::string dumpHeader = "\"" + crlf + "Content-Type: application/octet-stream" + crlf + crlf;
    formData.insert(formData.end(), dumpHeader.begin(), dumpHeader.end());
}

// Read file contents into buffer
bool ReadFileToBuffer(const wchar_t* filename, std::vector<char>& buffer) {
    Log(LogLevel::Info, L"Reading file: " + std::wstring(filename));
    
    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        Log(LogLevel::Error, L"Failed to open file: " + std::wstring(filename));
        return false;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        LogA(LogLevel::Error, "Failed to get file size");
        CloseHandle(hFile);
        return false;
    }

    LogA(LogLevel::Info, "File size: " + std::to_string(fileSize.QuadPart) + " bytes");
    buffer.resize(static_cast<size_t>(fileSize.QuadPart));

    DWORD bytesRead = 0;
    BOOL success = ReadFile(hFile, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr);

    CloseHandle(hFile);

    if (!success || bytesRead != buffer.size()) {
        LogA(LogLevel::Error, "Failed to read file contents");
        buffer.clear();
        return false;
    }

    LogA(LogLevel::Info, "File read successfully");
    return true;
}

// Send crash report via HTTP POST
HINTERNET SendCrashReport(const CrashReportData& crashData) {
    LogA(LogLevel::Info, "Initializing HTTP connection");
    HINTERNET hInternet = InternetOpenW(L"CrashReport", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (!hInternet) {
        LogA(LogLevel::Error, "Failed to initialize WinINet");
        return nullptr;
    }

    // Connect to server
    Log(LogLevel::Info, L"Connecting to server: " + crashData.full_url);
    HINTERNET hConnect = InternetConnectW(
        hInternet,
        crashData.full_url.c_str(),
        INTERNET_DEFAULT_HTTP_PORT,
        nullptr, nullptr,
        INTERNET_SERVICE_HTTP,
        0, 0
    );

    if (!hConnect) {
        LogA(LogLevel::Error, "Failed to connect to server");
        InternetCloseHandle(hInternet);
        return nullptr;
    }

    // Create HTTP request
    Log(LogLevel::Info, L"Creating HTTP POST request to: " + crashData.server_path);
    HINTERNET hRequest = HttpOpenRequestW(
        hConnect,
        L"POST",
        crashData.server_path.c_str(),
        L"HTTP/1.0",
        nullptr, nullptr,
        INTERNET_FLAG_NO_CACHE_WRITE,
        0
    );

    if (!hRequest) {
        LogA(LogLevel::Error, "Failed to create HTTP request");
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return nullptr;
    }

    // Prepare multipart form data
    LogA(LogLevel::Info, "Creating multipart form data");
    std::vector<char> formHeader;
    CreateMultiPartFormData(crashData, formHeader);

    std::vector<char> dumpFileData;
    if (!ReadFileToBuffer(crashData.dump_path.c_str(), dumpFileData)) {
        LogA(LogLevel::Error, "Failed to read dump file");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return nullptr;
    }

    std::string boundary = "\r\n--MULTIPART-DATA-BOUNDARY--";
    std::vector<char> formFooter(boundary.begin(), boundary.end());

    // Set HTTP headers
    const wchar_t* headers =
        L"Content-Transfer-Encoding: binary\r\n"
        L"Content-Type: multipart/form-data; charset=UTF-8; boundary=MULTIPART-DATA-BOUNDARY\r\n";

    if (!HttpAddRequestHeadersW(hRequest, headers, static_cast<DWORD>(-1), HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE)) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return nullptr;
    }

    // Calculate total content length
    DWORD totalLength = static_cast<DWORD>(formHeader.size() + dumpFileData.size() + formFooter.size());
    LogA(LogLevel::Info, "Total upload size: " + std::to_string(totalLength) + " bytes");

    // Prepare request
    INTERNET_BUFFERSW buffers = {0};
    buffers.dwStructSize = sizeof(INTERNET_BUFFERSW);
    buffers.dwBufferTotal = totalLength;

    if (!HttpSendRequestExW(hRequest, &buffers, nullptr, 0, 0)) {
        LogA(LogLevel::Error, "Failed to prepare HTTP request");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return nullptr;
    }

    // Send form data
    LogA(LogLevel::Info, "Uploading crash report data");
    DWORD bytesWritten;
    InternetWriteFile(hRequest, formHeader.data(), static_cast<DWORD>(formHeader.size()), &bytesWritten);
    InternetWriteFile(hRequest, dumpFileData.data(), static_cast<DWORD>(dumpFileData.size()), &bytesWritten);
    InternetWriteFile(hRequest, formFooter.data(), static_cast<DWORD>(formFooter.size()), &bytesWritten);

    // Complete the request
    LogA(LogLevel::Info, "Finalizing HTTP request");
    HttpEndRequestW(hRequest, nullptr, 0, 0);

    // Cleanup
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);

    return hInternet;
}

void CleanupTempFiles(const CrashReportData& crashData) {
    if (!crashData.temp_path.empty()) {
        DeleteFileW(crashData.temp_path.c_str());
    }
}

int wmain(int argc, wchar_t* argv[]) {
    std::setlocale(LC_ALL, "");

    // Initialize logging
    if (!InitializeLogging()) {
        std::wcerr << L"Warning: Failed to initialize logging" << std::endl;
    }

    auto crashData = CrashReportData{};

    InitializeCrashReportData(crashData);
    
    LogA(LogLevel::Info, "Parsing command line arguments");
    ParseCommandLineArguments(argc, argv, crashData);
    
    // Log parsed parameters
    Log(LogLevel::Info, L"URL: " + crashData.url);
    Log(LogLevel::Info, L"Version: " + crashData.version);
    Log(LogLevel::Info, L"Error: " + crashData.error);
    Log(LogLevel::Info, L"Dump path: " + crashData.dump_path);
    
    LogA(LogLevel::Info, "Processing server URL");
    ProcessServerUrl(crashData);
    
    Log(LogLevel::Info, L"Server: " + crashData.full_url);
    Log(LogLevel::Info, L"Path: " + crashData.server_path);

    LogA(LogLevel::Info, "Sending crash report");
    HINTERNET hInternet = SendCrashReport(crashData);
    if (hInternet) {
        LogA(LogLevel::Info, "Crash report sent successfully");
        InternetCloseHandle(hInternet);

        LogA(LogLevel::Info, "Cleaning up temporary files");
        CleanupTempFiles(crashData);
    } else {
        LogA(LogLevel::Error, "Failed to send crash report");
    }
    
    CloseLogging();
    return 0;
}
