#include "main_window_ui.h"
#include "ej_dialog.h"
#include "ej_parser.h"
#include "Resource.h"
#include <commctrl.h>
#include <string>

#pragma comment(lib, "comctl32.lib")

// Global Window Handles
HWND g_hMainWindow = NULL;
HWND g_hSidebarPanel = NULL;
HWND g_hContentPanel = NULL;
HWND g_hWelcomePanel = NULL;
HWND g_hEJParserPanel = NULL;
HWND g_hStatusBar = NULL;

// Current selected tool
int g_currentTool = IDC_WELCOME_SCREEN;

// Sidebar button state tracking
BOOL g_sidebarButtonHover[4] = { FALSE, FALSE, FALSE, FALSE };

// Font handles
HFONT g_hTitleFont = NULL;
HFONT g_hNormalFont = NULL;
HFONT g_hButtonFont = NULL;

// Forward declarations for window procedures
LRESULT CALLBACK SidebarPanelProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ContentPanelProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Create Segoe UI font
HFONT CreateUIFont(int size, BOOL bold) {
    return CreateFont(
        size, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        TEXT("Segoe UI")
    );
}

// Register custom window classes
void RegisterCustomClasses(HINSTANCE hInst) {
    WNDCLASSEX wc = { 0 };
    
    // Sidebar panel class
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = SidebarPanelProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COLOR_BG_LIGHT);
    wc.lpszClassName = TEXT("SidebarPanelClass");
    RegisterClassEx(&wc);
    
    // Content panel class
    wc.lpfnWndProc = ContentPanelProc;
    wc.hbrBackground = CreateSolidBrush(COLOR_BG_WHITE);
    wc.lpszClassName = TEXT("ContentPanelClass");
    RegisterClassEx(&wc);
}

// Create sidebar panel with navigation buttons
HWND CreateSidebarPanel(HWND hParent, HINSTANCE hInst) {
    RegisterCustomClasses(hInst);
    
    HWND hSidebar = CreateWindowEx(
        0,
        TEXT("SidebarPanelClass"),
        TEXT(""),
        WS_CHILD | WS_VISIBLE,
        0, 0, SIDEBAR_WIDTH, 500,
        hParent,
        (HMENU)IDC_SIDEBAR_PANEL,
        hInst,
        NULL
    );
    
    return hSidebar;
}

