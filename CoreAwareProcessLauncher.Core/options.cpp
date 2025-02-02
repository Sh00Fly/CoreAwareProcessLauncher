//options.cpp
#include "pch.h"
#include "options.h"
#include "logger.h"
#include "utilities.h" 
#include <sstream>
#include <format>
#include <iostream>


CommandLineOptions::CommandLineOptions() :
    pattern(0),
    invertSelection(false),
    queryMode(false),
    enableLogging(false),
    showHelp(false) {
}

CommandLineOptions ParseCommandLine(int argc, wchar_t* argv[]) {
    CommandLineOptions options;
    bool foundDelimiter = false;

    g_logger->Log(ApplicationLogger::Level::INFO, 
        "Starting command line parsing with " + std::to_string(argc) + " arguments");

    if (argc == 1) {
        options.showHelp = true;
        return options;
    }

    // Print the raw command line
    g_logger->Log(ApplicationLogger::Level::DEBUG, "Raw command line: " + WideToUtf8(GetCommandLineW()));
    for (int i = 0; i < argc; i++) {
        std::wstring arg = argv[i];
        g_logger->Log(ApplicationLogger::Level::DEBUG,
            "Arg[" + std::to_string(i) + "] length: " +
            std::to_string(arg.length()));
        g_logger->Log(ApplicationLogger::Level::DEBUG,
            "Arg[" + std::to_string(i) + "] raw: " + WideToUtf8(arg));
    }

    for (int i = 1; i < argc; i++) {
        std::wstring arg = argv[i];

        g_logger->Log(ApplicationLogger::Level::DEBUG,
            "Processing argument: '" + WideToUtf8(arg) + "'");

        if (arg == L"--") {
            foundDelimiter = true;
            // Collect everything after -- as target path and args
            if (i + 1 < argc) {
                options.targetPath = argv[i + 1];
                for (int j = i + 2; j < argc; j++) {
                    options.targetArgs.push_back(argv[j]);
                }
            }
            break;  // Stop processing arguments after --
        }

        if (arg == L"--help" || arg == L"-h" || arg == L"-?" || arg == L"/?") {
            g_logger->Log(ApplicationLogger::Level::INFO, "Help option detected");
            options.showHelp = true;
            return options;
        }
        else if (arg == L"--query" || arg == L"-q") {
            options.queryMode = true;
        }
        else if ((arg == L"--dir" || arg == L"-d") && i + 1 < argc) {
            options.targetWorkingDir = argv[++i];
        }
        else if ((arg == L"--mode" || arg == L"-m") && i + 1 < argc) {
            std::wstring mode = argv[++i];
            if (mode == L"p") {
                options.affinityMode = CommandLineOptions::CoreAffinityMode::P_CORES_ONLY;
            }
            else if (mode == L"e") {
                options.affinityMode = CommandLineOptions::CoreAffinityMode::E_CORES_ONLY;
            }
            else if (mode == L"lp") {
                options.affinityMode = CommandLineOptions::CoreAffinityMode::LP_CORES_ONLY;
            }
            else if (mode == L"alle") {
                options.affinityMode = CommandLineOptions::CoreAffinityMode::ALL_E_CORES;
            }
            else if (mode == L"all") {
                options.affinityMode = CommandLineOptions::CoreAffinityMode::ALL_CORES;
            }
            else {
                throw std::runtime_error("Invalid mode. Use: p, e, lp, alle, all");
            }
        }
        else if ((arg == L"--pattern") && i + 1 < argc) {
            options.affinityMode = CommandLineOptions::CoreAffinityMode::PATTERN;
            std::wstring patternStr = argv[++i];
            try {
                options.pattern = std::stoul(patternStr, nullptr, 16);
                if (options.pattern > 0xFF) {
                    throw std::runtime_error("Pattern must be between 0x00 and 0xFF");
                }
            }
            catch (...) {
                throw std::runtime_error("Invalid pattern format. Use hex value (e.g., 0x40)");
            }
        }
        else if (arg == L"--cores" && i + 1 < argc) {
            options.affinityMode = CommandLineOptions::CoreAffinityMode::CUSTOM;
            std::wstring coreList = argv[++i];
            std::wstringstream ss(coreList);
            std::wstring coreNum;

            while (std::getline(ss, coreNum, L',')) {
                try {
                    options.cores.push_back(std::stoi(coreNum));
                }
                catch (...) {
                    throw std::runtime_error("Invalid core number in list");
                }
            }

            // Validate core numbers
            for (int core : options.cores) {
                if (core < 0) {
                    throw std::runtime_error("Core numbers must be non-negative");
                }
            }
        }
        else if (arg == L"--invert" || arg == L"-i") {
            options.invertSelection = true;
        }
        else if (arg == L"--log" || arg == L"-l") {
            options.enableLogging = true;
        }
        else if (arg == L"--logpath" && i + 1 < argc) {
            options.logPath = argv[++i];
        }
        else {
            throw std::runtime_error("Unknown option: " + WideToUtf8(arg));
        }
    }

    // Comprehensive validation
    try {
        bool isQueryOrHelp = options.queryMode || options.showHelp;

        // Basic requirements
        if (!isQueryOrHelp && !foundDelimiter) {
            throw std::runtime_error("Program command line must be specified after --");
        }
        // Target validation message
        if (!isQueryOrHelp && options.targetPath.empty()) {
            throw std::runtime_error("Target program path must be specified after --");
        }

        // Working directory validation
        if (!options.targetWorkingDir.empty()) {
            DWORD attrs = GetFileAttributesW(options.targetWorkingDir.c_str());
            if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                throw std::runtime_error("Invalid working directory: " +
                    WideToUtf8(options.targetWorkingDir));
            }
            g_logger->Log(ApplicationLogger::Level::INFO,
                "Working directory validated: " + WideToUtf8(options.targetWorkingDir));
        }
        // Target path existence
        if (!isQueryOrHelp) {
            if (GetFileAttributesW(options.targetPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                throw std::runtime_error("Target executable not found: " +
                    WideToUtf8(options.targetPath));
            }
            g_logger->Log(ApplicationLogger::Level::INFO, "Target executable found: " +
                WideToUtf8(options.targetPath));
        }

        // System limits for cores
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        g_logger->Log(ApplicationLogger::Level::INFO, "System has " +
            std::to_string(sysInfo.dwNumberOfProcessors) + " processors");

        if (options.affinityMode == CommandLineOptions::CoreAffinityMode::CUSTOM) {
            // Check for duplicate cores
            std::set<int> uniqueCores(options.cores.begin(), options.cores.end());
            if (uniqueCores.size() != options.cores.size()) {
                throw std::runtime_error("Duplicate core numbers in --cores list");
            }

            // Validate core numbers
            for (int core : options.cores) {
                if (core >= static_cast<int>(sysInfo.dwNumberOfProcessors)) {
                    throw std::runtime_error("Core number " + std::to_string(core) +
                        " exceeds system limit of " +
                        std::to_string(sysInfo.dwNumberOfProcessors - 1));
                }
            }

            std::string coreList;
            for (int core : options.cores) {
                if (!coreList.empty()) coreList += ",";
                coreList += std::to_string(core);
            }
            g_logger->Log(ApplicationLogger::Level::INFO, "Valid core list specified: " + coreList);
        }

        // Pattern validation and warning
        if (options.affinityMode == CommandLineOptions::CoreAffinityMode::PATTERN) {
            g_logger->Log(ApplicationLogger::Level::WARNING, 
                "Using custom pattern detection (0x" + 
                std::format("{:02X}", options.pattern) + 
                "). This is an experimental feature.");
        }

        // Log path validation
        if (options.enableLogging && options.logPath.empty()) {
            throw std::runtime_error("Log path must be specified when logging is enabled");
        }

        // Mutually exclusive options
        if (options.queryMode && !options.targetPath.empty()) {
            throw std::runtime_error("--query cannot be used with --target");
        }

        g_logger->Log(ApplicationLogger::Level::INFO, "Command line validation completed successfully");
    }
    catch (const std::exception& e) {
        g_logger->Log(ApplicationLogger::Level::ERR, "Validation failed: " + std::string(e.what()));
        throw; // Re-throw for main() to handle
    }

    return options;
}

