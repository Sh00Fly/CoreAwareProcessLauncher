#pragma once

// Windows Header Files
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h> 
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
// Minimum Windows version for TaskDialog
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// C++ Standard Library
#include <string>
#include <memory>
#include <stdexcept>
#include <format>
#include <iostream>

// Project Headers
#include "options.h"
#include "utilities.h"
#include "cpu.h"
#include "process.h"
