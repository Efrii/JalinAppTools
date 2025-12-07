#pragma once
#include <windows.h>

// UI Colors (Windows 11 Style)
#define COLOR_ACCENT        RGB(0, 120, 212)      // #0078D4
#define COLOR_ACCENT_HOVER  RGB(0, 90, 158)       // #005A9E
#define COLOR_BG_WHITE      RGB(255, 255, 255)    // #FFFFFF
#define COLOR_BG_LIGHT      RGB(243, 243, 243)    // #F3F3F3
#define COLOR_BORDER        RGB(224, 224, 224)    // #E0E0E0
#define COLOR_TEXT_DARK     RGB(51, 51, 51)       // #333333
#define COLOR_TEXT_GRAY     RGB(102, 102, 102)    // #666666

// Layout Constants
#define SIDEBAR_WIDTH       200
#define STATUSBAR_HEIGHT    25
#define PANEL_PADDING       20
#define CONTROL_SPACING     15
#define BUTTON_HEIGHT       32
#define TEXTBOX_HEIGHT      24
#define SIDEBAR_BUTTON_HEIGHT 45

// Control IDs - Main Window Components
#define IDC_SIDEBAR_PANEL       2000
#define IDC_CONTENT_PANEL       2001
#define IDC_STATUSBAR           2002

// Control IDs - Sidebar Navigation Buttons
#define IDC_SIDEBAR_EJPARSER    2010
#define IDC_SIDEBAR_TOOL2       2011
#define IDC_SIDEBAR_TOOL3       2012

// Control IDs - EJ Parser Panel (keep existing)
#define IDC_EJ_INPUT_PATH       1001
#define IDC_EJ_BROWSE_INPUT     1002
#define IDC_EJ_OUTPUT_PATH      1003
#define IDC_EJ_BROWSE_OUTPUT    1004
#define IDC_EJ_RADIO_STANDARD   1005
#define IDC_EJ_RADIO_LENGKAP    1006
#define IDC_EJ_PROCESS          1007
#define IDC_EJ_TITLE            1008
#define IDC_EJ_LABEL_INPUT      1009
#define IDC_EJ_LABEL_OUTPUT     1010
#define IDC_EJ_GROUPBOX_FORMAT  1011
#define IDC_EJ_LABEL_TID        1012
#define IDC_EJ_TID_INPUT        1013
#define IDC_EJ_LABEL_DATE       1014
#define IDC_EJ_DATE_PICKER      1015
#define IDC_EJ_OUTPUT_PREVIEW   1016
#define IDC_EJ_PARTICIPANT_ID   1019
#define IDC_EJ_LABEL_PARTICIPANT 1020

// Global Window Handles
extern HWND g_hMainWindow;
extern HWND g_hSidebarPanel;
extern HWND g_hContentPanel;
extern HWND g_hWelcomePanel;
extern HWND g_hEJParserPanel;
extern HWND g_hStatusBar;

// Current selected tool
extern int g_currentTool;

// Special tool ID for welcome screen
#define IDC_WELCOME_SCREEN      2000

// UI Creation Functions
HWND CreateSidebarPanel(HWND hParent, HINSTANCE hInst);
HWND CreateWelcomePanel(HWND hParent, HINSTANCE hInst);
HWND CreateEJParserPanel(HWND hParent, HINSTANCE hInst);
HWND CreateStatusBar(HWND hParent, HINSTANCE hInst);

// UI Update Functions
void ResizeMainWindowControls(HWND hWnd);
void ResizeWelcomePanel(HWND hPanel, int width, int height);
void ResizeEJParserControls(HWND hPanel, int width, int height);
void ShowToolPanel(int toolID);
void UpdateSidebarSelection(int toolID);
void SetStatusText(const char* text);

// UI Helper Functions
HFONT CreateUIFont(int size, BOOL bold);
void DrawModernButton(HDC hdc, RECT* rect, const char* text, BOOL isHover, BOOL isPressed);