// Sidebar panel window procedure
LRESULT CALLBACK SidebarPanelProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        
        RECT rect;
        GetClientRect(hWnd, &rect);
        
        // Draw sidebar background
        HBRUSH bgBrush = CreateSolidBrush(COLOR_BG_LIGHT);
        FillRect(hdc, &rect, bgBrush);
        DeleteObject(bgBrush);
        
        // Draw sidebar buttons
        int yPos = 10;
        const char* buttonLabels[] = { "Beranda", "Parser EJ", "Alat 2", "Alat 3" };
        int buttonIDs[] = { IDC_WELCOME_SCREEN, IDC_SIDEBAR_EJPARSER, IDC_SIDEBAR_TOOL2, IDC_SIDEBAR_TOOL3 };
        
        for (int i = 0; i < 4; i++) {
            RECT btnRect = { 5, yPos, SIDEBAR_WIDTH - 5, yPos + SIDEBAR_BUTTON_HEIGHT };
            
            // Draw button background
            BOOL isSelected = (buttonIDs[i] == g_currentTool);
            BOOL isEnabled = (i <= 1); // Home and EJ Parser enabled
            
            HBRUSH btnBrush;
            if (isSelected) {
                btnBrush = CreateSolidBrush(COLOR_ACCENT);
            } else if (g_sidebarButtonHover[i] && isEnabled) {
                btnBrush = CreateSolidBrush(RGB(230, 230, 230));
            } else {
                btnBrush = CreateSolidBrush(COLOR_BG_LIGHT);
            }
            
            FillRect(hdc, &btnRect, btnBrush);
            DeleteObject(btnBrush);
            
            // Draw button text
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, isSelected ? COLOR_BG_WHITE : (isEnabled ? COLOR_TEXT_DARK : COLOR_TEXT_GRAY));
            
            HFONT hFont = CreateUIFont(14, FALSE);
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            
            DrawTextA(hdc, buttonLabels[i], -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            
            yPos += SIDEBAR_BUTTON_HEIGHT + 5;
        }
        
        // Draw border on right side
        HPEN borderPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        MoveToEx(hdc, rect.right - 1, 0, NULL);
        LineTo(hdc, rect.right - 1, rect.bottom);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);
        
        EndPaint(hWnd, &ps);
        return 0;
    }
    
    case WM_LBUTTONDOWN: {
        int y = HIWORD(lParam);
        int buttonIDs[] = { IDC_WELCOME_SCREEN, IDC_SIDEBAR_EJPARSER, IDC_SIDEBAR_TOOL2, IDC_SIDEBAR_TOOL3 };
        int yPos = 10;
        for (int i = 0; i < 4; i++) {
            if (y >= yPos && y < yPos + SIDEBAR_BUTTON_HEIGHT && i <= 1) {
                ShowToolPanel(buttonIDs[i]);
                break;
            }
            yPos += SIDEBAR_BUTTON_HEIGHT + 5;
        }
        return 0;
    }
    
    case WM_MOUSEMOVE: {
        int y = HIWORD(lParam);
        int yPos = 10;
        BOOL needRedraw = FALSE;
        for (int i = 0; i < 4; i++) {
            BOOL wasHover = g_sidebarButtonHover[i];
            g_sidebarButtonHover[i] = (y >= yPos && y < yPos + SIDEBAR_BUTTON_HEIGHT);
            if (wasHover != g_sidebarButtonHover[i]) needRedraw = TRUE;
            yPos += SIDEBAR_BUTTON_HEIGHT + 5;
        }
        if (needRedraw) InvalidateRect(hWnd, NULL, FALSE);
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hWnd;
        TrackMouseEvent(&tme);
        return 0;
    }
    
    case WM_MOUSELEAVE: {
        for (int i = 0; i < 4; i++) g_sidebarButtonHover[i] = FALSE;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Content panel window procedure
LRESULT CALLBACK ContentPanelProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        // Forward command messages to main window so buttons work
        return SendMessage(GetParent(GetParent(hWnd)), WM_COMMAND, wParam, lParam);
    
    case WM_CTLCOLORSTATIC: {
        // Fix gray background on static text controls
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, COLOR_TEXT_DARK);
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
        
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        
        RECT rect;
        GetClientRect(hWnd, &rect);
        
        // Draw white background
        HBRUSH bgBrush = CreateSolidBrush(COLOR_BG_WHITE);
        FillRect(hdc, &rect, bgBrush);
        DeleteObject(bgBrush);
        
        EndPaint(hWnd, &ps);
        return 0;
    }
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Create Welcome/Home panel
HWND CreateWelcomePanel(HWND hParent, HINSTANCE hInst) {
    HWND hPanel = CreateWindowEx(
        0,
        TEXT("ContentPanelClass"),
        TEXT(""),
        WS_CHILD | WS_VISIBLE,
        0, 0, 600, 500,
        hParent,
        NULL,
        hInst,
        NULL
    );
    
    int yPos = 60;
    int centerX = 350;
    
    // Welcome Title
    HWND hTitle = CreateWindowA(
        "STATIC",
        "Selamat Datang di JalinTools",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        50, yPos, 700, 35,
        hPanel,
        NULL,
        hInst,
        NULL
    );
    SendMessage(hTitle, WM_SETFONT, (WPARAM)CreateUIFont(24, TRUE), TRUE);
    
    yPos += 50;
    
    // Subtitle
    HWND hSubtitle = CreateWindowA(
        "STATIC",
        "Pilih alat dari menu samping untuk memulai",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        50, yPos, 700, 25,
        hPanel,
        NULL,
        hInst,
        NULL
    );
    SendMessage(hSubtitle, WM_SETFONT, (WPARAM)CreateUIFont(14, FALSE), TRUE);
    
    yPos += 60;
    
    // Available Tools Section
    HWND hToolsLabel = CreateWindowA(
        "STATIC",
        "Alat yang Tersedia:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        100, yPos, 200, 25,
        hPanel,
        NULL,
        hInst,
        NULL
    );
    SendMessage(hToolsLabel, WM_SETFONT, (WPARAM)CreateUIFont(16, TRUE), TRUE);
    
    yPos += 40;
    
    // Tool 1: EJ Parser
    HWND hTool1Name = CreateWindowA(
        "STATIC",
        "1. Parser EJ",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        120, yPos, 300, 25,
        hPanel,
        NULL,
        hInst,
        NULL
    );
    SendMessage(hTool1Name, WM_SETFONT, (WPARAM)CreateUIFont(14, TRUE), TRUE);
    
    yPos += 30;
    
    HWND hTool1Desc = CreateWindowA(
        "STATIC",
        "Memproses dan memformat file log Electronic Journal (EJ)\r\ndari EFTS menjadi laporan yang mudah dibaca.",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        120, yPos, 600, 40,
        hPanel,
        NULL,
        hInst,
        NULL
    );
    SendMessage(hTool1Desc, WM_SETFONT, (WPARAM)CreateUIFont(12, FALSE), TRUE);
    
    yPos += 60;
    
    // Tool 2: Coming Soon
    HWND hTool2Name = CreateWindowA(
        "STATIC",
        "2. Alat Tambahan (Segera Hadir)",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        120, yPos, 300, 25,
        hPanel,
        NULL,
        hInst,
        NULL
    );
    SendMessage(hTool2Name, WM_SETFONT, (WPARAM)CreateUIFont(14, FALSE), TRUE);
    
    yPos += 30;
    
    HWND hTool2Desc = CreateWindowA(
        "STATIC",
        "Lebih banyak alat akan ditambahkan di rilis mendatang.",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        120, yPos, 600, 25,
        hPanel,
        NULL,
        hInst,
        NULL
    );
    SendMessage(hTool2Desc, WM_SETFONT, (WPARAM)CreateUIFont(12, FALSE), TRUE);
    
    yPos += 80;
    
    // Instructions
    HWND hInstructions = CreateWindowA(
        "STATIC",
        "Klik 'Parser EJ' di menu samping untuk mulai memproses file log.",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        50, yPos, 700, 25,
        hPanel,
        NULL,
        hInst,
        NULL
    );
    SendMessage(hInstructions, WM_SETFONT, (WPARAM)CreateUIFont(13, FALSE), TRUE);
    
    return hPanel;
}

