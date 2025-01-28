#include "process.h"
#include "logger.h"
#include <format>

bool ProcessManager::LaunchProcess(
    const std::wstring& path, 
    DWORD_PTR affinityMask) {
    
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi;
    
    g_logger->Log(Logger::Level::INFO, 
        "Launching process with affinity mask: 0x" + 
        std::format("{:X}", affinityMask));
    
    // Create process suspended
    if (!CreateProcessW(
        path.c_str(),    // Application name
        NULL,            // Command line
        NULL,            // Process attributes
        NULL,            // Thread attributes
        FALSE,           // Inherit handles
        CREATE_SUSPENDED,// Creation flags
        NULL,            // Environment
        NULL,            // Current directory
        &si,             // Startup info
        &pi              // Process information
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
    ResumeThread(pi.hThread);
    
    // Clean up handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    g_logger->Log(Logger::Level::INFO, "Process launched successfully");
    return true;
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
    
    g_logger->Log(Logger::Level::ERR, errorMsg);
}