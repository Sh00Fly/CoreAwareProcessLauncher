#include "pch.h"
#include "logger.h"
#include <iostream>

std::unique_ptr<ApplicationLogger> g_logger;

ApplicationLogger::ApplicationLogger(bool enabled, const std::wstring& logPath) 
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

void ApplicationLogger::Log(Level level, const std::string& message) {
    if (!m_enabled) return;

    std::string levelStr;
    switch (level) {
        case Level::INFO:    levelStr = "INFO"; break;
        case Level::WARNING: levelStr = "WARNING"; break;
        case Level::ERR:     levelStr = "ERROR"; break;
        case Level::DEBUG:   levelStr = "DEBUG"; break;
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
    
    if (level == Level::WARNING || level == Level::ERR) {
        std::cerr << logMessage << std::endl;
    }
}