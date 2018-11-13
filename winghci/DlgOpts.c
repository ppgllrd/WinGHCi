/******************************************************************************
	WinGHCi, a GUI for GHCi

	DlgOpts.c: options dialog code

	Original code taken from Winhugs (http://haskell.org/hugs)

	With modifications by Pepe Gallardo, 2009-March
*******************************************************************************/

#include "Combo.h"
#include "CommonIncludes.h"
#include "Colors.h"
#include "General.h"
#include "Registry.h"
#include "RtfWindow.h"
#include "Strings.h"
#include "Utf8.h"
#include "WndMain.h"
#include "WinGHCi.h"


#define MAX_STRING 3*MAX_PATH



BOOL GHCi_Flag_RevertCAFs, GHCi_Flag_PrintStats, GHCi_Flag_PrintTypes;

COMBO GHCi_Combo_Startup,  GHCi_Combo_Editor, GHCi_Combo_Prompt;





BOOL GetDlgItemBool(HWND hDlg, INT CtrlID)
{
    return (IsDlgButtonChecked(hDlg, CtrlID) == BST_CHECKED);
}

VOID SetDlgItemBool(HWND hDlg, INT CtrlID, BOOL Value)
{
    CheckDlgButton(hDlg, CtrlID, Value ? BST_CHECKED : BST_UNCHECKED);
}

typedef struct {
	BOOL	*FlagState;
	LPTSTR	GHCiText;
	INT		CtrlID;
} FLAG;


FLAG flags[] = 
	{ {&GHCi_Flag_RevertCAFs, TEXT("r"), IDC_ChkRevertCAFs}
    , {&GHCi_Flag_PrintStats, TEXT("s"), IDC_ChkPrintStats}
	, {&GHCi_Flag_PrintTypes, TEXT("t"), IDC_ChkPrintTypes}
	, {NULL,NULL,0}
    };


#define FLAG_REGISTRY_PREFFIX	TEXT("GHCi_FLAG_")



VOID ReadOptionsFromRegistry(VOID)
{
 FLAG *fs;
 TCHAR Buffer[1024];
 
 for(fs=flags; fs->FlagState != NULL; fs++) {
	 wsprintf(Buffer, TEXT("%s%s"), FLAG_REGISTRY_PREFFIX, fs->GHCiText);
	 *(fs->FlagState) = (BOOL) readRegInt(Buffer, FALSE);
 }


 ReadComboFromRegistry(GHCi_Combo_Startup);
 ReadComboFromRegistry(GHCi_Combo_Editor);
 ReadComboFromRegistry(GHCi_Combo_Prompt);

}


VOID WriteOptions2Registry(VOID)
{
 FLAG *fs;
 TCHAR Buffer[1024];
 
 for(fs=flags; fs->FlagState != NULL; fs++) {
	 wsprintf(Buffer, TEXT("%s%s"), FLAG_REGISTRY_PREFFIX, fs->GHCiText);
	 writeRegInt(Buffer, *(fs->FlagState));
 }


 WriteCombo2Registry(GHCi_Combo_Startup);
 WriteCombo2Registry(GHCi_Combo_Editor);
 WriteCombo2Registry(GHCi_Combo_Prompt);
}


VOID MakeGHCiFlagCommand(FLAG *f, LPTSTR Command)
{
	wsprintf(Command,TEXT(":%s +%s"),*(f->FlagState) ? TEXT("set") : TEXT("unset"), f->GHCiText);
}

VOID MakeGHCiEditorCommand(LPTSTR Editor, LPTSTR Command)
{
	wsprintf(Command,TEXT(":set editor %s"), Editor);
}



VOID MakeGHCiExpandedEditorCommand(LPTSTR Editor, LPTSTR Command)
{
	TCHAR ShortEditor[MAX_PATH], StartProcess[MAX_PATH], ShortStartProcess[MAX_PATH];

	

	if(Editor[0]==TEXT('&')) {
		wsprintf(StartProcess, TEXT("%sStartProcess.exe"), GetWinGHCiInstallDir());
	    AsShortFileName(StartProcess, ShortStartProcess, MAX_PATH);

		AsShortFileName(&Editor[1],ShortEditor,MAX_PATH);
		wsprintf(Command, TEXT(":set editor %s %s"), ShortStartProcess, ShortEditor);

	} else {
		AsShortFileName(Editor,ShortEditor,MAX_PATH);
		wsprintf(Command, TEXT(":set editor %s"), ShortEditor);
	}
}


VOID MakeGHCiPromptCommand(LPTSTR Prompt, LPTSTR Command, BOOL AddMarkers) 
{
	#define SETPROMPT TEXT(":set prompt %s%s%s")
	wsprintf(Command,SETPROMPT,AddMarkers ? BEGIN_OF_PROMPT : TEXT(""),Prompt,AddMarkers ? END_OF_PROMPT : TEXT(""));
}



