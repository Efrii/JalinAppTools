#include "ej_dialog.h"
#include "ej_parser.h"
#include "main_window_ui.h"
#include "Resource.h"
#include <windows.h>
#include <commdlg.h>
#include <CommCtrl.h>
#include <string>
#include <sstream>

// Global variables for file paths
static std::string g_inputFilePath;
static std::string g_outputFilePath;

// Handler for Browse Input button
void HandleBrowseInput(HWND hPanel) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hPanel;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrTitle = "Select EJ Input File";

    if (GetOpenFileNameA(&ofn) == TRUE) {
        g_inputFilePath = szFile;
        SetDlgItemTextA(hPanel, IDC_EJ_INPUT_PATH, szFile);
        SetStatusText("Input file selected");
    }
}

// HandleBrowseOutput removed - output is auto-generated now

// Handler for Process button
void HandleProcessEJ(HWND hPanel) {
    // Get input path
    char inputPath[MAX_PATH];
    GetDlgItemTextA(hPanel, IDC_EJ_INPUT_PATH, inputPath, MAX_PATH);

    if (strlen(inputPath) == 0) {
        MessageBoxA(hPanel, "Please select an input file.", "Error", MB_OK | MB_ICONERROR);
        SetStatusText("Error: No input file selected");
        return;
    }

    // Get TID
    char tid[50];
    GetDlgItemTextA(hPanel, IDC_EJ_TID_INPUT, tid, 50);

    if (strlen(tid) == 0) {
        MessageBoxA(hPanel, "Please enter a TID (Terminal ID).", "Error", MB_OK | MB_ICONERROR);
        SetStatusText("Error: TID is required");
        return;
    }

    // Get Date from DateTimePicker
    SYSTEMTIME st;
    HWND hDatePicker = GetDlgItem(hPanel, IDC_EJ_DATE_PICKER);
    if (DateTime_GetSystemtime(hDatePicker, &st) == GDT_VALID) {
        // Format date as DDMMYYYY
        char date[20];
        sprintf_s(date, 20, "%02d%02d%04d", st.wDay, st.wMonth, st.wYear);

        // Get application folder
        char exePath[MAX_PATH];
        char outputFolder[MAX_PATH];
        char outputPath[MAX_PATH];
        
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash) *lastSlash = '\0';
        
        // Create EJParse folder path
        sprintf_s(outputFolder, MAX_PATH, "%s\\EJParse", exePath);
        
        // Create folder if it doesn't exist
        CreateDirectoryA(outputFolder, NULL);
        
        // Generate output filename: EJ_TID_DDMMYYYY.txt
        sprintf_s(outputPath, MAX_PATH, "%s\\EJ_%s_%s.txt", outputFolder, tid, date);

        // Get selected format
        EJParserFormat format = EJParserFormat::Lengkap;
        if (IsDlgButtonChecked(hPanel, IDC_EJ_RADIO_STANDARD) == BST_CHECKED) {
            format = EJParserFormat::Standard;
        }
        
        // Get Participant ID from dropdown - read window text directly
        char participantId[10] = "200"; // Default
        HWND hParticipantCombo = GetDlgItem(hPanel, IDC_EJ_PARTICIPANT_ID);
        
        if (hParticipantCombo) {
            // Read the currently displayed text in the combobox
            GetWindowTextA(hParticipantCombo, participantId, 10);
            
            // If empty or invalid, use default
            if (strlen(participantId) == 0 || 
                (strcmp(participantId, "200") != 0 && 
                 strcmp(participantId, "008") != 0 && 
                 strcmp(participantId, "002") != 0 && 
                 strcmp(participantId, "009") != 0)) {
                strcpy_s(participantId, 10, "200");
            }
        }

        // Create parser and process file
        EJParser parser(inputPath, outputPath, format);
        
        // Set TID and Participant ID for Standard format
        parser.setTID(std::string(tid));
        parser.setParticipantId(std::string(participantId));

        // Show processing status
        std::ostringstream statusMsg;
        statusMsg << "Processing: " << tid << " - " << date;
        SetStatusText(statusMsg.str().c_str());

        bool success = parser.processFile();

        if (success) {
            std::ostringstream msg;
            msg << "EJ file processed successfully!\n\n"
                << "Output file: " << outputPath << "\n\n"
                << "TID: " << tid << "\n"
                << "Date: " << date;
            MessageBoxA(hPanel, msg.str().c_str(), "Success", MB_OK | MB_ICONINFORMATION);
            SetStatusText("Processing complete - File saved successfully");
            
            // Update preview label
            char preview[MAX_PATH];
            sprintf_s(preview, MAX_PATH, "Saved to: %s", outputPath);
            SetDlgItemTextA(hPanel, IDC_EJ_OUTPUT_PREVIEW, preview);
        }
        else {
            std::ostringstream msg;
            msg << "Failed to process EJ file.\n\n"
                << "Error: " << parser.getLastError();
            MessageBoxA(hPanel, msg.str().c_str(), "Error", MB_OK | MB_ICONERROR);
            SetStatusText("Error: Processing failed");
        }
    }
    else {
        MessageBoxA(hPanel, "Please select a date.", "Error", MB_OK | MB_ICONERROR);
        SetStatusText("Error: Date is required");
    }
}

// Legacy dialog procedure (deprecated but kept for backward compatibility)
INT_PTR CALLBACK EJParserDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    // This is no longer used in the new UI
    return (INT_PTR)FALSE;
}

// Legacy show dialog function (deprecated)
void ShowEJParserDialog(HWND hWndParent) {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EJPARSER_DIALOG), hWndParent, EJParserDialogProc);
}
