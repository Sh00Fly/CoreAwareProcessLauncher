// utilities.cpp
#include "pch.h"
#include "utilities.h"
#include <iostream>

namespace Utilities {
std::string ConvertToNarrowString(const std::wstring &wide) {
    if (wide.empty())
        return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                          static_cast<int>(wide.length()),
                                          nullptr, 0, nullptr, nullptr);

    std::string utf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                        static_cast<int>(wide.length()), &utf8[0], size_needed,
                        nullptr, nullptr);

    return utf8;
}

std::wstring ConvertToWideString(const std::string &utf8) {
    if (utf8.empty())
        return std::wstring();

    int size_needed = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.length()), nullptr, 0);

    std::wstring wide(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                        static_cast<int>(utf8.length()), &wide[0], size_needed);

    return wide;
}

bool PathExists(const std::wstring &path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
}

bool IsDirectory(const std::wstring &path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES) &&
           (attrs & FILE_ATTRIBUTE_DIRECTORY);
}
} // namespace Utilities

// Logger
std::unique_ptr<ApplicationLogger> g_logger;

ApplicationLogger::ApplicationLogger(bool enabled, const std::wstring &logPath)
    : m_enabled(enabled), m_logPath(logPath) {
    if (m_enabled && !logPath.empty()) {
        m_logFile.open(logPath, std::ios::out | std::ios::app);
    }
}

ApplicationLogger::~ApplicationLogger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void ApplicationLogger::Log(Level level, const std::string &message) {
    if (!m_enabled)
        return;

    std::string levelStr;
    switch (level) {
    case Level::INFO:
        levelStr = "INFO";
        break;
    case Level::WARNING:
        levelStr = "WARNING";
        break;
    case Level::ERR:
        levelStr = "ERROR";
        break;
    case Level::DEBUG:
        levelStr = "DEBUG";
        break;
    }

    std::time_t now = std::time(nullptr);
    char timestamp[26];
    ctime_s(timestamp, sizeof(timestamp), &now);
    timestamp[24] = '\0'; // Remove newline

    std::string logMessage =
        std::string(timestamp) + " [" + levelStr + "] " + message;

    if (m_logFile.is_open()) {
        m_logFile << logMessage << std::endl;
    }
}

// MessageHandler global instance
std::unique_ptr<MessageHandler> g_messageHandler;
