/******************************************************************************
	WinGHCi, a GUI for GHCi

	DlgAbout.c: About dialog code

	Original code taken from Winhugs (http://haskell.org/hugs)

	With minor modifications by Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"
#include "General.h"
#include "WinGHCi.h"
#include "Strings.h"
#include "General.h"

const INT BmpTransparent = RGB(255,0,255);

LPCTSTR AppId = TEXT("WinGHCi %s\n");

LPCTSTR AboutText = 
    TEXT("A simple GUI for GHCi on Windows\n")
	TEXT("http://code.google.com/p/winghci\n")
	TEXT("Pepe Gallardo, 2009\n")
    TEXT("\n")

    TEXT("GHC Website: http://www.haskell.org/ghc\n")
    TEXT("\n")

	TEXT("Acknowledgements\n")
	TEXT("WinGHCi is closely based on Winhugs code\n")
    TEXT("http://www.haskell.org/hugs\n");





typedef struct _AboutData
{
    HIMAGELIST hImgList;
} AboutData;

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

VOID ShowAboutDialog(VOID)
{
    DialogBox(hThisInstance, MAKEINTRESOURCE(DLG_ABOUT), hWndMain, AboutDlgProc);
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg) {
	case WM_INITDIALOG:
	    {
		HWND hRTF = GetDlgItem(hDlg, IDC_RtfAbout);
		HWND hAppId = GetDlgItem(hDlg, IDC_RtfAboutTitle);
		AboutData* ad = malloc(sizeof(AboutData));
		HBITMAP hBmp;
		BITMAP bm;
		TCHAR Text[2048];
        CHARFORMAT cf;

		SetWindowLongPtr(hDlg, GWL_USERDATA, (LONG) ad);

	    CenterDialogInParent(hDlg);
		hBmp = LoadBitmap(hThisInstance, MAKEINTRESOURCE(BMP_ABOUT));
		GetObject(hBmp, sizeof(BITMAP), (LPTSTR)&bm);

		ad->hImgList = ImageList_Create(bm.bmWidth, bm.bmHeight, ILC_COLOR16 | ILC_MASK, 1, 1);
	    
	    ImageList_AddMasked(ad->hImgList, hBmp, BmpTransparent);
	    DeleteObject(hBmp);

	    SendMessage(hRTF, EM_AUTOURLDETECT, TRUE, 0);

		wsprintf(Text,AppId,WinGHCi_VERSION_STRING);
		  

		SetWindowText(hAppId, Text);

		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwEffects = CFE_BOLD;
		cf.yHeight = PointToTwip(15);
		cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_SIZE;       
		SendMessage(hAppId, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &cf);

		wsprintf(Text,AppId,WinGHCi_VERSION_STRING);
	    SetWindowText(hRTF, AboutText);

	    SendMessage(hRTF, EM_SETEVENTMASK, 0, ENM_LINK);
	}
	return (INT_PTR)TRUE;

	case WM_PAINT:
	    {
		PAINTSTRUCT ps;
		AboutData* ad = (AboutData*) GetWindowLongPtr(hDlg, GWL_USERDATA);

		BeginPaint(hDlg, &ps);
		ImageList_Draw(ad->hImgList, 0, ps.hdc, 20, 25, ILD_TRANSPARENT);
		EndPaint(hDlg, &ps);
	    }
	    break;

	case WM_COMMAND:
	    switch (LOWORD(wParam)) {
		case IDOK: case IDCANCEL:
		    EndDialog(hDlg, TRUE);
		    return (INT_PTR)TRUE;
	    }
	    break;

	case WM_NOTIFY:
	    if (wParam == IDC_RtfAbout && ((LPNMHDR) lParam)->code == EN_LINK)
	    {
		TEXTRANGE tr;
		TCHAR Buffer[1000];
	        ENLINK* enl = (ENLINK*) lParam;

		if (enl->msg == WM_LBUTTONUP)
		{
		    tr.lpstrText = Buffer;
		    tr.chrg.cpMin = enl->chrg.cpMin;
		    tr.chrg.cpMax = enl->chrg.cpMax;

		    SendMessage(enl->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM) &tr);
		    ExecuteFile(Buffer);
		}
	    }
	    break;

	case WM_DESTROY:
	    {
		AboutData* ad = (AboutData*) GetWindowLongPtr(hDlg, GWL_USERDATA);
		ImageList_Destroy(ad->hImgList);
		free(ad);
	    }
    }
    return (INT_PTR)FALSE;
}
