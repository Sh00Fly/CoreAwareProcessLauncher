#pragma once
#include <string>
#include <fstream>
#include <memory>
#include <ctime>

class ApplicationLogger  {
public:
    enum class Level {
        INFO,
        WARNING,
        ERR,
        DEBUG
    };

    ApplicationLogger (bool enabled = false, const std::wstring& logPath = L"");  // Constructor
    ~ApplicationLogger ();  // Destructor

    void Log(Level level, const std::string& message);

private:
    bool m_enabled;
    std::wstring m_logPath;
    std::ofstream m_logFile;
};

// Global logger instance
extern std::unique_ptr<ApplicationLogger> g_logger;