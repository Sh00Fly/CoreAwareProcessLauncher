// utilities.h
#pragma once
#include <string>
#include <fstream>
#include <memory>
#include <ctime>

namespace Utilities {
// --------------------------------Defaults----------------------------------
const std::wstring DEFAULT_LOG_PATH = L"capl.log";
// --------------------------------Functions---------------------------------
std::wstring ConvertToWideString(const std::string &utf8);
std::string ConvertToNarrowString(const std::wstring &wide);
bool PathExists(const std::wstring &path);
bool IsDirectory(const std::wstring &path);
} // namespace Utilities
// --------------------------ApplicationLogger ---------------------------
class ApplicationLogger {
  public:
    enum class Level { INFO, WARNING, ERR, DEBUG };

    ApplicationLogger(bool enabled = false,
                      const std::wstring &logPath = L""); // Constructor
    ~ApplicationLogger();                                 // Destructor

    void Log(Level level, const std::string &message);

  private:
    bool m_enabled;
    std::wstring m_logPath;
    std::ofstream m_logFile;
};

// Global logger instance
extern std::unique_ptr<ApplicationLogger> g_logger;

// --------------------------MessageHandler---------------------------
class MessageHandler {
  public:
    virtual ~MessageHandler() = default;
    virtual void ShowError(const std::wstring &message) = 0;
    virtual void ShowInfo(const std::wstring &message) = 0;
    virtual void ShowHelp(const std::wstring &message) = 0;
    virtual void ShowQueryResult(const std::wstring &message) = 0;
};

// Global message handler instance (similar to g_logger)
extern std::unique_ptr<MessageHandler> g_messageHandler;
