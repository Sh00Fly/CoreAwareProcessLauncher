//options.cpp
#include "pch.h"
#include "options.h"
#include "logger.h"
#include <sstream>
#include <format>
#include <iostream>

// Helper function to convert wide string to narrow string
std::string WideToUtf8(const std::wstring& wide) {
	if (wide.empty()) return std::string();

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
		static_cast<int>(wide.length()), nullptr, 0, nullptr, nullptr);

	std::string utf8(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
		static_cast<int>(wide.length()),
		&utf8[0], size_needed, nullptr, nullptr);

	return utf8;
}
CommandLineOptions::CommandLineOptions() :
	cpuidValue(0),
	invertSelection(false),
	queryMode(false),
	enableLogging(false),
	showHelp(false) {
}

CommandLineOptions ParseCommandLine(int argc, wchar_t* argv[]) {
	CommandLineOptions options;

	g_logger->Log(ApplicationLogger::Level::INFO, "Starting command line parsing with " + std::to_string(argc) + " arguments");

	if (argc == 1) {
		options.showHelp = true;
		return options;
	}

	// Print the raw command line
	g_logger->Log(ApplicationLogger::Level::DEBUG, "Raw command line: " + WideToUtf8(GetCommandLineW()));
	for (int i = 0; i < argc; i++) {  // Start from 0 to see program name too
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

		if (arg == L"--help" || arg == L"-h" || arg == L"-?" || arg == L"/?") {
			g_logger->Log(ApplicationLogger::Level::INFO, "Help option detected");
			options.showHelp = true;
			return options;
		}
		else if (arg == L"--query" || arg == L"-q") {
			options.queryMode = true;
		}
		else if ((arg == L"--target" || arg == L"-t") && i + 1 < argc) {
			options.targetPath = argv[++i];
		}
		else if ((arg == L"--detect" || arg == L"-d") && i + 1 < argc) {
			options.selectionMethod = CommandLineOptions::SelectionMethod::DETECT;
		    options.detectMode = argv[++i]; 

			if (options.detectMode != L"P" &&
				options.detectMode != L"E" &&
				options.detectMode != L"CUSTOM") {
				throw std::runtime_error("Invalid detection mode. Use P, E, or CUSTOM");
			}
		}
		else if ((arg == L"--cpuid" || arg == L"-c") && i + 1 < argc) {
			if (options.selectionMethod != CommandLineOptions::SelectionMethod::DETECT) {
				throw std::runtime_error("--cpuid can only be used with --detect CUSTOM");
			}
			try {
				options.cpuidValue = std::stoul(argv[++i], nullptr, 16);
			}
			catch (...) {
				throw std::runtime_error("Invalid CPUID value");
			}
		}
		else if ((arg == L"--cores") && i + 1 < argc) {
			if (options.selectionMethod == CommandLineOptions::SelectionMethod::DETECT) {
				throw std::runtime_error("Cannot use both --detect and --cores");
			}
			options.selectionMethod = CommandLineOptions::SelectionMethod::DIRECT;

			// Parse comma-separated list of core numbers
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
		if (!isQueryOrHelp && options.targetPath.empty()) {
			throw std::runtime_error("Target path is required for process launching");
		}

		if (!isQueryOrHelp && options.selectionMethod == CommandLineOptions::SelectionMethod::NONE) {
			throw std::runtime_error("Either --detect or --cores must be specified");
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

		if (options.selectionMethod == CommandLineOptions::SelectionMethod::DIRECT) {
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

		// CPUID validation
		if (options.detectMode == L"CUSTOM") {
			if (options.cpuidValue == 0) {
				throw std::runtime_error("CPUID value is required for CUSTOM detection mode");
			}
			if (options.cpuidValue > 0xFF) {
				throw std::runtime_error("CPUID value must be between 0x00 and 0xFF");
			}
			g_logger->Log(ApplicationLogger::Level::INFO, "Custom CPUID value: 0x" +
				std::format("{:02X}", options.cpuidValue));
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
	std::wcout << L"Core Aware Process Launcher (CAPL)\n"
		<< L"Usage: capl.exe [options]\n\n"
		<< L"Core Selection (use either detection or direct specification):\n"
		<< L"  --detect, -d <mode>    Detection mode:\n"
		<< L"                         P     - P-cores (Performance)\n"
		<< L"                         E     - E-cores (Efficiency)\n"
		<< L"                         CUSTOM - Custom detection\n"
		<< L"  --cpuid, -c <value>    Custom CPUID value (hex) for CUSTOM mode\n"
		<< L"  --cores <list>         Direct core specification (comma-separated)\n"
		<< L"  --invert, -i           Invert core selection\n\n"
		<< L"Process Control:\n"
		<< L"  --target, -t <path>    Target executable path\n\n"
		<< L"Utility Options:\n"
		<< L"  --query, -q            Show system information only\n"
		<< L"  --log, -l              Enable logging (disabled by default)\n"
		<< L"  --logpath <path>       Specify log file path (default: capl.log)\n"
		<< L"  --help, -h, -?, /?     Show this help\n\n"
		<< L"Examples:\n"
		<< L"  capl.exe --target \"program.exe\" --detect P\n"
		<< L"  capl.exe --target \"program.exe\" --cores 0,2,4,6\n"
		<< L"  capl.exe --detect CUSTOM --cpuid 40 --target \"program.exe\"\n"
		<< L"  capl.exe --query\n\n"
		<< L"Notes:\n"
		<< L"  - Either --detect or --cores must be specified for launching\n"
		<< L"  - --cpuid is required when using --detect CUSTOM\n"
		<< L"  - Core numbers must be non-negative and within system limits\n"
		<< std::endl;
}