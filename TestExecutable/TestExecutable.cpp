// TestExecutable.cpp
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <csignal>

// Global control flag for threads
std::atomic<bool> g_running = true;

// Ctrl+C Signal handler
void SignalHandler(int signal) {
    if (signal == SIGINT) {
        std::wcout << L"\nCtrl+C received. Stopping...\n";
        g_running = false;
    }
}
void ShowArgs(int argc, wchar_t* argv[]) {
    std::wcout << L"Working Directory: " << _wgetcwd(nullptr, 0) << L"\n";
    std::wcout << L"Command line arguments (" << argc << L"):\n";
    for (int i = 0; i < argc; i++) {
        std::wcout << i << L": " << argv[i] << L"\n";
    }
}

void CpuLoadThread(int threadId, bool showProgress) {
    // Simple CPU load with yield
    while (g_running) {
        // Spin for a while
        for (volatile int i = 0; i < 1000000 && g_running; i++) {}
        // Yield to prevent system lockup
        Sleep(1);

        if (showProgress && threadId == 0) { // Only first thread reports
            std::wcout << L"." << std::flush; // Progress indicator
        }
    }
}

int wmain(int argc, wchar_t* argv[]) {
    // Register signal handler
    signal(SIGINT, SignalHandler);

    // Show brief help if no arguments
    if (argc == 1) {
        std::wcout << L"TestExecutable - CPU Load Testing Tool\n"
            << L"Usage: TestExecutable.exe [options] [additional args]\n"
            << L"Options:\n"
            << L"  --time <seconds>     Run duration (default: 30)\n"
            << L"  --threads <count>    Number of threads (default: all)\n"
            << L"  --show-args          Show command line arguments\n"
            << L"  --help               Show detailed help\n"
            << L"\nNote: Press Ctrl+C to stop before timeout\n"
            << L"\nExample: TestExecutable.exe --time 10 --threads 4\n";
       // return 0;
    }
    // Parse command line
    int duration = 30; // Default 30 seconds
    int threadCount = 0; // Default: use all available threads
    bool showArgs = false;
    bool showProgress = true;

    for (int i = 1; i < argc; i++) {
        std::wstring arg = argv[i];
        if (arg == L"--no-progress") {
            showProgress = false;
        }
        if (arg == L"--time" && i + 1 < argc) {
            duration = _wtoi(argv[++i]);
        }
        else if (arg == L"--threads" && i + 1 < argc) {
            threadCount = _wtoi(argv[++i]);
        }
        else if (arg == L"--show-args") {
            showArgs = true;
        }
        else if (arg == L"--help") {
            std::wcout << L"TestExecutable - CPU Load Testing Tool\n"
                << L"\nUsage: TestExecutable.exe [options] [additional args]\n"
                << L"\nOptions:\n"
                << L"  --time <seconds>     Run duration (default: 30)\n"
                << L"  --threads <count>    Number of threads (default: all)\n"
                << L"  --no-progress        Disable progress bar\n"
                << L"  --show-args          Show command line arguments\n"
                << L"  --help               Show this detailed help\n"
                << L"\nOperation:\n"
                << L"  - Creates specified number of CPU-loading threads\n"
                << L"  - Each thread performs computation with periodic yields\n"
                << L"  - Progress shown with dots (.)\n"
                << L"  - Automatically stops after specified duration\n"
                << L"\nExamples:\n"
                << L"  TestExecutable.exe --time 10 --threads 4\n"
                << L"  TestExecutable.exe --show-args arg1 arg2\n"
                << L"  TestExecutable.exe --threads 8 --time 60\n";
            return 0;
        }
    }

    if (showArgs) {
        ShowArgs(argc, argv);
    }

    // Determine thread count if not specified
    if (threadCount <= 0) {
        threadCount = std::thread::hardware_concurrency();
    }

    std::wcout << L"Starting " << threadCount << L" threads for "
        << duration << L" seconds...\n";

    // Create threads
    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back(CpuLoadThread, i,showProgress);
    }

    // Wait for specified duration
    auto startTime = std::chrono::steady_clock::now();
    while (g_running) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>
            (currentTime - startTime).count();
        
        if (elapsedSeconds >= duration) {
            g_running = false;
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop threads
    g_running = false;
    std::wcout << L"\nStopping threads...\n";

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    std::wcout << L"Test complete.\n";
    return 0;
}
