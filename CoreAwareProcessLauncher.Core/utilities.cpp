// utilities.cpp
#include "pch.h"
#include "utilities.h"
#include <windows.h>

// Helper function to convert wide string to narrow string
std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
        static_cast<int>(wide.length()), nullptr, 0, nullptr, nullptr);

    std::string utf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
        static_cast<int>(wide.length()),
        &utf8[0], size_needed, nullptr, nullptr);

    return utf8;
}