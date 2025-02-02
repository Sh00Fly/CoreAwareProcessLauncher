// gui_message_handler.h
#pragma once
#include "pch.h"

class GuiMessageHandler : public MessageHandler {
public:
    void ShowError(const std::wstring& message) override;
    void ShowInfo(const std::wstring& message) override;
    void ShowHelp(const std::wstring& message) override;
    void ShowQueryResult(const std::wstring& message) override;
private:
    HFONT CreateMonospaceFont(int height = 14);
};
