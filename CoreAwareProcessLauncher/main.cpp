//main.cpp
#include "options.h"
#include "logger.h"
#include "cpu.h"
#include "process.h"
#include <iostream>
#include <format>

int wmain(int argc, wchar_t* argv[]) {
    try {
        g_logger = std::make_unique<ApplicationLogger>(true, L"capl.log");
        g_logger->Log(ApplicationLogger::Level::INFO, "CAPL starting...");

        // Get command line directly from Windows
        int cmdArgc;
        LPWSTR* cmdArgv = CommandLineToArgvW(GetCommandLineW(), &cmdArgc);
        
        if (cmdArgv == nullptr) {
            throw std::runtime_error("Failed to parse command line");
        }

        CommandLineOptions options = ParseCommandLine(cmdArgc, cmdArgv);
        
        // Free the command line arguments
        LocalFree(cmdArgv);        

        if (options.showHelp) {
            ShowHelp();
            return 0;
        }

        if (options.queryMode) {
            g_logger->Log(ApplicationLogger::Level::INFO, "Running in query mode");
            std::wcout << CpuInfo::QuerySystemInfo();
            return 0;
        }

        // Get appropriate core mask based on selection method
        DWORD_PTR coreMask = 0;
        if (options.selectionMethod == CommandLineOptions::SelectionMethod::DETECT) {
            if (options.detectMode == "P") {
                coreMask = CpuInfo::GetPCoreMask();
            }
            else if (options.detectMode == "E") {
                coreMask = CpuInfo::GetECoreMask();
            }
            else if (options.detectMode == "CUSTOM") {
                coreMask = CpuInfo::GetCoreMask(options.cpuidValue);
            }
        }
        else if (options.selectionMethod == CommandLineOptions::SelectionMethod::DIRECT) {
            coreMask = CpuInfo::CoreListToMask(options.cores);
        }

        // Apply inversion if requested
        if (options.invertSelection) {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            DWORD_PTR fullMask = (1ULL << sysInfo.dwNumberOfProcessors) - 1;
            coreMask = fullMask & ~coreMask;
            g_logger->Log(ApplicationLogger::Level::INFO, 
                "Inverted core mask: 0x" + std::format("{:X}", coreMask));
        }

        // Validate final mask
        if (coreMask == 0) {
            throw std::runtime_error("Resulting core mask is empty");
        }

        // Launch the process
        if (!ProcessManager::LaunchProcess(options.targetPath, coreMask)) {
            throw std::runtime_error("Failed to launch process");
        }
        
    }
    catch (const std::exception& e) {
        if (g_logger) {
            g_logger->Log(ApplicationLogger::Level::ERR, 
                "Fatal error: " + std::string(e.what()));
        }
        std::cerr << "Error: " << e.what() << std::endl;
        std::wcout << L"\nUse --help for usage information." << std::endl;
        return 1;
    }
    
    return 0;
}

//#include <windows.h>
//#include <iostream>
//#include <string>
//#include <vector>
//#include <intrin.h>
//
//DWORD_PTR GetPCoreMask() {
//    int cpuInfo[4] = {0};
//    DWORD_PTR mask = 0;
//    
//    for (int i = 0; i < 32; i++) {
//        DWORD_PTR threadMask = 1ULL << i;
//        SetThreadAffinityMask(GetCurrentThread(), threadMask);
//        Sleep(0);
//        
//        __cpuidex(cpuInfo, 0x1A, 0);
//        if (((cpuInfo[0] >> 24) & 0xFF) == 0x40) {
//            mask |= threadMask;
//        }
//    }
//    
//    return mask;
//}
//
//std::wstring GetExecutablePath() {
//    // Get current executable path
//    wchar_t exePath[MAX_PATH];
//    GetModuleFileNameW(NULL, exePath, MAX_PATH);
//    
//    // Create INI path by replacing .exe with .ini
//    wchar_t iniPath[MAX_PATH];
//    wcscpy_s(iniPath, exePath);
//    wchar_t* ext = wcsrchr(iniPath, L'.');
//    if (ext) {
//        wcscpy_s(ext, 5, L".ini");
//    }
//    
//    // Read the target path from INI
//    wchar_t targetPath[MAX_PATH] = L"";
//    if (GetPrivateProfileStringW(L"Settings", L"TargetPath", L"", targetPath, MAX_PATH, iniPath) == 0) {
//        throw std::runtime_error("Target path not found in INI file");
//    }
//    
//    return std::wstring(targetPath);
//}
//
//int main() {
//    try {
//        // Get P-core mask
//        DWORD_PTR pCoreMask = GetPCoreMask();
//        
//        // Get target executable path
//        std::wstring targetPath = GetExecutablePath();
//        
//        // Prepare process creation
//        STARTUPINFOW si = { sizeof(STARTUPINFOW) };
//        PROCESS_INFORMATION pi;
//        
//        // Create process suspended
//        if (!CreateProcessW(
//            targetPath.c_str(),    // Application name
//            NULL,                  // Command line
//            NULL,                  // Process attributes
//            NULL,                  // Thread attributes
//            FALSE,                 // Inherit handles
//            CREATE_SUSPENDED,      // Creation flags
//            NULL,                  // Environment
//            NULL,                  // Current directory
//            &si,                   // Startup info
//            &pi                    // Process information
//        )) {
//            throw std::runtime_error("Failed to create process");
//        }
//        
//        // Set affinity
//        if (!SetProcessAffinityMask(pi.hProcess, pCoreMask)) {
//            TerminateProcess(pi.hProcess, 1);
//            CloseHandle(pi.hProcess);
//            CloseHandle(pi.hThread);
//            throw std::runtime_error("Failed to set process affinity");
//        }
//        
//        // Resume the process
//        ResumeThread(pi.hThread);
//        
//        // Clean up handles
//        CloseHandle(pi.hProcess);
//        CloseHandle(pi.hThread);
//        
//    } catch (const std::exception& e) {
//        std::cerr << "Error: " << e.what() << std::endl;
//        return 1;
//    }
//    
//    return 0;
//}
