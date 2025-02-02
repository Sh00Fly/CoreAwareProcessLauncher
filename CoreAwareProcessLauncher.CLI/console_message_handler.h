// console_message_handler.h
#pragma once
#include "pch.h"

class ConsoleMessageHandler : public MessageHandler {
public:
    void ShowError(const std::wstring& message) override;
    void ShowInfo(const std::wstring& message) override;
    void ShowHelp(const std::wstring& message) override;
    void ShowQueryResult(const std::wstring& message) override;
};
