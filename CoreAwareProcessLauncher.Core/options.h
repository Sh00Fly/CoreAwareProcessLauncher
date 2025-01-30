//options.h
#pragma once
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <windows.h>

struct CommandLineOptions {
    std::wstring targetPath;
    
    enum class SelectionMethod {
        NONE,
        DETECT,
        DIRECT
    } selectionMethod = SelectionMethod::NONE;
    
    std::string detectMode;    // "P", "E", "CUSTOM"
    uint32_t cpuidValue;       // For custom detection
    std::vector<int> cores;    // List of core numbers
    
    bool invertSelection;
    bool queryMode;
    bool enableLogging;
    std::wstring logPath;
    bool showHelp;
    
    CommandLineOptions();
};

CommandLineOptions ParseCommandLine(int argc, wchar_t* argv[]);
void ShowHelp();