// Create EJ Parser panel
HWND CreateEJParserPanel(HWND hParent, HINSTANCE hInst) {
    HWND hPanel = CreateWindowEx(
        0,
        TEXT("ContentPanelClass"),
        TEXT(""),
        WS_CHILD | WS_VISIBLE,
        0, 0, 600, 800,
        hParent,
        NULL,
        hInst,
        NULL
    );
    
    int yPos = 30;
    int cardPadding = 30;
    int cardWidth = 650;
    
    // Title
    HWND hTitle = CreateWindowA(
        "STATIC",
        "Parser Electronic Journal",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        cardPadding, yPos, 600, 35,
        hPanel,
        (HMENU)IDC_EJ_TITLE,
        hInst,
        NULL
    );
    HFONT hTitleFont = CreateUIFont(26, TRUE);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
    
    yPos += 50;
    
    // Subtitle
    HWND hSubtitle = CreateWindowA(
        "STATIC",
        "Memproses dan memformat file log EFTS menjadi laporan EJ yang mudah dibaca",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        cardPadding, yPos, 600, 25,
        hPanel,
        NULL,
        hInst,
        NULL
    );
    SendMessage(hSubtitle, WM_SETFONT, (WPARAM)CreateUIFont(14, FALSE), TRUE);
    
    yPos += 40;
    
    // ===== GROUPBOX: FILE INPUT =====
    int gbFileHeight = 85;
    HWND hGroupFile = CreateWindowA("BUTTON", "File Input",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        cardPadding, yPos, cardWidth, gbFileHeight,
        hPanel, NULL, hInst, NULL);
    SendMessage(hGroupFile, WM_SETFONT, (WPARAM)CreateUIFont(14, TRUE), TRUE);
    
    int fileY = yPos + 30;
    int labelX = cardPadding + 20;
    int inputX = cardPadding + 130;
    
    // File Input Label
    HWND hInputLabel = CreateWindowA("STATIC", "Input File:", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        labelX, fileY, 100, 25, hPanel, (HMENU)IDC_EJ_LABEL_INPUT, hInst, NULL);
    SendMessage(hInputLabel, WM_SETFONT, (WPARAM)CreateUIFont(13, FALSE), TRUE);
    
    // Input File Textbox
    HWND hInputPath = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_READONLY,
        inputX, fileY - 2, 360, 28,
        hPanel, (HMENU)IDC_EJ_INPUT_PATH, hInst, NULL);
    SendMessage(hInputPath, WM_SETFONT, (WPARAM)CreateUIFont(12, FALSE), TRUE);
    
    // Browse Input Button
    HWND hBrowseIn = CreateWindowA("BUTTON", "Cari...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        inputX + 370, fileY - 2, 85, 28,
        hPanel, (HMENU)IDC_EJ_BROWSE_INPUT, hInst, NULL);
    SendMessage(hBrowseIn, WM_SETFONT, (WPARAM)CreateUIFont(12, FALSE), TRUE);
    
    yPos += gbFileHeight + 25;
    
    // ===== GROUPBOX: PENGATURAN OUTPUT =====
    int gbOutputHeight = 160;
    HWND hGroupOutput = CreateWindowA("BUTTON", "Pengaturan Output",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        cardPadding, yPos, cardWidth, gbOutputHeight,
        hPanel, NULL, hInst, NULL);
    SendMessage(hGroupOutput, WM_SETFONT, (WPARAM)CreateUIFont(14, TRUE), TRUE);
    
    int outY = yPos + 35;
    
    // TID Input
    HWND hTIDLabel = CreateWindowA("STATIC", "TID (Terminal ID):", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        labelX, outY, 130, 25, hPanel, (HMENU)IDC_EJ_LABEL_TID, hInst, NULL);
    SendMessage(hTIDLabel, WM_SETFONT, (WPARAM)CreateUIFont(13, FALSE), TRUE);
    
    HWND hTIDInput = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_UPPERCASE,
        inputX + 30, outY - 2, 240, 28,
        hPanel, (HMENU)IDC_EJ_TID_INPUT, hInst, NULL);
    SendMessage(hTIDInput, WM_SETFONT, (WPARAM)CreateUIFont(12, FALSE), TRUE);
    SendMessageA(hTIDInput, EM_SETCUEBANNER, TRUE, (LPARAM)L"e.g. T0800004");
    
    outY += 45;
    
    // Date Input with DateTimePicker
    HWND hDateLabel = CreateWindowA("STATIC", "Tanggal:", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        labelX, outY, 130, 25, hPanel, (HMENU)IDC_EJ_LABEL_DATE, hInst, NULL);
    SendMessage(hDateLabel, WM_SETFONT, (WPARAM)CreateUIFont(13, FALSE), TRUE);
    
    // Initialize common controls for DateTimePicker
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);
    
    HWND hDatePicker = CreateWindowExA(0, DATETIMEPICK_CLASSA, "",
        WS_CHILD | WS_VISIBLE | DTS_SHORTDATEFORMAT,
        inputX + 30, outY - 2, 240, 28,
        hPanel, (HMENU)IDC_EJ_DATE_PICKER, hInst, NULL);
    SendMessage(hDatePicker, WM_SETFONT, (WPARAM)CreateUIFont(12, FALSE), TRUE);
    
    outY += 45;
    
    // Participant ID Dropdown
    HWND hParticipantLabel = CreateWindowA("STATIC", "Participant ID:", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        labelX, outY, 130, 25, hPanel, (HMENU)IDC_EJ_LABEL_PARTICIPANT, hInst, NULL);
    SendMessage(hParticipantLabel, WM_SETFONT, (WPARAM)CreateUIFont(13, FALSE), TRUE);
    
    HWND hParticipantCombo = CreateWindowExA(0, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        inputX + 30, outY - 2, 240, 100,
        hPanel, (HMENU)IDC_EJ_PARTICIPANT_ID, hInst, NULL);
    SendMessage(hParticipantCombo, WM_SETFONT, (WPARAM)CreateUIFont(12, FALSE), TRUE);
    
    // Add dropdown options
    SendMessageA(hParticipantCombo, CB_ADDSTRING, 0, (LPARAM)"200");
    SendMessageA(hParticipantCombo, CB_ADDSTRING, 0, (LPARAM)"008");
    SendMessageA(hParticipantCombo, CB_ADDSTRING, 0, (LPARAM)"002");
    SendMessageA(hParticipantCombo, CB_ADDSTRING, 0, (LPARAM)"009");
    SendMessageA(hParticipantCombo, CB_SETCURSEL, 0, 0); // Default: 200
    
    yPos += gbOutputHeight + 25;
    
    // Output preview
    HWND hOutputPreview = CreateWindowA("STATIC", "", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        cardPadding, yPos, 600, 25, hPanel, (HMENU)IDC_EJ_OUTPUT_PREVIEW, hInst, NULL);
    SendMessage(hOutputPreview, WM_SETFONT, (WPARAM)CreateUIFont(12, FALSE), TRUE);
    
    yPos += 45;
    
    // Format Options GroupBox
    HWND hGroupBox = CreateWindowA("BUTTON", "Opsi Format",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        cardPadding, yPos, cardWidth, 80,
        hPanel, (HMENU)IDC_EJ_GROUPBOX_FORMAT, hInst, NULL);
    SendMessage(hGroupBox, WM_SETFONT, (WPARAM)CreateUIFont(15, TRUE), TRUE);
    
    // Radio buttons
    HWND hRadioStd = CreateWindowA("BUTTON", "Standard",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        cardPadding + 25, yPos + 32, 150, 26,
        hPanel, (HMENU)IDC_EJ_RADIO_STANDARD, hInst, NULL);
    SendMessage(hRadioStd, WM_SETFONT, (WPARAM)CreateUIFont(14, FALSE), TRUE);
    
    HWND hRadioLengkap = CreateWindowA("BUTTON", "Lengkap (Comprehensive)",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        cardPadding + 200, yPos + 32, 250, 26,
        hPanel, (HMENU)IDC_EJ_RADIO_LENGKAP, hInst, NULL);
    SendMessage(hRadioLengkap, WM_SETFONT, (WPARAM)CreateUIFont(14, FALSE), TRUE);
    
    // Set Lengkap as default
    SendMessage(GetDlgItem(hPanel, IDC_EJ_RADIO_LENGKAP), BM_SETCHECK, BST_CHECKED, 0);
    
    yPos += 95;
    
    // Process Button
    HWND hProcessBtn = CreateWindowA("BUTTON", "Proses File EJ",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
        cardPadding + 220, yPos, 220, 48,
        hPanel, (HMENU)IDC_EJ_PROCESS, hInst, NULL);
    SendMessage(hProcessBtn, WM_SETFONT, (WPARAM)CreateUIFont(16, TRUE), TRUE);
    
    yPos += 60;
    
    // Help text
    HWND hHelp = CreateWindowA("STATIC", 
        "Output akan disimpan ke: [Folder Aplikasi]\\EJParse\\EJ_TID_DDMMYYYY.txt",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        cardPadding, yPos, 600, 25,
        hPanel,
        NULL,
        hInst,
        NULL
    );
    SendMessage(hHelp, WM_SETFONT, (WPARAM)CreateUIFont(12, FALSE), TRUE);
    
    return hPanel;
}

