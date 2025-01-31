//cpu.h
#pragma once
#include <windows.h>
#include <vector>
#include <cstdint>
#include <string>   

class CpuInfo {
public:
    struct CpuCapabilities {
        bool isHybrid;
        bool supportsLeaf1A;
        int totalCores;
        std::wstring brandString;
        DWORD_PTR pCoreMask;
        DWORD_PTR eCoreMask;
    };

    static DWORD_PTR GetCoreMask(uint32_t cpuidValue);
    static DWORD_PTR GetPCoreMask() { return GetCoreMask(0x40); }
    static DWORD_PTR GetECoreMask() { return GetCoreMask(0x20); }
    static DWORD_PTR CoreListToMask(const std::vector<int>& cores);
    static std::wstring QuerySystemInfo();
    static CpuCapabilities GetCapabilities();
    static std::wstring GetDetailedInfo();

private:
    static void ExecuteCpuid(int cpuInfo[4], int leaf, int subleaf);
    static bool CheckCpuidSupport(int leaf);
};
