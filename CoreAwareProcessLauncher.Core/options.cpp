#include "pch.h"
#include "options.h"
#include "logger.h"
#include <sstream>
#include <format>
#include <iostream>

CommandLineOptions::CommandLineOptions() :
    cpuidValue(0),
    invertSelection(false),
    queryMode(false),
    enableLogging(false),
    showHelp(false) {
}

CommandLineOptions ParseCommandLine(int argc, wchar_t* argv[]) {
    CommandLineOptions options;
    
    if (argc == 1) {
        options.showHelp = true;
        return options;
    }

    g_logger->Log(Logger::Level::INFO, "Starting command line parsing");
    
    // ... existing argument parsing ...

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
                    std::string(options.targetPath.begin(), options.targetPath.end()));
            }
            g_logger->Log(Logger::Level::INFO, "Target executable found: " + 
                std::string(options.targetPath.begin(), options.targetPath.end()));
        }

        // System limits for cores
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        g_logger->Log(Logger::Level::INFO, "System has " + 
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
            g_logger->Log(Logger::Level::INFO, "Valid core list specified: " + coreList);
        }

        // CPUID validation
        if (options.detectMode == "CUSTOM") {
            if (options.cpuidValue == 0) {
                throw std::runtime_error("CPUID value is required for CUSTOM detection mode");
            }
            if (options.cpuidValue > 0xFF) {
                throw std::runtime_error("CPUID value must be between 0x00 and 0xFF");
            }
            g_logger->Log(Logger::Level::INFO, "Custom CPUID value: 0x" + 
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

        g_logger->Log(Logger::Level::INFO, "Command line validation completed successfully");
    }
    catch (const std::exception& e) {
        g_logger->Log(Logger::Level::ERR, "Validation failed: " + std::string(e.what()));
        throw; // Re-throw for main() to handle
    }
    
    return options;
}

void ShowHelp() {
    std::wcout << L"Core Aware Process Launcher (CAPL)\n"
               << L"Usage: CAPL.exe [options]\n\n"
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
               << L"  --log, -l              Enable logging\n"
               << L"  --logpath <path>       Specify log file path\n"
               << L"  --help, -h, -?         Show this help\n\n"
               << L"Examples:\n"
               << L"  CAPL.exe --target \"program.exe\" --detect P\n"
               << L"  CAPL.exe --target \"program.exe\" --cores 0,2,4,6\n"
               << L"  CAPL.exe --detect CUSTOM --cpuid 40 --target \"program.exe\"\n"
               << L"  CAPL.exe --query\n\n"
               << L"Notes:\n"
               << L"  - Either --detect or --cores must be specified for launching\n"
               << L"  - --cpuid is required when using --detect CUSTOM\n"
               << L"  - Core numbers must be non-negative and within system limits\n"
               << std::endl;
}