// Create status bar
HWND CreateStatusBar(HWND hParent, HINSTANCE hInst) {
    HWND hStatus = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        hParent,
        (HMENU)IDC_STATUSBAR,
        hInst,
        NULL
    );
    
    SetStatusText("Ready");
    return hStatus;
}

// Helper function to resize EJ Parser panel controls responsively
void ResizeEJParserControls(HWND hPanel, int width, int height) {
    if (!hPanel) return;
    
    int cardPadding = 30;
    int availableWidth = width - (cardPadding * 2);
    int textboxWidth = availableWidth - 110;
    int yPos = 30;
    
    if (textboxWidth < 300) textboxWidth = 300;
    if (availableWidth < 400) return;
    
    // Resize title
    HWND hTitle = GetDlgItem(hPanel, IDC_EJ_TITLE);
    if (hTitle) SetWindowPos(hTitle, NULL, cardPadding, yPos, availableWidth, 30, SWP_NOZORDER);
    yPos += 45;
    
    // Subtitle
    HWND hChild = GetWindow(hPanel, GW_CHILD);
    while (hChild) {
        char text[256];
        GetWindowTextA(hChild, text, 256);
        if (strstr(text, "Process and format")) {
            SetWindowPos(hChild, NULL, cardPadding, yPos, availableWidth, 20, SWP_NOZORDER);
            break;
        }
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }
    yPos += 40;
    
    // Input File Label
    HWND hInputLabel = GetDlgItem(hPanel, IDC_EJ_LABEL_INPUT);
    if (hInputLabel) SetWindowPos(hInputLabel, NULL, cardPadding, yPos, 120, 22, SWP_NOZORDER);
    yPos += 25;
    
    // Input File Textbox
    HWND hInputPath = GetDlgItem(hPanel, IDC_EJ_INPUT_PATH);
    if (hInputPath) SetWindowPos(hInputPath, NULL, cardPadding, yPos, textboxWidth, 28, SWP_NOZORDER);
    
    // Browse Input Button
    HWND hBrowseIn = GetDlgItem(hPanel, IDC_EJ_BROWSE_INPUT);
    if (hBrowseIn) SetWindowPos(hBrowseIn, NULL, cardPadding + textboxWidth + 10, yPos, 100, 28, SWP_NOZORDER);
    yPos += 45;
    
    // Output Settings Header
    HWND hChild2 = GetWindow(hPanel, GW_CHILD);
    while (hChild2) {
        char text2[256];
        GetWindowTextA(hChild2, text2, 256);
        if (strstr(text2, "Output Settings")) {
            SetWindowPos(hChild2, NULL, cardPadding, yPos, 200, 25, SWP_NOZORDER);
            break;
        }
        hChild2 = GetWindow(hChild2, GW_HWNDNEXT);
    }
    yPos += 35;
    
    // TID Label
    HWND hTIDLabel = GetDlgItem(hPanel, IDC_EJ_LABEL_TID);
    if (hTIDLabel) SetWindowPos(hTIDLabel, NULL, cardPadding, yPos, 150, 25, SWP_NOZORDER);
    
    // TID Input
    HWND hTIDInput = GetDlgItem(hPanel, IDC_EJ_TID_INPUT);
    int tidWidth = min(300, availableWidth - 170);
    if (hTIDInput) SetWindowPos(hTIDInput, NULL, cardPadding + 160, yPos, tidWidth, 32, SWP_NOZORDER);
    yPos += 40;
    
    // Date Label
    HWND hDateLabel = GetDlgItem(hPanel, IDC_EJ_LABEL_DATE);
    if (hDateLabel) SetWindowPos(hDateLabel, NULL, cardPadding, yPos, 150, 25, SWP_NOZORDER);
    
    // Date Picker
    HWND hDatePicker = GetDlgItem(hPanel, IDC_EJ_DATE_PICKER);
    int dateWidth = min(300, availableWidth - 170);
    if (hDatePicker) SetWindowPos(hDatePicker, NULL, cardPadding + 160, yPos, dateWidth, 32, SWP_NOZORDER);
    yPos += 40;
    
    // Output Preview
    HWND hOutputPreview = GetDlgItem(hPanel, IDC_EJ_OUTPUT_PREVIEW);
    if (hOutputPreview) SetWindowPos(hOutputPreview, NULL, cardPadding, yPos, availableWidth, 25, SWP_NOZORDER);
    yPos += 45;
    
    // Format Options GroupBox
    int groupBoxWidth = min(650, availableWidth);
    HWND hGroupBox = GetDlgItem(hPanel, IDC_EJ_GROUPBOX_FORMAT);
    if (hGroupBox) SetWindowPos(hGroupBox, NULL, cardPadding, yPos, groupBoxWidth, 75, SWP_NOZORDER);
    
    // Radio buttons
    HWND hRadioStd = GetDlgItem(hPanel, IDC_EJ_RADIO_STANDARD);
    if (hRadioStd) SetWindowPos(hRadioStd, NULL, cardPadding + 25, yPos + 30, 150, 24, SWP_NOZORDER);
    
    HWND hRadioLengkap = GetDlgItem(hPanel, IDC_EJ_RADIO_LENGKAP);
    if (hRadioLengkap) SetWindowPos(hRadioLengkap, NULL, cardPadding + 200, yPos + 30, 220, 24, SWP_NOZORDER);
    yPos += 90;
    
    // Process Button
    int btnWidth = 200;
    int btnX = cardPadding + (availableWidth - btnWidth) / 2;
    if (btnX < cardPadding) btnX = cardPadding;
    
    HWND hProcessBtn = GetDlgItem(hPanel, IDC_EJ_PROCESS);
    if (hProcessBtn) SetWindowPos(hProcessBtn, NULL, btnX, yPos, btnWidth, 42, SWP_NOZORDER);
}