VOID UpdateOptions(HWND hDlg) 
{
  FLAG *fs;
  TCHAR Buffer[3*MAX_PATH];

  if(ComboHasChanged(hDlg,GHCi_Combo_Startup)) {
	  INT resp = MessageBox( hDlg
		                   , TEXT("GHCi startup has changed. The interpreter must be initialized. Do you want to proceed?")
						   , TEXT("WinGHCi"), MB_YESNO|MB_ICONQUESTION);

	  if(resp==IDYES) {
        RtfWindowPutS(TEXT("\n"));
		ComboGetDlgText(hDlg, GHCi_Combo_Startup, Buffer, 3*MAX_PATH);
		
		SetEvent(hKillGHCi);
		//pause StdoutPrinterThread thread
		SignalObjectAndWait(hSigSuspendStdoutPrinterThread
			               ,hSigStdoutPrinterThreadSuspended, INFINITE, FALSE);

		if (CreateGHCiProcess(Buffer)) {		
            ComboUpdate(hDlg,GHCi_Combo_Startup);
		} else {
			TCHAR ErrorMsg[3*MAX_PATH];
			wsprintf( ErrorMsg,TEXT("GHCi could not be initialized as follows:\n %s\n\nIt will be restarted using the previous configuration:\n %s")
				    , Buffer, ComboGetValue(GHCi_Combo_Startup)
				);
			MessageBox( hDlg
		              , ErrorMsg
					  , TEXT("WinGHCi"), MB_OK|MB_ICONSTOP);
			CreateGHCiProcess(ComboGetValue(GHCi_Combo_Startup));
		}

		// resume StdoutPrinterThread
		Sleep(100);
		SetEvent(hSigResumeStdoutPrinterThread);
	  }	
  }



 //update flags
 for(fs = flags; fs->FlagState != NULL; fs++) {
	 BOOL newState = GetDlgItemBool(hDlg, fs->CtrlID);
	 BOOL hasChanged = newState != *(fs->FlagState);
	 *(fs->FlagState) = newState;

	 if(hasChanged) {
		MakeGHCiFlagCommand(fs,Buffer);
		FireCommand(Buffer);
	 }
 }

 // update editor	
 if(ComboHasChanged(hDlg,GHCi_Combo_Editor)) {
	 ComboUpdate(hDlg,GHCi_Combo_Editor);
	 MakeGHCiEditorCommand(ComboGetValue(GHCi_Combo_Editor), Buffer);
	 FireCommand(Buffer);
 }


 //update prompt
 if(ComboHasChanged(hDlg,GHCi_Combo_Prompt)) {
	 ComboUpdate(hDlg,GHCi_Combo_Prompt);
	 MakeGHCiPromptCommand(ComboGetValue(GHCi_Combo_Prompt), Buffer, FALSE);
	 FireCommand(Buffer);
 }


}

VOID SendOptions2GHCi(VOID)
{
	FLAG *fs;
	TCHAR Buffer[1024];

	for(fs = flags; fs->FlagState != NULL; fs++) {
		MakeGHCiFlagCommand(fs,Buffer);
		SendToGHCiStdinLn(Buffer);
	}

	MakeGHCiExpandedEditorCommand(ComboGetValue(GHCi_Combo_Editor), Buffer);
	SendToGHCiStdinLn(Buffer);
	 
	MakeGHCiPromptCommand(ComboGetValue(GHCi_Combo_Prompt), Buffer, TRUE);
	SendToGHCiStdinLn(Buffer);

	PrintGHCiOutput(hChildStdoutRd, ForegroundColor);

	// clear screen, so that only one instance of the prompt is shown
	RtfWindowClearLastLine();
	SendToGHCiStdinLn(TEXT(""));
	PrintGHCiOutput(hChildStdoutRd, ForegroundColor);

}


INT CALLBACK ListAllFonts(CONST LOGFONT* lpelfe, CONST TEXTMETRIC* lpntme, DWORD FontType, LPARAM lParam)
{
    HWND hLst = (HWND) lParam;
    LPCTSTR FontName = (LPCTSTR) lpelfe->lfFaceName;
    if (SendMessage(hLst, CB_FINDSTRINGEXACT, -1, (LPARAM) FontName) == CB_ERR)
	SendMessage(hLst, CB_ADDSTRING, 0, (LPARAM) FontName);
    return 1;
}


