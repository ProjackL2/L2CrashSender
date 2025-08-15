
// Logging system globals
static const std::wstring LOG_FILENAME = L"L2CrashSender.log";

static std::ofstream g_logFile;

enum class LogLevel
{
    Debug, Info, Warn, Error
};

// Get current timestamp as string
std::string GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Initialize logging system
bool InitializeLogging() {
    // Convert wide string filename to narrow string for ofstream
    int len = WideCharToMultiByte(CP_UTF8, 0, LOG_FILENAME.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return false;
    
    std::string filename(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, LOG_FILENAME.c_str(), -1, &filename[0], len, nullptr, nullptr);
    
    g_logFile.open(filename, std::ios::app);
    if (!g_logFile.is_open()) {
        return false;
    }
    
    g_logFile << "\n" << GetCurrentTimestamp() << " [INFO] L2CrashSender started" << std::endl;
    return true;
}

// Close logging system
void CloseLogging() {
    if (g_logFile.is_open()) {
        g_logFile << GetCurrentTimestamp() << " [INFO] L2CrashSender finished" << std::endl;
        g_logFile.close();
    }
}

const char* LogLevelToString(LogLevel level) {
    switch (level)
    {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warn: return "WARN";
    case LogLevel::Error: return "ERROR";
    default: return "UNKNOWN";
    }
}

// Write log message (wide string version)
void Log(LogLevel level, const std::wstring& message) {
    if (!g_logFile.is_open()) {
        return;
    }
    
    // Convert wide strings to UTF-8
    auto convertToUtf8 = [](const std::wstring& wstr) -> std::string {
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return "";
        std::string str(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], len, nullptr, nullptr);
        return str;
    };
    
    std::string messageStr = convertToUtf8(message);
    
    g_logFile << GetCurrentTimestamp() << " [" << LogLevelToString(level) << "] " << messageStr << std::endl;
    g_logFile.flush();
}

// Write log message (narrow string version)
void LogA(LogLevel level, const std::string& message) {
    if (g_logFile.is_open()) {
        return;
    }
    
    g_logFile << GetCurrentTimestamp() << " [" << LogLevelToString(level) << "] " << message << std::endl;
    g_logFile.flush();
}
