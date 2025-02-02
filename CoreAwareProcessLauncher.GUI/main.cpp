#include "pch.h"
#include "gui_message_handler.h"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    try {
        g_messageHandler = std::make_unique<GuiMessageHandler>();
        g_logger = std::make_unique<ApplicationLogger>(false);

        int argc;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv == nullptr) {
            throw std::runtime_error(Utilities::ConvertToNarrowString(
                L"Failed to parse command line"));
        }

        CommandLineOptions options = ParseCommandLine(argc, argv);
        LocalFree(argv);

        if (options.enableLogging) {
            g_logger = std::make_unique<ApplicationLogger>(true,
                options.logPath.empty() ? L"capl.log" : options.logPath);
            g_logger->Log(ApplicationLogger::Level::INFO, "CAPL GUI starting...");
        }

        if (options.showHelp) {
            ShowHelp();
            return 0;
        }

        if (options.queryMode) {
            g_logger->Log(ApplicationLogger::Level::INFO, "Running in query mode");
            g_messageHandler->ShowQueryResult(CpuInfo::QuerySystemInfo());
            return 0;
        }

        // Get appropriate core mask based on affinity mode
        DWORD_PTR coreMask = 0;
        auto caps = CpuInfo::GetCapabilities();

        switch (options.affinityMode) {
        case CommandLineOptions::CoreAffinityMode::P_CORES_ONLY:
            if (!caps.isHybrid || !caps.supportsLeaf1A) {
                throw std::runtime_error(Utilities::ConvertToNarrowString(
                    L"This CPU does not support hybrid architecture"));
            }
            coreMask = CpuInfo::GetPCoreMask();
            break;

        case CommandLineOptions::CoreAffinityMode::E_CORES_ONLY:
            if (!caps.isHybrid || !caps.supportsLeaf1A) {
                throw std::runtime_error(Utilities::ConvertToNarrowString(
                    L"This CPU does not support hybrid architecture"));
            }
            coreMask = CpuInfo::GetECoreMask();
            break;

        case CommandLineOptions::CoreAffinityMode::LP_CORES_ONLY:
            if (!caps.isHybrid || !caps.supportsLeaf1A) {
                throw std::runtime_error(Utilities::ConvertToNarrowString(
                    L"This CPU does not support hybrid architecture"));
            }
            coreMask = CpuInfo::GetLpECoreMask();
            break;

        case CommandLineOptions::CoreAffinityMode::ALL_E_CORES:
            if (!caps.isHybrid || !caps.supportsLeaf1A) {
                throw std::runtime_error(Utilities::ConvertToNarrowString(
                    L"This CPU does not support hybrid architecture"));
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

        case CommandLineOptions::CoreAffinityMode::PATTERN:
            if (!caps.supportsLeaf1A) {
                throw std::runtime_error(Utilities::ConvertToNarrowString(
                    L"This CPU does not support core type detection"));
            }
            coreMask = CpuInfo::GetCoreMask(options.pattern);
            break;

        default:
            throw std::runtime_error(Utilities::ConvertToNarrowString(
                L"Invalid affinity mode"));
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
            throw std::runtime_error(Utilities::ConvertToNarrowString(
                L"Resulting core mask is empty"));
        }

        // Launch the process
        if (!ProcessManager::LaunchProcess(
            options.targetPath,
            options.targetArgs,
            options.targetWorkingDir,
            coreMask)) {
            throw std::runtime_error(Utilities::ConvertToNarrowString(
                L"Failed to launch process"));
        }
    }
    catch (const std::exception& e) {
        if (g_logger) {
            g_logger->Log(ApplicationLogger::Level::ERR,
                "Fatal error: " + std::string(e.what()));
        }
        if (g_messageHandler) {
            g_messageHandler->ShowError(Utilities::ConvertToWideString(e.what()));
            g_messageHandler->ShowInfo(L"Use --help for usage information.");
        }
        return 1;
    }

    return 0;
}