#pragma once
#include <windows.h>
#include <vector>
#include <cstdint>
#include <string>   

class CpuInfo {
public:
    // Get mask for specific core types using CPUID
    static DWORD_PTR GetCoreMask(uint32_t cpuidValue);
    
    // Get mask for P-cores (wrapper for common case)
    static DWORD_PTR GetPCoreMask() {
        return GetCoreMask(0x40); // P-core identifier
    }
    
    // Get mask for E-cores (if needed in future)
    static DWORD_PTR GetECoreMask() {
        return GetCoreMask(0x20); // E-core identifier (verify value)
    }
    
    // Convert core numbers to mask
    static DWORD_PTR CoreListToMask(const std::vector<int>& cores);
    
    // Get system CPU information for query mode
    static std::wstring QuerySystemInfo();  

private:
    static void ExecuteCpuid(int cpuInfo[4], int leaf, int subleaf);
};
