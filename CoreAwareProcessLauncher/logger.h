#pragma once
#include <string>
#include <fstream>
#include <memory>
#include <ctime>

class Logger {
public:
    enum class Level {
        INFO,
        WARNING,
        ERR,
        DEBUG
    };

    Logger(bool enabled = false, const std::wstring& logPath = L"");  // Constructor
    ~Logger();  // Destructor

    void Log(Level level, const std::string& message);

private:
    bool m_enabled;
    std::wstring m_logPath;
    std::ofstream m_logFile;
};

// Global logger instance
extern std::unique_ptr<class Logger> g_logger;