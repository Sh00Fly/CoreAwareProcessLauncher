#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <intrin.h>

DWORD_PTR GetPCoreMask() {
    int cpuInfo[4] = {0};
    DWORD_PTR mask = 0;
    
    for (int i = 0; i < 32; i++) {
        DWORD_PTR threadMask = 1ULL << i;
        SetThreadAffinityMask(GetCurrentThread(), threadMask);
        Sleep(0);
        
        __cpuidex(cpuInfo, 0x1A, 0);
        if (((cpuInfo[0] >> 24) & 0xFF) == 0x40) {
            mask |= threadMask;
        }
    }
    
    return mask;
}

std::wstring GetExecutablePath() {
    // Get current executable path
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    
    // Create INI path by replacing .exe with .ini
    wchar_t iniPath[MAX_PATH];
    wcscpy_s(iniPath, exePath);
    wchar_t* ext = wcsrchr(iniPath, L'.');
    if (ext) {
        wcscpy_s(ext, 5, L".ini");
    }
    
    // Read the target path from INI
    wchar_t targetPath[MAX_PATH] = L"";
    if (GetPrivateProfileStringW(L"Settings", L"TargetPath", L"", targetPath, MAX_PATH, iniPath) == 0) {
        throw std::runtime_error("Target path not found in INI file");
    }
    
    return std::wstring(targetPath);
}

int main() {
    try {
        // Get P-core mask
        DWORD_PTR pCoreMask = GetPCoreMask();
        
        // Get target executable path
        std::wstring targetPath = GetExecutablePath();
        
        // Prepare process creation
        STARTUPINFOW si = { sizeof(STARTUPINFOW) };
        PROCESS_INFORMATION pi;
        
        // Create process suspended
        if (!CreateProcessW(
            targetPath.c_str(),    // Application name
            NULL,                  // Command line
            NULL,                  // Process attributes
            NULL,                  // Thread attributes
            FALSE,                 // Inherit handles
            CREATE_SUSPENDED,      // Creation flags
            NULL,                  // Environment
            NULL,                  // Current directory
            &si,                   // Startup info
            &pi                    // Process information
        )) {
            throw std::runtime_error("Failed to create process");
        }
        
        // Set affinity
        if (!SetProcessAffinityMask(pi.hProcess, pCoreMask)) {
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            throw std::runtime_error("Failed to set process affinity");
        }
        
        // Resume the process
        ResumeThread(pi.hThread);
        
        // Clean up handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