// Helper function to resize Welcome panel controls responsively
void ResizeWelcomePanel(HWND hPanel, int width, int height) {
    if (!hPanel) return;
    
    int padding = 50;
    int availableWidth = width - (padding * 2);
    if (availableWidth < 400) {
        padding = 20;
        availableWidth = width - (padding * 2);
    }
    
    // Center all elements horizontally
    int yPos = 60;
    
    // Resize all static text children to be centered and responsive
    HWND hChild = GetWindow(hPanel, GW_CHILD);
    int childIndex = 0;
    
    while (hChild) {
        char className[256];
        GetClassNameA(hChild, className, 256);
        
        if (strcmp(className, "Static") == 0) {
            char text[512];
            GetWindowTextA(hChild, text, 512);
            
            // Title - "Selamat Datang"
            if (strstr(text, "Selamat Datang")) {
                SetWindowPos(hChild, NULL, padding, yPos, availableWidth, 40, SWP_NOZORDER);
                yPos = 60 + 50;
            }
            // Subtitle - "Pilih alat"
            else if (strstr(text, "Pilih alat")) {
                SetWindowPos(hChild, NULL, padding, yPos, availableWidth, 30, SWP_NOZORDER);
                yPos = 60 + 50 + 60;
            }
            // "Alat yang Tersedia"
            else if (strstr(text, "Alat yang Tersedia")) {
                SetWindowPos(hChild, NULL, padding + 50, yPos, availableWidth - 100, 30, SWP_NOZORDER);
                yPos = 60 + 50 + 60 + 40;
            }
            // "1. Parser EJ"
            else if (strstr(text, "1. Parser EJ")) {
                SetWindowPos(hChild, NULL, padding + 70, yPos, availableWidth - 120, 30, SWP_NOZORDER);
                yPos = 60 + 50 + 60 + 40 + 30;
            }
            // Description of Parser EJ
            else if (strstr(text, "Memproses dan memformat")) {
                SetWindowPos(hChild, NULL, padding + 70, yPos, availableWidth - 120, 50, SWP_NOZORDER);
                yPos = 60 + 50 + 60 + 40 + 30 + 60;
            }
            // "2. Alat Tambahan"
            else if (strstr(text, "2. Alat Tambahan")) {
                SetWindowPos(hChild, NULL, padding + 70, yPos, availableWidth - 120, 30, SWP_NOZORDER);
                yPos = 60 + 50 + 60 + 40 + 30 + 60 + 30;
            }
            // Description of Alat Tambahan
            else if (strstr(text, "Lebih banyak alat")) {
                SetWindowPos(hChild, NULL, padding + 70, yPos, availableWidth - 120, 30, SWP_NOZORDER);
                yPos = 60 + 50 + 60 + 40 + 30 + 60 + 30 + 30;
            }
            // Instructions - "Klik 'Parser EJ'"
            else if (strstr(text, "Klik")) {
                yPos = 60 + 50 + 60 + 40 + 30 + 60 + 30 + 30 + 80;
                SetWindowPos(hChild, NULL, padding, yPos, availableWidth, 30, SWP_NOZORDER);
            }
        }
        
        hChild = GetWindow(hChild, GW_HWNDNEXT);
        childIndex++;
    }
}

