#include "cpu.h"
#include "logger.h"
#include <intrin.h>
#include <sstream>
#include <format>

DWORD_PTR CpuInfo::GetCoreMask(uint32_t cpuidValue) {
    int cpuInfo[4] = {0};
    DWORD_PTR mask = 0;
    
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    g_logger->Log(Logger::Level::INFO, 
        "Detecting cores with CPUID value: 0x" + std::format("{:02X}", cpuidValue));
        //"Detecting cores with CPUID value: 0x" + std::format("{:02X}"_fmt, cpuidValue)
    for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
        DWORD_PTR threadMask = 1ULL << i;
        SetThreadAffinityMask(GetCurrentThread(), threadMask);
        Sleep(0); // Allow thread to actually switch cores
        
        ExecuteCpuid(cpuInfo, 0x1A, 0);
        if (((cpuInfo[0] >> 24) & 0xFF) == cpuidValue) {
            mask |= threadMask;
            g_logger->Log(Logger::Level::DEBUG, 
                "Core " + std::to_string(i) + " matches type");
        }
    }
    
    // Restore thread affinity
    SetThreadAffinityMask(GetCurrentThread(), 
        (1ULL << sysInfo.dwNumberOfProcessors) - 1);
    
    g_logger->Log(Logger::Level::INFO, 
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
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    
    ss << L"System CPU Information:\n"
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
    
    // Add P-core mask
    DWORD_PTR pCoreMask = GetPCoreMask();
    ss << L"P-core mask: 0x" << std::hex << pCoreMask << L"\n";
    
    // List P-cores
    ss << L"P-cores: ";
    for (DWORD i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
        if (pCoreMask & (1ULL << i)) {
            ss << i << L" ";
        }
    }
    ss << L"\n";
    
    return ss.str();
}