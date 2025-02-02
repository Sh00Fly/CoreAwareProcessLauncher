// console_message_handler.cpp
#include "pch.h"
#include "console_message_handler.h"

void ConsoleMessageHandler::ShowError(const std::wstring& message) {
    std::wcerr << L"Error: " << message << std::endl;
}

void ConsoleMessageHandler::ShowInfo(const std::wstring& message) {
    std::wcout << message << std::endl;
}

void ConsoleMessageHandler::ShowHelp(const std::wstring& message) {
    std::wcout << message << std::endl;
}

void ConsoleMessageHandler::ShowQueryResult(const std::wstring& message) {
    std::wcout << message << std::endl;
}