// Resize all main window controls
void ResizeMainWindowControls(HWND hWnd) {
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    
    // Resize status bar
    if (g_hStatusBar) {
        SendMessage(g_hStatusBar, WM_SIZE, 0, 0);
        RECT statusRect;
        GetClientRect(g_hStatusBar, &statusRect);
        height -= (statusRect.bottom - statusRect.top);
    }
    
    // Resize sidebar
    if (g_hSidebarPanel) {
        SetWindowPos(g_hSidebarPanel, NULL, 0, 0, SIDEBAR_WIDTH, height, SWP_NOZORDER);
    }
    
    // Resize content panel
    if (g_hContentPanel) {
        SetWindowPos(g_hContentPanel, NULL, SIDEBAR_WIDTH, 0,
            width - SIDEBAR_WIDTH, height, SWP_NOZORDER);
    }
    
    int contentWidth = width - SIDEBAR_WIDTH;
    int contentHeight = height;
    
    // Resize all tool panels
    if (g_hWelcomePanel) {
        SetWindowPos(g_hWelcomePanel, NULL, 0, 0, contentWidth, contentHeight, SWP_NOZORDER);
        ResizeWelcomePanel(g_hWelcomePanel, contentWidth, contentHeight);
    }
    
    if (g_hEJParserPanel) {
        SetWindowPos(g_hEJParserPanel, NULL, 0, 0, contentWidth, contentHeight, SWP_NOZORDER);
        ResizeEJParserControls(g_hEJParserPanel, contentWidth, contentHeight);
    }
}

