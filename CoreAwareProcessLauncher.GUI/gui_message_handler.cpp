// gui_message_handler.cpp
#include "pch.h"
#include "gui_message_handler.h"

void GuiMessageHandler::ShowError(const std::wstring& message) {
    MessageBoxW(NULL, message.c_str(), L"CAPL Error",
        MB_OK | MB_ICONERROR);
}

void GuiMessageHandler::ShowInfo(const std::wstring& message) {
    MessageBoxW(NULL, message.c_str(), L"CAPL Information",
        MB_OK | MB_ICONINFORMATION);
}

//void GuiMessageHandler::ShowHelp(const std::wstring& message) {
//    MessageBoxW(NULL, message.c_str(), L"CAPL Help",
//        MB_OK | MB_ICONINFORMATION);
//}

void GuiMessageHandler::ShowQueryResult(const std::wstring& message) {
    MessageBoxW(NULL, message.c_str(), L"CAPL Query Result",
        MB_OK | MB_ICONINFORMATION);
}

HFONT GuiMessageHandler::CreateMonospaceFont(int height) {
    // Try fonts in order of preference
    const wchar_t* fontNames[] = {
        L"Consolas",        // Windows Vista and later
        L"Lucida Console",  // Earlier Windows versions
        L"Courier New",     // Very widely available
        L"Courier"         // Basic fallback
    };

    HFONT hFont = NULL;
    for (const auto& fontName : fontNames) {
        hFont = CreateFont(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, fontName);
        
        if (hFont) break;
    }

    return hFont ? hFont : 
        (HFONT)GetStockObject(SYSTEM_FIXED_FONT); // Ultimate fallback
}

//------------------------------------------------------------------------
void GuiMessageHandler::ShowHelp(const std::wstring& message) {
    TASKDIALOGCONFIG config = { 0 };
    config.cbSize = sizeof(config);
    config.hwndParent = NULL;
    config.dwFlags = TDF_SIZE_TO_CONTENT;
    config.dwCommonButtons = TDCBF_OK_BUTTON;
    config.pszWindowTitle = L"Core Aware Process Launcher Help";
    //config.pszMainInstruction = L"Core Aware Process Launcher";  // Maybe a better title?
    config.pszContent = message.c_str();

    HRESULT hr = TaskDialogIndirect(&config, nullptr, nullptr, nullptr);
    if (FAILED(hr)) {
        MessageBoxW(NULL, message.c_str(), L"CAPL Help", 
            MB_OK | MB_ICONINFORMATION);
    }
}