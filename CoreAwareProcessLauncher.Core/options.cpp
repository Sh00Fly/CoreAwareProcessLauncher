// options.cpp
#include "pch.h"
#include "options.h"
#include "utilities.h"
#include <format>
#include <iostream>
#include <sstream>

using Utilities::ConvertToNarrowString;
using Utilities::ConvertToWideString;
using Utilities::IsDirectory;
using Utilities::PathExists;

CommandLineOptions::CommandLineOptions()
    : invertSelection(false), queryMode(false), enableLogging(false),
      showHelp(false) {}

CommandLineOptions ParseCommandLine(int argc, wchar_t *argv[]) {
    CommandLineOptions options;
    bool foundDelimiter = false;
    bool foundMode = false;
    bool foundCores = false;
    bool targetDirErr = false;
    std::string targetDirErrMsg = "";
    bool foundLogpath = false;
    bool logpathErr = false;
    std::string logpathErrMsg = "";

    g_logger->Log(ApplicationLogger::Level::INFO,
                  "Starting command line parsing with " + std::to_string(argc) +
                      " arguments");

    if (argc == 1) {
        options.showHelp = true;
        return options;
    }

    // Print the raw command line
    g_logger->Log(ApplicationLogger::Level::DEBUG,
                  "Raw command line: " +
                      ConvertToNarrowString(GetCommandLineW()));
    for (int i = 0; i < argc; i++) {
        std::wstring arg = argv[i];
        g_logger->Log(ApplicationLogger::Level::DEBUG,
                      "Arg[" + std::to_string(i) +
                          "] length: " + std::to_string(arg.length()));
        g_logger->Log(ApplicationLogger::Level::DEBUG,
                      "Arg[" + std::to_string(i) +
                          "] raw: " + ConvertToNarrowString(arg));
    }

    for (int i = 1; i < argc; i++) {
        std::wstring arg = argv[i];

        g_logger->Log(ApplicationLogger::Level::DEBUG,
                      "Processing argument: '" + ConvertToNarrowString(arg) +
                          "'");

        if (arg == L"--") {
            foundDelimiter = true;
            // Collect everything after -- as target path and args
            if (i + 1 < argc) {
                options.targetPath = argv[i + 1];
                for (int j = i + 2; j < argc; j++) {
                    options.targetArgs.push_back(argv[j]);
                }
            }
            break; // Stop processing arguments after --
        }
        // --help
        if (arg == L"--help" || arg == L"-h" || arg == L"-?" || arg == L"/?") {
            g_logger->Log(ApplicationLogger::Level::INFO,
                          "Help option detected");
            options.showHelp = true;
            return options;

            // --query
        } else if (arg == L"--query" || arg == L"-q") {
            options.queryMode = true;

            // --dir
        } else if ((arg == L"--dir" || arg == L"-d")) {
            if (i + 1 >= argc) { // Check if there's a next argument
                targetDirErr = true;
                targetDirErrMsg = "--dir option requires a path argument";
            } else {
                // Get the next argument
                std::wstring path = argv[i + 1];
                // Check if it's not another option (doesn't start with -)
                if (path.starts_with(L"-")) {
                    targetDirErr = true;
                    targetDirErrMsg =
                        "--dir option requires a path argument, got: " +
                        ConvertToNarrowString(path);
                }
                // Check if path exists
                else if (!(PathExists(path.c_str()))) {
                    targetDirErr = true;
                    targetDirErrMsg = "Specified directory does not exist: " +
                                      ConvertToNarrowString(path);
                    i++;
                }
                // Check if it's a directory
                else if (!(IsDirectory(path.c_str()))) {
                    targetDirErr = true;
                    targetDirErrMsg = "Specified path is not a directory: " +
                                      ConvertToNarrowString(path);
                    i++;
                }
            }
            if (!targetDirErr) {
                options.targetWorkingDir = argv[++i];
            }

            // --mode
        } else if ((arg == L"--mode" || arg == L"-m") && i + 1 < argc) {
            foundMode = true;
            std::wstring mode = argv[++i];
            if (mode == L"p") {
                options.affinityMode =
                    CommandLineOptions::CoreAffinityMode::P_CORES_ONLY;
            } else if (mode == L"e") {
                options.affinityMode =
                    CommandLineOptions::CoreAffinityMode::E_CORES_ONLY;
            } else if (mode == L"lp") {
                options.affinityMode =
                    CommandLineOptions::CoreAffinityMode::LP_CORES_ONLY;
            } else if (mode == L"alle") {
                options.affinityMode =
                    CommandLineOptions::CoreAffinityMode::ALL_E_CORES;
            } else if (mode == L"all") {
                options.affinityMode =
                    CommandLineOptions::CoreAffinityMode::ALL_CORES;
            } else {
                throw std::runtime_error(ConvertToNarrowString(
                    L"Invalid mode. Use: p, e, lp, alle, all"));
            }
		} else if ((arg == L"--mode" || arg == L"-m") && i + 1 >= argc) {
            foundMode = true;
			i++;
            // --cores
        } else if (arg == L"--cores" && i + 1 < argc) {
            foundCores = true;
            options.affinityMode = CommandLineOptions::CoreAffinityMode::CUSTOM;
            std::wstring coreList = argv[++i];
            std::wstringstream ss(coreList);
            std::wstring coreNum;

            while (std::getline(ss, coreNum, L',')) {
                try {
                    options.cores.push_back(std::stoi(coreNum));
                } catch (...) {
                    throw std::runtime_error(
                        ConvertToNarrowString(L"Invalid core number in list"));
                }
            }

            // Validate core numbers
            for (int core : options.cores) {
                if (core < 0) {
                    throw std::runtime_error(ConvertToNarrowString(
                        L"Core numbers must be non-negative"));
                }
            }

            // --invert
        } else if (arg == L"--invert" || arg == L"-i") {
            options.invertSelection = true;

            // --log
        } else if (arg == L"--log" || arg == L"-l") {
            options.enableLogging = true;

            // --logpath
        } else if (arg == L"--logpath") {
            if (i + 1 >= argc) { // Check if there's a next argument
                logpathErr = true;
                logpathErrMsg =
                    "--logpath option requires a file name argument";
            } else {
                // Get the next argument
                std::wstring path = argv[i + 1];
                // Check if it's not another option (doesn't start with -)
                if (path.starts_with(L"-")) {
                    logpathErr = true;
                    logpathErrMsg = "--logpath option requires a file name "
                                    "argument, got: " +
                                    ConvertToNarrowString(path);
                }
                // Check if it's a directory
                if (IsDirectory(path.c_str())) {
                    logpathErr = true;
                    logpathErrMsg = "Specified path is a directory: " +
                                    ConvertToNarrowString(path);
                }
            }
            if (!logpathErr) {
                options.logPath = argv[++i];
            }

            // --Unknown option
        } else {
            throw std::runtime_error(
                ConvertToNarrowString(L"Unknown option: " + arg));
        }
    }

    // Comprehensive validation
    try {
        bool isQueryOrHelp = options.queryMode || options.showHelp;

        // Basic requirements
        if (!options.queryMode && !options.showHelp &&
            options.affinityMode ==
                CommandLineOptions::CoreAffinityMode::NOT_SET) {
            throw std::runtime_error(ConvertToNarrowString(
                L"Either of --query, --help, or Affinity mode "
                L"(--mode or --cores) must be specified"));
        }
        if (!isQueryOrHelp && !foundDelimiter) {
            throw std::runtime_error(ConvertToNarrowString(
                L"Program command line must be specified after --"));
        }

        // Mutually exclusive options
        if (options.queryMode && !options.targetPath.empty()) {
            throw std::runtime_error(
                ConvertToNarrowString(L"--query cannot be used with --target"));
        }
        if (options.queryMode && foundMode) {
            throw std::runtime_error(
                ConvertToNarrowString(L"--query cannot be used with --mode"));
        }
        if (foundCores && foundMode) {
            throw std::runtime_error(ConvertToNarrowString(
                L"--mode and --cores cannot be used at the same time"));
        }
        if (options.invertSelection && !foundCores && !foundMode) {
            throw std::runtime_error(ConvertToNarrowString(
                L"--invert must be used with --mode or --cores"));
        }
        if (isQueryOrHelp &&
            (!options.targetWorkingDir.empty() || targetDirErr)) {
            throw std::runtime_error(ConvertToNarrowString(
                L"--dir cannot be used with --query or --help"));
        }

        // Target validation message
        if (!isQueryOrHelp && options.targetPath.empty()) {
            throw std::runtime_error(ConvertToNarrowString(
                L"Target program path must be specified after --"));
        }

        // Working directory validation
        if (!options.targetWorkingDir.empty() && options.targetPath.empty()) {
            throw std::runtime_error(ConvertToNarrowString(
                L"--dir must be used with -- <target.exe>"));
        }

        if (targetDirErr) {
            throw std::runtime_error(targetDirErrMsg);
        }
        g_logger->Log(ApplicationLogger::Level::INFO,
                      "Working directory validated: " +
                          ConvertToNarrowString(options.targetWorkingDir));

        // Logpath validation
        if (foundLogpath && !options.enableLogging) {
            throw std::runtime_error(
                ConvertToNarrowString(L"--logpath must be used with --log"));
        }
        if (!options.logPath.empty() && !options.enableLogging) {
            throw std::runtime_error(
                ConvertToNarrowString(L"--logpath must be used with --log"));
        }

        if (logpathErr) {
            throw std::runtime_error(logpathErrMsg);
        }
        g_logger->Log(ApplicationLogger::Level::INFO,
                      "Logpath validated: " +
                          ConvertToNarrowString(options.logPath));

        // System limits for cores
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        g_logger->Log(ApplicationLogger::Level::INFO,
                      "System has " +
                          std::to_string(sysInfo.dwNumberOfProcessors) +
                          " processors");

        if (options.affinityMode ==
            CommandLineOptions::CoreAffinityMode::CUSTOM) {
            // Check for duplicate cores
            std::set<int> uniqueCores(options.cores.begin(),
                                      options.cores.end());
            if (uniqueCores.size() != options.cores.size()) {
                throw std::runtime_error(ConvertToNarrowString(
                    L"Duplicate core numbers in --cores list"));
            }

            // Validate core numbers
            for (int core : options.cores) {
                if (core >= static_cast<int>(sysInfo.dwNumberOfProcessors)) {
                    std::wstring errorMsg = std::format(
                        L"Core number {} exceeds system limit of {}", core,
                        sysInfo.dwNumberOfProcessors - 1);
                    throw std::runtime_error(ConvertToNarrowString(errorMsg));
                }
            }

            std::string coreList;
            for (int core : options.cores) {
                if (!coreList.empty())
                    coreList += ",";
                coreList += std::to_string(core);
            }
            g_logger->Log(ApplicationLogger::Level::INFO,
                          "Valid core list specified: " + coreList);
        }

        // Log path validation
        if (!options.enableLogging && !options.logPath.empty()) {
            throw std::runtime_error(
                ConvertToNarrowString(L"--logPath must be used with --log"));
        }

        if (options.enableLogging) {
            if (options.logPath.empty()) {
                options.logPath = Utilities::DEFAULT_LOG_PATH;
            }
        }

        g_logger->Log(ApplicationLogger::Level::INFO,
                      "Command line validation completed successfully");
    } catch (const std::exception &e) {
        g_logger->Log(ApplicationLogger::Level::ERR,
                      "Validation failed: " + std::string(e.what()));
        throw; // Re-throw for main() to handle
    }

    return options;
}

