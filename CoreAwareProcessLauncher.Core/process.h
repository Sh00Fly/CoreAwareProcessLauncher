//process.h
#pragma once
#include <windows.h>
#include <string>
#include <vector>

class ProcessManager {
public:
    static bool LaunchProcess(
        const std::wstring& path,
        const std::vector<std::wstring>& args,
        const std::wstring& workingDir,
        DWORD_PTR affinityMask);

private:
    static void LogWin32Error(const std::string& context);
    static std::wstring BuildCommandLine(
        const std::wstring& path,
        const std::vector<std::wstring>& args);
};