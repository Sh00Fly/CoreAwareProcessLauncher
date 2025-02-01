//options.h
#pragma once
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <windows.h>

struct CommandLineOptions {
    std::wstring targetPath;
    std::vector<std::wstring> targetArgs;
    std::wstring targetWorkingDir;
    
    enum class CoreAffinityMode {
        P_CORES_ONLY,     // Only Performance cores
        E_CORES_ONLY,     // Only regular E-cores
        LP_CORES_ONLY,    // Only LP E-cores
        ALL_E_CORES,      // Both E-cores and LP E-cores
        ALL_CORES,        // Lock to all cores
        CUSTOM,          // Custom core selection via --cores
        PATTERN          // Custom CPUID pattern detection
    } affinityMode = CoreAffinityMode::CUSTOM;
    
    std::vector<int> cores;    // Used when mode is CUSTOM
    uint32_t pattern;          // Used when mode is PATTERN
    bool invertSelection;
    bool queryMode;
    bool enableLogging;
    std::wstring logPath;
    bool showHelp;
    
    CommandLineOptions();
};

CommandLineOptions ParseCommandLine(int argc, wchar_t* argv[]);
void ShowHelp();