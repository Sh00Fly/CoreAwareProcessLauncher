//options.cpp
#include "pch.h"
#include "options.h"
#include "utilities.h"
#include <sstream>
#include <format>
#include <iostream>

using Utilities::ConvertToNarrowString;
using Utilities::ConvertToWideString;

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
	g_logger->Log(ApplicationLogger::Level::DEBUG, "Raw command line: " + ConvertToNarrowString(GetCommandLineW()));
	for (int i = 0; i < argc; i++) {
		std::wstring arg = argv[i];
		g_logger->Log(ApplicationLogger::Level::DEBUG,
			"Arg[" + std::to_string(i) + "] length: " +
			std::to_string(arg.length()));
		g_logger->Log(ApplicationLogger::Level::DEBUG,
			"Arg[" + std::to_string(i) + "] raw: " + ConvertToNarrowString(arg));
	}

	for (int i = 1; i < argc; i++) {
		std::wstring arg = argv[i];

		g_logger->Log(ApplicationLogger::Level::DEBUG,
			"Processing argument: '" + ConvertToNarrowString(arg) + "'");

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
				throw std::runtime_error(ConvertToNarrowString(L"Invalid mode. Use: p, e, lp, alle, all"));
			}
		}
		else if ((arg == L"--pattern") && i + 1 < argc) {
			options.affinityMode = CommandLineOptions::CoreAffinityMode::PATTERN;
			std::wstring patternStr = argv[++i];
			try {
				options.pattern = std::stoul(patternStr, nullptr, 16);
				if (options.pattern > 0xFF) {
					throw std::runtime_error(ConvertToNarrowString(L"Pattern must be between 0x00 and 0xFF"));
				}
			}
			catch (...) {
				throw std::runtime_error(ConvertToNarrowString(L"Invalid pattern format. Use hex value (e.g., 0x40)"));
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
					throw std::runtime_error(ConvertToNarrowString(L"Invalid core number in list"));
				}
			}

			// Validate core numbers
			for (int core : options.cores) {
				if (core < 0) {
					throw std::runtime_error(ConvertToNarrowString(L"Core numbers must be non-negative"));
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
			throw std::runtime_error(ConvertToNarrowString(L"Unknown option: " + arg));
		}
	}

	// Comprehensive validation
	try {
		bool isQueryOrHelp = options.queryMode || options.showHelp;

		// Basic requirements
		if (!isQueryOrHelp && !foundDelimiter) {
			throw std::runtime_error(ConvertToNarrowString(L"Program command line must be specified after --"));
		}
		// Target validation message
		if (!isQueryOrHelp && options.targetPath.empty()) {
			throw std::runtime_error(ConvertToNarrowString(L"Target program path must be specified after --"));
		}

		// Working directory validation
		if (!options.targetWorkingDir.empty()) {
			DWORD attrs = GetFileAttributesW(options.targetWorkingDir.c_str());
			if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
				throw std::runtime_error(ConvertToNarrowString(L"Invalid working directory: " +
					options.targetWorkingDir));
			}
			g_logger->Log(ApplicationLogger::Level::INFO,
				"Working directory validated: " + ConvertToNarrowString(options.targetWorkingDir));
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
				throw std::runtime_error(ConvertToNarrowString(L"Duplicate core numbers in --cores list"));
			}

			// Validate core numbers
			for (int core : options.cores) {
				if (core >= static_cast<int>(sysInfo.dwNumberOfProcessors)) {
					std::wstring errorMsg = std::format(L"Core number {} exceeds system limit of {}",
						core, sysInfo.dwNumberOfProcessors - 1);
					throw std::runtime_error(ConvertToNarrowString(errorMsg));
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
		if (options.enableLogging) {
			if (options.logPath.empty()) {
				options.logPath = Utilities::DEFAULT_LOG_PATH;
			}
		}

		// Mutually exclusive options
		if (options.queryMode && !options.targetPath.empty()) {
			throw std::runtime_error(ConvertToNarrowString(L"--query cannot be used with --target"));
		}

		g_logger->Log(ApplicationLogger::Level::INFO, "Command line validation completed successfully");
	}
	catch (const std::exception& e) {
		g_logger->Log(ApplicationLogger::Level::ERR, "Validation failed: " + std::string(e.what()));
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
  --pattern <hex>        Custom CPUID pattern detection (e.g., 0x40)
                         Advanced: Use with caution
  --invert, -i           Invert core selection\n

Process Control:
  --dir, -d <path>       Working directory for target process
  -- <program> [args]     Program to launch with its arguments\n

Utility Options:
  --query, -q            Show system information only
  --log, -l              Enable logging (disabled by default)
  --logpath <path>       Specify log file path (default: capl.log)
  --help, -h, -?, /?     Show this help\n

Examples:
  capl.exe --mode p -- notepad.exe \"file.txt\"
  capl.exe --mode lp --dir \"C:\\Work\" -- program.exe -arg1 -arg2
  capl.exe --mode alle -- cmd.exe /c \"batch.cmd\"
  capl.exe --cores 0,2,4 -- program.exe
  capl.exe --pattern 0x40 -- program.exe
  capl.exe --query\n

Notes:
  - Either --mode or --cores must be specified for launching
  - Core numbers must be non-negative and within system limits
  - --mode all explicitly locks process to all cores, preventing
    Windows from dynamically restricting core usage\n

Pattern Detection Notes:
  - Known patterns: P-cores=0x40, E-cores=0x20, LP E-cores=0x30
  - Custom patterns are experimental and may not work on all CPUs
  - Use --query first to understand your CPU's configuration)";
}

void ShowHelp() {
	if (g_messageHandler) {
		g_messageHandler->ShowHelp(GetHelpText());
	}
}