std::wstring GetHelpText() {
    return LR"(
Core Aware Process Launcher (CAPL)

Usage: caplcli.exe [options] -- <program> [program arguments]
Usage: caplgui.exe [options] -- <program> [program arguments]

Core Affinity Modes:
  --mode, -m <mode>      Predefined modes:
                         p     - P-cores only (0x40)
                         e     - E-cores only (0x20)
                         lp    - LP E-cores only (0x30)
                         alle  - All E-cores (E + LP)
                         all   - Lock to all cores
  --cores <list>         Custom core selection (comma-separated)
  --invert, -i           Invert core selection\n

Process Control:
  --dir, -d <path>       Working directory for target process
  -- <program> [args]     Program to launch with its arguments\n

Utility Options:
  --query, -q            Show system information only
  --log, -l              Enable logging (disabled by default)
  --logpath <path>       Specify log file path (default: capl.log)
  --help, -h, -?, /?     Show this help

Examples:
  caplcli.exe --mode p -- notepad.exe \"file.txt\"
  caplgui.exe --mode lp --dir \"C:\\Work\" -- program.exe -arg1 -arg2
  caplcli.exe --mode alle -- cmd.exe /c \"batch.cmd\"
  caplgui.exe --cores 0,2,4 -- program.exe
  caplcli.exe --query

Notes:
  - Either --mode or --cores must be specified for launching
  - Core numbers must be non-negative and within system limits
  - --mode all explicitly locks process to all cores, preventing
    Windows from dynamically restricting core usage)";
}

void ShowHelp() {
    if (g_messageHandler) {
        g_messageHandler->ShowHelp(GetHelpText());
    }
}
