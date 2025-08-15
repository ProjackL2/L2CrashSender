// Structure to hold crash report data

#include <string>
#include <string_view>
#include <vector>

struct CrashReportData {
    std::wstring url{};          // Server URL
    std::wstring version{};      // Application version
    std::wstring error{};        // Error description
    std::wstring dump_path{};    // Path to dump file
    std::wstring temp_path{};    // Temporary file path
    std::wstring full_url{};     // Complete URL with path
    std::wstring server_path{};  // Server path component

    void clear() noexcept;
};

// Forward declarations
void ParseCommandLineArguments(int argc, wchar_t* argv[], CrashReportData& crashData);
void ParseCommandLineParameter(int argc, wchar_t* const argv[], std::wstring_view parameter, std::wstring& output) noexcept;
void ProcessErrorContent(CrashReportData& crashData) noexcept;
void ProcessServerUrl(CrashReportData& crashData) noexcept;
void CreateMultiPartFormData(const CrashReportData& crashData, std::vector<char>& formData);
bool ReadFileToBuffer(std::wstring_view filename, std::vector<char>& buffer) noexcept;
bool SendCrashReport(const CrashReportData& crashData) noexcept;
void CleanupTempFiles(const CrashReportData& crashData) noexcept;
