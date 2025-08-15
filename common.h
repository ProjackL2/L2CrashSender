#include <string>
#include <string_view>
#include <windows.h>

[[nodiscard]]
inline std::string WideToUtf8(std::wstring_view wstr) noexcept {
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