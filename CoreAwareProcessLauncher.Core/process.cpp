//process.cpp
#include "pch.h"
#include "process.h"
#include "utilities.h" 
#include <format>

using Utilities::ConvertToNarrowString;
using Utilities::ConvertToWideString;

bool ProcessManager::LaunchProcess(
    const std::wstring& path,
    const std::vector<std::wstring>& args,
    const std::wstring& workingDir,
    DWORD_PTR affinityMask) {
    
    g_logger->Log(ApplicationLogger::Level::INFO, 
        "Attempting to launch: " + ConvertToNarrowString(path));

    // Resolve full path
    WCHAR fullPath[MAX_PATH];
    if (!SearchPathW(NULL, path.c_str(), L".exe", MAX_PATH, fullPath, NULL)) {
        DWORD error = GetLastError();
        g_logger->Log(ApplicationLogger::Level::ERR,
            std::format("SearchPath failed for '{}' with error: {}",
                ConvertToNarrowString(path), error));
        LogWin32Error("SearchPath failed");
        return false;
    }
    
    g_logger->Log(ApplicationLogger::Level::INFO, 
        "Resolved path: " + ConvertToNarrowString(fullPath));
    
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi;
    
    g_logger->Log(ApplicationLogger::Level::INFO, 
        std::string("Launching process with affinity mask: 0x") + 
        std::format("{:X}", affinityMask));
    
    std::wstring cmdLine = BuildCommandLine(fullPath, args);
    g_logger->Log(ApplicationLogger::Level::DEBUG, 
        std::string("Command line: ") + ConvertToNarrowString(cmdLine));    

    // Create process suspended
    if (!CreateProcessW(
        NULL,                // Application name (NULL when using command line)
        cmdLine.data(),      // Command line
        NULL,               // Process attributes
        NULL,               // Thread attributes
        TRUE,              // Inherit handles
        CREATE_SUSPENDED,   // Creation flags
        NULL,               // Environment
        workingDir.empty() ? NULL : workingDir.c_str(), // Working directory
        &si,                // Startup info
        &pi                 // Process information
    )) {
        LogWin32Error("CreateProcess failed");
        return false;
    }
    
    // Set affinity
    if (!SetProcessAffinityMask(pi.hProcess, affinityMask)) {
        LogWin32Error("SetProcessAffinityMask failed");
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return false;
    }
    
    // Resume the process
    if (ResumeThread(pi.hThread) == -1) {        // NEW: Error check for ResumeThread
        LogWin32Error("ResumeThread failed");
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return false;
    }
    
    // After process is launched and running, wait for it to complete
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Force console refresh
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);

    // Clean up handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    g_logger->Log(ApplicationLogger::Level::INFO, "Process launched successfully");
    return true;
}

std::wstring ProcessManager::BuildCommandLine(
    const std::wstring& path,
    const std::vector<std::wstring>& args) {
    
    std::wstring cmdLine;
    
    // Quote the program path
    cmdLine = L"\"" + path + L"\"";
    
    // Add arguments
    for (const auto& arg : args) {
        cmdLine += L" ";
        // Quote arguments containing spaces
        if (arg.find(L' ') != std::wstring::npos) {
            cmdLine += L"\"" + arg + L"\"";
        } else {
            cmdLine += arg;
        }
    }
    
    return cmdLine;
}

void ProcessManager::LogWin32Error(const std::string& context) {
    DWORD error = GetLastError();
    LPVOID msgBuf;
    
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msgBuf,
        0, NULL);
    
    std::string errorMsg = context + ": " + (char*)msgBuf;
    LocalFree(msgBuf);
    
    g_logger->Log(ApplicationLogger::Level::ERR, errorMsg);
}