VOID CalculateFont(HWND hDlg, CHARFORMAT* cf)
{
    BOOL ValidSize;
    INT NewSize;
    INT CurSel;
    HWND hFace = GetDlgItem(hDlg, IDC_LstFontFace);

    RegistryReadFont(cf);

    CurSel = (INT) SendMessage(hFace, CB_GETCURSEL, 0, 0);
    if (CurSel == CB_ERR)
	GetWindowText(hFace, cf->szFaceName, 32);
    else
	SendMessage(hFace, CB_GETLBTEXT, CurSel, (LPARAM) cf->szFaceName);

    cf->dwEffects = 0;
    cf->dwEffects |= (GetDlgItemBool(hDlg, IDC_ChkFontBold) ? CFE_BOLD : 0);
    cf->dwEffects |= (GetDlgItemBool(hDlg, IDC_ChkFontItalic) ? CFE_ITALIC : 0);

    // check the size
    NewSize = GetDlgItemInt(hDlg, IDC_TxtFontSize, &ValidSize, FALSE);
    if (ValidSize) cf->yHeight = PointToTwip(NewSize);
}

#define PREVIEW_PROMPT TEXT("Prelude> ")
#define PREVIEW_EXPR   TEXT("1+a")
#define PREVIEW_ERROR  TEXT("<interactive>:1:2: Not in scope: `a'")

VOID UpdateFontPreview(HWND hDlg)
{
    CHARFORMAT cf;
    HWND hRTF = GetDlgItem(hDlg, IDC_RtfPreview);
    CHARRANGE cr;

    CalculateFont(hDlg, &cf);
	cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_SIZE | CFM_FACE | CFM_COLOR;
	cf.crTextColor = ForegroundColor;
    SendMessage(hRTF, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &cf);
	SendMessage(hRTF, EM_SETBKGNDCOLOR, 0, (LPARAM) BackgroundColor);

	SendMessage(hRTF, EM_HIDESELECTION, TRUE, 0);	

	cr.cpMin = StringLen(PREVIEW_PROMPT) + StringLen(PREVIEW_EXPR) + 1;
	cr.cpMax  = cr.cpMin + StringLen(PREVIEW_ERROR);
	SendMessage(hRTF, EM_EXSETSEL, 0, (LPARAM) &cr);
    cf.crTextColor = ErrorColor;
    SendMessage(hRTF, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);


	cr.cpMin = 0;
	cr.cpMax = cr.cpMin + StringLen(PREVIEW_PROMPT);
	SendMessage(hRTF, EM_EXSETSEL, 0, (LPARAM) &cr);
	cf.dwEffects |= CFE_BOLD;
    cf.crTextColor = PromptColor;
    SendMessage(hRTF, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);

}



VOID InitOptionsFont(HWND hDlg)
{
    // load up the list of fonts
    HDC hDC = GetDC(hDlg);
    CHARFORMAT cf;
    LOGFONT lf;

    SendDlgItemMessage(hDlg, IDC_SpnFontSize, UDM_SETRANGE, 0, MAKELONG(72, 6));

    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfFaceName[0] = TEXT('\0');;
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(hDC, &lf, ListAllFonts, (LPARAM) GetDlgItem(hDlg, IDC_LstFontFace), 0);
    ReleaseDC(hDlg, hDC);

	SendMessage(GetDlgItem(hDlg, IDC_RtfPreview), EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN, (LPARAM)MAKELONG(5,5));

    SetDlgItemText(hDlg, IDC_RtfPreview, PREVIEW_PROMPT PREVIEW_EXPR TEXT("\n") PREVIEW_ERROR);
 
    // setup the config options
    RegistryReadFont(&cf);
    SetDlgItemText(hDlg, IDC_LstFontFace, cf.szFaceName);
    SetDlgItemBool(hDlg, IDC_ChkFontBold, cf.dwEffects & CFE_BOLD);
    SetDlgItemBool(hDlg, IDC_ChkFontItalic, cf.dwEffects & CFE_ITALIC);
    SetDlgItemInt(hDlg, IDC_TxtFontSize, TwipToPoint(cf.yHeight), FALSE);

	SetDlgItemBool(hDlg, IDC_ChkRevertCAFs, GHCi_Flag_RevertCAFs);
	SetDlgItemBool(hDlg, IDC_ChkPrintStats, GHCi_Flag_PrintStats);
	SetDlgItemBool(hDlg, IDC_ChkPrintTypes, GHCi_Flag_PrintTypes);


    UpdateFontPreview(hDlg);
}




INT_PTR CALLBACK OptsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL ShowOptsDialog(VOID)
{
    return DialogBox(hThisInstance, MAKEINTRESOURCE(DLG_OPTS), hWndMain, OptsDlgProc);
}


