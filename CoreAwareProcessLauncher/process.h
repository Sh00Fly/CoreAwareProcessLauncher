#pragma once
#include <windows.h>
#include <string>

class ProcessManager {
public:
    static bool LaunchProcess(
        const std::wstring& path, 
        DWORD_PTR affinityMask);

private:
    static void LogWin32Error(const std::string& context);
};
