# L2 Crash Sender

A lightweight C++ crash report sender utility designed to automatically upload crash dumps and error information to a remote server. Built with modern C++20 practices and robust error handling.

## Features

- **Automatic Crash Report Upload**: Sends crash dumps and error logs to a configured server endpoint
- **Command-line Interface**: Simple parameter-based configuration
- **Robust Error Handling**: Comprehensive logging and graceful failure recovery
- **Windows Native**: Optimized for Windows using WinINet API

## Build Requirements

- **Compiler**: MSVC with C++20 support (Visual Studio 2019 16.11+ or Visual Studio 2022)
- **CMake**: Version 3.20 or higher
- **Platform**: Windows (uses Windows API and WinINet)
- **Dependencies**: Windows SDK (wininet.lib)

## Building

```bash
# Configure the project
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build the executable
cmake --build build --config Release

# The executable will be in build/bin/L2CrashSender.exe
```

## Usage

The application is designed to be called automatically by crash reporting systems. It requires four command-line parameters:

```bash
L2CrashSender.exe -url=http://example.com/crashreport -version=1.0.0 -error=C:\temp\error.txt -dump=C:\temp\crashdump.dmp
```

### Parameters

| Parameter | Description | Required |
|-----------|-------------|----------|
| `-url=`   | Server endpoint URL for crash reports | Yes |
| `-version=` | Application version string | Yes |
| `-error=` | Path to error description file (UTF-16 format) | Yes |
| `-dump=`  | Path to crash dump file | Yes |

### Example

```bash
L2CrashSender.exe -url=https://crash-server.example.com/api/submit -version=2.1.3 -error=C:\temp\app_error.txt -dump=C:\temp\app_crash.dmp
```

## Architecture

The application follows a modular design with clear separation of concerns:

```
CrashSender/
├── main.cpp              # Application entry point
├── main.h                # Main application interface
├── crash_report_data.h   # Data structure definitions
├── crash_report_data.cpp # Data validation and management
├── command_line_parser.h # Command line argument processing
├── command_line_parser.cpp
├── http_client.h         # HTTP communication
├── http_client.cpp
├── logger.h              # Logging system
├── logger.cpp
├── utils.h               # Utility functions
├── utils.cpp
└── CMakeLists.txt        # Build configuration
```

### Key Components

#### CrashReportData
- Validates and stores crash report information
- Ensures data integrity before transmission

#### CommandLineParser  
- Parses command-line arguments safely
- Processes error file content and server URLs
- Provides detailed error messages for debugging

#### HttpClient
- Handles HTTP communication using WinINet
- Creates multipart form data for file uploads
- Manages connection lifecycle and error recovery

#### Logger
- Thread-safe singleton logging system
- Configurable log levels (Debug, Info, Error)
- Automatic timestamping and file output

#### Utils
- Text conversion utilities (Wide ↔ UTF-8)
- File system operations with RAII
- Time formatting functions

## HTTP Protocol

The application sends crash reports using HTTP POST with multipart form data:

### Request Format
```
POST /api/crashreport HTTP/1.1
Host: crash-server.example.com
Content-Type: multipart/form-data; boundary=MULTIPART-DATA-BOUNDARY
Content-Transfer-Encoding: binary

--MULTIPART-DATA-BOUNDARY
Content-Disposition: form-data; name="CRVersion"

1.0.0
--MULTIPART-DATA-BOUNDARY
Content-Disposition: form-data; name="error"

Application crash details...
--MULTIPART-DATA-BOUNDARY
Content-Disposition: form-data; name="dumpfile"; filename="crash.dmp"
Content-Type: application/octet-stream

[Binary crash dump data]
--MULTIPART-DATA-BOUNDARY--
```

### Response Handling
- **2xx**: Success - temporary files are cleaned up
- **4xx/5xx**: Error - detailed error message logged
- **Network errors**: Timeout/connection failures logged

## Logging

The application creates a log file `L2CrashSender.log` in the current directory with detailed execution information:

```
2024-01-15 14:30:25.123 [INF] L2CrashSender started
2024-01-15 14:30:25.124 [DBG] Version: 1.0.0
2024-01-15 14:30:25.125 [DBG] Connecting to server: crash-server.example.com
2024-01-15 14:30:25.200 [DBG] Total upload size: 1048576 bytes
2024-01-15 14:30:26.150 [DBG] Server responded with status: 200
2024-01-15 14:30:26.151 [INF] Crash report sent successfully
2024-01-15 14:30:26.152 [INF] L2CrashSender finished
```

## Error Handling

The application implements robust error handling:

- **Invalid arguments**: Clear error messages for missing/malformed parameters
- **File access errors**: Graceful handling of missing or locked files
- **Network failures**: Detailed HTTP error reporting with server responses  
- **Resource cleanup**: Automatic cleanup even on failure paths
- **Exception safety**: All operations are exception-safe with RAII

## Security Considerations

- **File validation**: Basic validation of input file paths and sizes
- **Memory safety**: Modern C++ with automatic memory management
- **Buffer overflow protection**: Bounds checking on all file operations

## Integration Example

For integration with crash reporting frameworks:

```cpp
// Example integration with a crash handler
void OnApplicationCrash(const std::wstring& dumpPath, const std::wstring& errorDesc) {
    // Write error description to temporary file
    std::wstring errorFile = WriteErrorToTempFile(errorDesc);
    
    // Launch crash sender
    std::wstring commandLine = L"L2CrashSender.exe "
        L"-url=https://crashes.myapp.com/submit "
        L"-version=" + GetApplicationVersion() + L" "
        L"-error=" + errorFile + L" "
        L"-dump=" + dumpPath;
        
    // Execute asynchronously
    CreateProcess(nullptr, commandLine.data(), /* ... */);
}
```
