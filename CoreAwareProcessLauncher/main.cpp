//main.cpp
#include "options.h"
#include "utilities.h"
#include "cpu.h"
#include "process.h"
#include <iostream>
#include <format>


int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd)
{
	// Just attach to parent console if we're launched from command prompt
	AttachConsole(ATTACH_PARENT_PROCESS);
	// Give the console time to stabilize
	Sleep(50);  // 50ms should be enough
	// Set up console output
	FILE* dummy;
	freopen_s(&dummy, "CONOUT$", "w", stdout);
	freopen_s(&dummy, "CONOUT$", "w", stderr);

	printf("\n");
	try {
		g_logger = std::make_unique<ApplicationLogger>(false);

		// Get command line directly from Windows
		int cmdArgc;
		LPWSTR* cmdArgv = CommandLineToArgvW(GetCommandLineW(), &cmdArgc);

		if (cmdArgv == nullptr) {
			throw std::runtime_error(Utilities::ConvertToNarrowString(L"Failed to parse command line"));
		}

		CommandLineOptions options = ParseCommandLine(cmdArgc, cmdArgv);

		// Free the command line arguments
		LocalFree(cmdArgv);

		if (options.enableLogging) {
			g_logger = std::make_unique<ApplicationLogger>(true,
				options.logPath.empty() ? L"capl.log" : options.logPath);
			g_logger->Log(ApplicationLogger::Level::INFO, "CAPL starting...");
		}

		if (options.showHelp) {
			//if (hasConsole) {
			ShowHelp();
			//}
			//else {
			//	MessageBoxW(NULL, L"Run capl.exe from command prompt to see help",
			//		L"CAPL Help", MB_OK | MB_ICONINFORMATION);
			//}
			return 0;
		}

		if (options.queryMode) {
			g_logger->Log(ApplicationLogger::Level::INFO, "Running in query mode");
			std::wcout << CpuInfo::QuerySystemInfo() << std::flush;
			return 0;
		}

		// Get appropriate core mask based on affinity mode
		DWORD_PTR coreMask = 0;
		auto caps = CpuInfo::GetCapabilities();

		switch (options.affinityMode) {
		case CommandLineOptions::CoreAffinityMode::P_CORES_ONLY:
			if (!caps.isHybrid || !caps.supportsLeaf1A) {
				throw std::runtime_error(Utilities::ConvertToNarrowString(L"This CPU does not support hybrid architecture"));
			}
			coreMask = CpuInfo::GetPCoreMask();
			break;

		case CommandLineOptions::CoreAffinityMode::E_CORES_ONLY:
			if (!caps.isHybrid || !caps.supportsLeaf1A) {
				throw std::runtime_error(Utilities::ConvertToNarrowString(L"This CPU does not support hybrid architecture"));
			}
			coreMask = CpuInfo::GetECoreMask();
			break;

		case CommandLineOptions::CoreAffinityMode::LP_CORES_ONLY:
			if (!caps.isHybrid || !caps.supportsLeaf1A) {
				throw std::runtime_error(Utilities::ConvertToNarrowString(L"This CPU does not support hybrid architecture"));
			}
			coreMask = CpuInfo::GetLpECoreMask();
			break;

		case CommandLineOptions::CoreAffinityMode::ALL_E_CORES:
			if (!caps.isHybrid || !caps.supportsLeaf1A) {
				throw std::runtime_error(Utilities::ConvertToNarrowString(L"This CPU does not support hybrid architecture"));
			}
			coreMask = CpuInfo::GetECoreMask() | CpuInfo::GetLpECoreMask();
			break;

		case CommandLineOptions::CoreAffinityMode::ALL_CORES:
		{
			SYSTEM_INFO sysInfo;
			GetSystemInfo(&sysInfo);
			coreMask = (1ULL << sysInfo.dwNumberOfProcessors) - 1;
		}
		break;

		case CommandLineOptions::CoreAffinityMode::CUSTOM:
			coreMask = CpuInfo::CoreListToMask(options.cores);
			break;

		default:
			throw std::runtime_error(Utilities::ConvertToNarrowString(L"Invalid affinity mode"));
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
			throw std::runtime_error(Utilities::ConvertToNarrowString(L"Resulting core mask is empty"));
		}

		// Launch the process
		if (!ProcessManager::LaunchProcess(
			options.targetPath,
			options.targetArgs,
			options.targetWorkingDir,
			coreMask)) {
			throw std::runtime_error(Utilities::ConvertToNarrowString(L"Failed to launch process"));
		}
	}
	catch (const std::exception& e) {
		if (g_logger) {
			g_logger->Log(ApplicationLogger::Level::ERR,
				"Fatal error: " + std::string(e.what()));
		}
		//if (hasConsole) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::wcout << L"\nUse --help for usage information.\n\n" << std::endl;
		//}
		//else {
		//	MessageBoxA(NULL, e.what(), "CAPL Error", MB_OK | MB_ICONERROR);
		//}
      // Force shell prompt redraw
        printf("\n\r");
        fflush(stdout);
        FreeConsole();
		return 1;
	}
      // Force shell prompt redraw
        printf("\n\r");
        fflush(stdout);
		Sleep(50);
        FreeConsole();
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
