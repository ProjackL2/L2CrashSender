#include "crash_report_data.h"

namespace CrashSender {

void CrashReportData::Clear() noexcept {
    url.clear();
    version.clear(); 
    error.clear();
    dump_path.clear();
    temp_path.clear();
    full_url.clear();
    server_path.clear();
}

bool CrashReportData::IsValid() const noexcept {
    return !url.empty() &&  !version.empty() && !dump_path.empty() && !temp_path.empty();
}

} // namespace CrashSender