COLORREF* colorRefForId(HWND hDlg,HWND hCtrl)
{
	if (hCtrl==GetDlgItem(hDlg, IDC_BackgroundColor))
			return &BackgroundColor;
	else if (hCtrl==GetDlgItem(hDlg, IDC_ForegroundColor))
			return &ForegroundColor;
	else if (hCtrl==GetDlgItem(hDlg, IDC_PromptColor))
			return &PromptColor;
	else if (hCtrl==GetDlgItem(hDlg, IDC_ErrorColor))
			return &ErrorColor;
	else			 
			return NULL;
}

VOID ChooseColorFor(HWND hDlg,HWND hCtrl)
{	CHOOSECOLOR cc;                 // common dialog box structure 
	static COLORREF acrCustClr[16]; // array of custom colors 
    COLORREF *cr = colorRefForId(hDlg,hCtrl);

	// Initialize CHOOSECOLOR 
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hDlg;
	cc.lpCustColors = (LPDWORD) acrCustClr;
	cc.rgbResult = *cr;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
						 
	if (ChooseColor(&cc)==TRUE) 
				*cr = cc.rgbResult; 
}


INT_PTR CALLBACK OptsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hBrushStatic = NULL;

	switch (msg) {
		case WM_INITDIALOG:
			CenterDialogInParent(hDlg);
			InitOptionsFont(hDlg);
			ComboInitDialog(hDlg,GHCi_Combo_Editor);
			ComboInitDialog(hDlg,GHCi_Combo_Startup);
			ComboInitDialog(hDlg,GHCi_Combo_Prompt);

			break;

		case WM_CTLCOLORSTATIC: {
			 
			        COLORREF *cr = colorRefForId(hDlg,(HWND)lParam); 
					if(cr)
					{
						SetBkColor((HDC) wParam, *cr);
						SetBkMode((HDC) wParam, TRANSPARENT);

						if(!hBrushStatic)
							DeleteObject(hBrushStatic);
						hBrushStatic = CreateSolidBrush(*cr);

						return (UINT) hBrushStatic;
					}  else 
					 // Let Windows do default handling
					 return FALSE;
		} 

		case WM_COMMAND: {
				INT Code = HIWORD(wParam);
				INT CtrlId = LOWORD(wParam);
				HWND hCtrl = (HWND) lParam;

				if ((CtrlId == IDC_ChkFontBold && Code == BN_CLICKED) ||
					(CtrlId == IDC_ChkFontItalic && Code == BN_CLICKED) ||
					(CtrlId == IDC_TxtFontSize && Code == EN_CHANGE) ||
					(CtrlId == IDC_LstFontFace && (Code == CBN_EDITCHANGE || Code == CBN_SELCHANGE))
					)
					UpdateFontPreview(hDlg);


			switch (LOWORD(wParam)) {

				case IDC_BackgroundColor: 
				case IDC_ForegroundColor:
				case IDC_PromptColor:
				case IDC_ErrorColor:
					 if(Code==STN_CLICKED) ChooseColorFor(hDlg,hCtrl);
					 UpdateFontPreview(hDlg);
					 InvalidateRect(hCtrl,NULL,1);
					 UpdateWindow(hCtrl);
					 break;                 

				case IDOK: {

					CHARFORMAT cf, prevCf;

					RegistryReadFont(&prevCf);
					CalculateFont(hDlg, &cf);

					if( (StringCmp(cf.szFaceName,prevCf.szFaceName)) 
						  || (cf.yHeight != prevCf.yHeight)  
						  || (cf.dwEffects != prevCf.dwEffects)) {

							RegistryWriteFont(&cf);
							RtfWindowUpdateFont();
					}

					RegistryWriteColors(); 
					RtfWindowUpdateColors(); 

					UpdateOptions(hDlg) ;

					EndDialog(hDlg, TRUE);
					return (INT_PTR)TRUE;
				}
				case IDCANCEL:
					RegistryReadColors(); 
					EndDialog(hDlg, TRUE);
					return (INT_PTR)TRUE;
			}
		}
		break;
	}
	return (INT_PTR)FALSE;
}


VOID InitOptions(VOID)
{
	GHCi_Combo_Startup = NewCombo( TEXT("GHCi_STARTUP_COMBO")
		                         , IDC_GHCi_Combo_Startup
								 , TEXT("ghc --interactive"));
	GHCi_Combo_Editor = NewCombo( TEXT("GHCi_EDITOR_COMBO")
		                        , IDC_GHCi_Combo_Editor
								, TEXT("&notepad"));
	GHCi_Combo_Prompt = NewCombo( TEXT("GHCi_PROMPT_COMBO")
		                        , IDC_GHCi_Combo_Prompt
								, TEXT("%s>"));
	ReadOptionsFromRegistry();
}

VOID FinalizeOptions(VOID)
{
	WriteOptions2Registry();

	FreeCombo(GHCi_Combo_Startup);
	FreeCombo(GHCi_Combo_Editor);
	FreeCombo(GHCi_Combo_Prompt);
}