void ShowHelp() {
    std::wcout << L"\nCore Aware Process Launcher (CAPL)\n"
        // MODIFIED: Updated usage line to show new syntax
        << L"Usage: capl.exe [options] -- <program> [program arguments]\n\n"
        << L"Core Affinity Modes:\n"
        << L"  --mode, -m <mode>      Predefined modes:\n"
        << L"                         p     - P-cores only (0x40)\n"
        << L"                         e     - E-cores only (0x20)\n"
        << L"                         lp    - LP E-cores only (0x30)\n"
        << L"                         alle  - All E-cores (E + LP)\n"
        << L"                         all   - Lock to all cores\n"
        << L"  --cores <list>         Custom core selection (comma-separated)\n"
        << L"  --pattern <hex>        Custom CPUID pattern detection (e.g., 0x40)\n"
        << L"                         Advanced: Use with caution\n"
        << L"  --invert, -i           Invert core selection\n\n"
        << L"Process Control:\n"
        << L"  --dir, -d <path>       Working directory for target process\n"
        << L"  -- <program> [args]     Program to launch with its arguments\n\n"
        << L"Utility Options:\n"
        << L"  --query, -q            Show system information only\n"
        << L"  --log, -l              Enable logging (disabled by default)\n"
        << L"  --logpath <path>       Specify log file path (default: capl.log)\n"
        << L"  --help, -h, -?, /?     Show this help\n\n"
        << L"Examples:\n"
        << L"  capl.exe --mode p -- notepad.exe \"file.txt\"\n"
        << L"  capl.exe --mode lp --dir \"C:\\Work\" -- program.exe -arg1 -arg2\n"
        << L"  capl.exe --mode alle -- cmd.exe /c \"batch.cmd\"\n"
        << L"  capl.exe --cores 0,2,4 -- program.exe\n"
        << L"  capl.exe --pattern 0x40 -- program.exe\n"
        << L"  capl.exe --query\n\n"
        << L"Notes:\n"
        << L"  - Either --mode or --cores must be specified for launching\n"
        << L"  - Core numbers must be non-negative and within system limits\n"
        << L"  - --mode all explicitly locks process to all cores, preventing\n"
        << L"    Windows from dynamically restricting core usage\n\n"
        << L"Pattern Detection Notes:\n"
        << L"  - Known patterns: P-cores=0x40, E-cores=0x20, LP E-cores=0x30\n"
        << L"  - Custom patterns are experimental and may not work on all CPUs\n"
        << L"  - Use --query first to understand your CPU's configuration\n\n"
        //<< std::endl;
		<< std::endl << std::flush;  // Add explicit flush
}	if (g_messageHandler) {
	if (g_messageHandler) {
		g_messageHandler->ShowHelp(GetHelpText());
	}
}

