#pragma once

// Forward declarations for main application entry point
namespace CrashSender {
    /**
     * @brief Main application logic for crash report sending
     * @param argc Number of command line arguments
     * @param argv Command line argument values
     * @return Exit code (0 for success, non-zero for failure)
     */
    int RunApplication(int argc, wchar_t* argv[]) noexcept;
}
