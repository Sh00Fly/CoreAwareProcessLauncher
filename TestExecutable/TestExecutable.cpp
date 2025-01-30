#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>

int main(int argc, char* argv[]) {
    // Print process and thread info
    std::cout << "TestExecutable started" << std::endl;
    std::cout << "Process ID: " << GetCurrentProcessId() << std::endl;
    std::cout << "Thread ID: " << GetCurrentThreadId() << std::endl;
    
    // Print core affinity
    DWORD_PTR processAffinityMask, systemAffinityMask;
    if (GetProcessAffinityMask(GetCurrentProcess(), 
                              &processAffinityMask, 
                              &systemAffinityMask)) {
        std::cout << "Process Affinity Mask: 0x" 
                  << std::hex << processAffinityMask << std::endl;
        std::cout << "System Affinity Mask: 0x" 
                  << std::hex << systemAffinityMask << std::endl;
    }

    // If arguments provided, print them
    if (argc > 1) {
        std::cout << "Arguments:" << std::endl;
        for (int i = 1; i < argc; i++) {
            std::cout << i << ": " << argv[i] << std::endl;
        }
    }

    // Sleep for a bit to allow for testing
    std::cout << "Sleeping for 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::cout << "TestExecutable finishing" << std::endl;
    return 0;
}
