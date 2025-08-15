
#include <fstream>
#include <string_view>

class Logger {
public:
    enum class LogLevel : int {
        Debug, Info, Error
    };

    Logger();
    ~Logger();

    static void Debug(std::string_view message) noexcept;
    static void Debug(std::wstring_view message) noexcept;
    static void Info(std::wstring_view message) noexcept;
    static void Info(std::string_view message) noexcept;
    static void Error(std::string_view message) noexcept;
    static void Error(std::wstring_view message) noexcept;
private:
    inline static std::ofstream logFile;
    constexpr static std::string_view LogLevelToString(LogLevel level) noexcept {
        switch (level) {
        case LogLevel::Debug: return "DBG";
        case LogLevel::Info:  return "INF";
        case LogLevel::Error: return "ERR";
        default:              return "UNK";
        }
    }

    static void Log(LogLevel level, std::string_view message) noexcept;
    static void Log(LogLevel level, std::wstring_view message) noexcept;
};
