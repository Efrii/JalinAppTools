#pragma once
#include <windows.h>

// Panel handler functions for EJ Parser
void HandleBrowseInput(HWND hPanel);
void HandleProcessEJ(HWND hPanel);

// Legacy dialog function (deprecated - use panel handlers)
void ShowEJParserDialog(HWND hWndParent);
