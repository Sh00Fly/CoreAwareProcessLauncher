//cpu.cpp
#include "pch.h"
#include "cpu.h"
#include "logger.h"
#include <intrin.h>
#include <sstream>
#include <format>

CpuInfo::CpuCapabilities CpuInfo::GetCapabilities() {
    CpuCapabilities caps = {};  // Initialize all members to 0/false/empty
    int cpuInfo[4] = {0};

    // Get processor brand string
    ExecuteCpuid(cpuInfo, 0x80000000, 0);
    if (cpuInfo[0] >= 0x80000004) {
        char brand[0x40] = {};  // Initialize array to zeros
        ExecuteCpuid(cpuInfo, 0x80000002, 0);
        memcpy(brand, cpuInfo, sizeof(cpuInfo));
        ExecuteCpuid(cpuInfo, 0x80000003, 0);
        memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));
        ExecuteCpuid(cpuInfo, 0x80000004, 0);
        memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));
        caps.brandString = std::wstring(brand, brand + strlen(brand));
    }

    // Check for leaf 0x1A support
    ExecuteCpuid(cpuInfo, 0, 0);
    caps.supportsLeaf1A = (cpuInfo[0] >= 0x1A);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    caps.totalCores = sysInfo.dwNumberOfProcessors;

    // Detect core types if leaf 0x1A is supported
    if (caps.supportsLeaf1A) {
        caps.pCoreMask = GetPCoreMask();
        caps.eCoreMask = GetECoreMask();
        caps.lpECoreMask = GetLpECoreMask();
        caps.isHybrid = (caps.pCoreMask != 0);
    }

    return caps;
}

std::wstring CpuInfo::GetDetailedInfo() {
    auto caps = GetCapabilities();
    std::wstringstream ss;
    
    ss << L"CPU Information:\n"
       << L"Brand: " << caps.brandString << L"\n"
       << L"Total Cores: " << caps.totalCores << L"\n"
       << L"Hybrid Architecture: " << (caps.isHybrid ? L"Yes" : L"No") << L"\n"
       << L"Supports Core Type Detection: " << (caps.supportsLeaf1A ? L"Yes" : L"No") << L"\n";

    if (caps.isHybrid) {
        ss << L"P-core mask: 0x" << std::hex << caps.pCoreMask << L"\n"
           << L"E-core mask: 0x" << caps.eCoreMask << L"\n"
           << L"LP E-core mask: 0x" << caps.lpECoreMask << L"\n"  // Add this line
           << L"P-cores: ";        

        for (int i = 0; i < caps.totalCores; i++) {
            if (caps.pCoreMask & (1ULL << i)) {
                ss << std::dec << i << L" ";
            }
        }
        
        ss << L"\nE-cores: ";
        for (int i = 0; i < caps.totalCores; i++) {
            if (caps.eCoreMask & (1ULL << i)) {
                ss << std::dec << i << L" ";
            }
        }

        ss << L"\nLP E-cores: ";
        for (int i = 0; i < caps.totalCores; i++) {
            if (caps.lpECoreMask & (1ULL << i)) {
                ss << std::dec << i << L" ";
            }
        }
    }
    
    return ss.str();
}

DWORD_PTR CpuInfo::GetCoreMask(uint32_t cpuidValue) {
    int cpuInfo[4] = {0};
    DWORD_PTR mask = 0;
    
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    g_logger->Log(ApplicationLogger::Level::INFO, 
        "Detecting cores with CPUID value: 0x" + std::format("{:02X}", cpuidValue));
        //"Detecting cores with CPUID value: 0x" + std::format("{:02X}"_fmt, cpuidValue)
    for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
        DWORD_PTR threadMask = 1ULL << i;
        SetThreadAffinityMask(GetCurrentThread(), threadMask);
        Sleep(0); // Allow thread to actually switch cores
        
        ExecuteCpuid(cpuInfo, 0x1A, 0);
        if (((cpuInfo[0] >> 24) & 0xFF) == cpuidValue) {
            mask |= threadMask;
            g_logger->Log(ApplicationLogger::Level::DEBUG, 
                "Core " + std::to_string(i) + " matches type");
        }
    }
    
    // Restore thread affinity
    SetThreadAffinityMask(GetCurrentThread(), 
        (1ULL << sysInfo.dwNumberOfProcessors) - 1);
    
    g_logger->Log(ApplicationLogger::Level::INFO, 
        "Detected core mask: 0x" + std::format("{:X}", mask));
    
    return mask;
}

DWORD_PTR CpuInfo::CoreListToMask(const std::vector<int>& cores) {
    DWORD_PTR mask = 0;
    for (int core : cores) {
        mask |= (1ULL << core);
    }
    return mask;
}

void CpuInfo::ExecuteCpuid(int cpuInfo[4], int leaf, int subleaf) {
    __cpuidex(cpuInfo, leaf, subleaf);
}

std::wstring CpuInfo::QuerySystemInfo() {
    std::wstringstream ss;
    auto caps = GetCapabilities();
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    ss << L"\n";  // Start with newline
    ss << L"System CPU Information:\n"
       << L"Processor: " << caps.brandString << L"\n"
       << L"Number of processors: " << sysInfo.dwNumberOfProcessors << L"\n"
       << L"Processor architecture: ";
    
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            ss << L"x64 (AMD or Intel)\n";
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            ss << L"x86\n";
            break;
        default:
            ss << L"Other\n";
    }

    ss << L"Hybrid Architecture: " << (caps.isHybrid ? L"Yes" : L"No") << L"\n"
       << L"Supports Core Type Detection: " << (caps.supportsLeaf1A ? L"Yes" : L"No") << L"\n";
    
    if (caps.isHybrid) {
        DWORD_PTR pCoreMask = GetPCoreMask();
        DWORD_PTR eCoreMask = GetECoreMask();
        DWORD_PTR lpECoreMask = GetLpECoreMask();
        
        ss << L"\nPerformance Cores:\n"
           << L"Mask: 0x" << std::hex << pCoreMask << L"\n"
           << L"Threads: ";
        for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
            if (pCoreMask & (1ULL << i)) {
                ss << std::dec << i << L" ";
            }
        }
        ss << L"\n";
        
        ss << L"\nEfficiency Cores:\n"
           << L"Mask: 0x" << std::hex << eCoreMask << L"\n"
           << L"Threads: ";
        for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
            if (eCoreMask & (1ULL << i)) {
                ss << std::dec << i << L" ";
            }
        }
        ss << L"\n";

        ss << L"\nLow Power Efficiency Cores:\n"
           << L"Mask: 0x" << std::hex << lpECoreMask << L"\n"
           << L"Threads: ";
        for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
            if (lpECoreMask & (1ULL << i)) {
                ss << std::dec << i << L" ";
            }
        }
        ss << L"\n";

    } else {
        // Non-hybrid CPU output
        DWORD_PTR allCoresMask = (1ULL << sysInfo.dwNumberOfProcessors) - 1;
        ss << L"Core mask: 0x" << std::hex << allCoresMask << L"\n"
           << L"Available threads: ";
        for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
            ss << std::dec << i << L" ";
        }
        ss << L"\n";
    }
    
	ss << L"\n";  // End with newline
	ss << L"\n";  // End with newline
    return ss.str();
}