// Show selected tool panel
void ShowToolPanel(int toolID) {
    g_currentTool = toolID;
    
    // Hide all panels
    if (g_hWelcomePanel) ShowWindow(g_hWelcomePanel, SW_HIDE);
    if (g_hEJParserPanel) ShowWindow(g_hEJParserPanel, SW_HIDE);
    
    // Show selected panel
    switch (toolID) {
    case IDC_WELCOME_SCREEN:
        if (g_hWelcomePanel) ShowWindow(g_hWelcomePanel, SW_SHOW);
        SetStatusText("Selamat Datang di JalinTools - Pilih alat dari menu samping");
        break;
    case IDC_SIDEBAR_EJPARSER:
        if (g_hEJParserPanel) ShowWindow(g_hEJParserPanel, SW_SHOW);
        SetStatusText("Parser EJ - Siap memproses file Electronic Journal");
        break;
    case IDC_SIDEBAR_TOOL2:
        SetStatusText("Alat 2 - Segera Hadir");
        break;
    case IDC_SIDEBAR_TOOL3:
        SetStatusText("Alat 3 - Segera Hadir");
        break;
    }
    
    // Redraw sidebar
    if (g_hSidebarPanel) {
        InvalidateRect(g_hSidebarPanel, NULL, TRUE);
    }
}

// Set status bar text
void SetStatusText(const char* text) {
    if (g_hStatusBar) {
        SendMessageA(g_hStatusBar, SB_SETTEXTA, 0, (LPARAM)text);
    }
}
