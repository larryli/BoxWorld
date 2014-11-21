#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#ifdef __LCC__
#pragma lib <shell32.lib>
#else
#pragma comment(lib, "shell32.lib")
#endif
#include "linktext.h"

static WNDPROC OldStaticProc = NULL;

typedef struct LINKTEXTINFO {
	HCURSOR hCursorLinkText, hCursorNormal;
	BOOL bIsOverControl;
	HFONT hFont;
	COLORREF cLinkText, cHoverText;
} LINKTEXTINFO;

static LRESULT CALLBACK LinkTextProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	RECT rect;
	TCHAR szBuf[1024];
	LINKTEXTINFO *plti;

	plti = (LINKTEXTINFO *)GetWindowLong(hwnd, GWL_USERDATA);
	if (plti == NULL) {
		plti = (LINKTEXTINFO *)malloc(sizeof(LINKTEXTINFO));
		if (plti == NULL)
			goto end;
		plti->bIsOverControl = FALSE;
		plti->hCursorLinkText = LoadCursor((HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), TEXT("HAND"));
		if (plti->hCursorLinkText == NULL)
				plti->hCursorLinkText = LoadCursor(NULL, IDC_HELP);
		plti->hFont = (HFONT) SendMessage(GetParent(hwnd), WM_GETFONT, 0, 0);
		if (plti->hFont != NULL) {
			LOGFONT lf;

			GetObject(plti->hFont, sizeof(LOGFONT), &lf);
			lf.lfUnderline = (BYTE) TRUE;
			plti->hFont = CreateFontIndirect(&lf);
		}

// [HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\Settings]
// "Anchor Color"="0,0,255"
// "Anchor Color Hover"="255,0,0"
		{
			HKEY hKey;
			DWORD size, res;
			TCHAR szBuf[MAX_PATH];
			int r, g, b;
			const TCHAR szKey[] = TEXT("Software\\Microsoft\\Internet Explorer\\Settings");
			const TCHAR szLinkText[] = TEXT("Anchor Color");
			const TCHAR szHoverText[] = TEXT("Anchor Color Hover");

			RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_READ, &hKey);
			size = sizeof(szBuf);
			res = REG_SZ;
			if (RegQueryValueEx(hKey, szLinkText, NULL, &res, szBuf, &size) == ERROR_SUCCESS) {
				sscanf(szBuf, "%d,%d,%d", &r, &g, &b);
				r = ((unsigned)r) & 0xFF, g = ((unsigned)g) & 0xFF, b = ((unsigned)b) & 0xFF;
				plti->cLinkText = RGB(r, g, b);
			} else
				plti->cLinkText = RGB(0, 0, 255);
			if (RegQueryValueEx(hKey, szHoverText, NULL, &res, szBuf, &size) == ERROR_SUCCESS) {
				sscanf(szBuf, "%d,%d,%d", &r, &g, &b);
				r = ((unsigned)r) & 0xFF, g = ((unsigned)g) & 0xFF, b = ((unsigned)b) & 0xFF;
				plti->cHoverText = RGB(r, g, b);
			} else
				plti->cHoverText = RGB(255, 0, 0);
			RegCloseKey(hKey);
		}

		SetWindowLong(hwnd, GWL_USERDATA, (LONG)plti);
	}

	switch (msg) {

	case WM_NCHITTEST: {
		plti->hCursorNormal = SetCursor(plti->hCursorLinkText);
		SetCapture(hwnd);
		plti->bIsOverControl = TRUE;
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	}

	case WM_MOUSEMOVE: {
		GetClientRect(hwnd, &rect);
		if (plti->bIsOverControl) {
			POINT ptMousePos;
			DWORD dwMousePos;

			dwMousePos = GetMessagePos();

			ptMousePos.x = LOWORD(dwMousePos);
			ptMousePos.y = HIWORD(dwMousePos);

			ScreenToClient(hwnd, &ptMousePos);

			if (!(BOOL) PtInRect(&rect, ptMousePos)) {
				SetCursor(plti->hCursorNormal);
			    ReleaseCapture();
				plti->bIsOverControl = FALSE;
				InvalidateRect(hwnd, NULL, TRUE);
			}
		}
		break;
	}

	case WM_LBUTTONUP: {
		SHELLEXECUTEINFO seli;

		SetCursor(plti->hCursorNormal);
		ReleaseCapture();
		plti->bIsOverControl = FALSE;
		InvalidateRect(hwnd, NULL, TRUE);

		GetWindowText(hwnd, szBuf, sizeof(szBuf) / sizeof(TCHAR) - 1);
		ZeroMemory(&seli, sizeof(seli));
		seli.cbSize = sizeof(seli);
		seli.lpVerb = TEXT("open");
		seli.lpFile = szBuf;
		seli.nShow = SW_SHOW;
		ShellExecuteEx(&seli);
		break;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc;

		GetClientRect(hwnd, &rect);
		GetWindowText(hwnd, szBuf, sizeof(szBuf) / sizeof(TCHAR) - 1);

		hdc	= BeginPaint(hwnd, &ps);

		SetBkMode(hdc, TRANSPARENT);
		SelectObject(hdc, plti->hFont);

		// Link Color?
		if (plti->bIsOverControl)
			SetTextColor(hdc, plti->cHoverText);
		else
			SetTextColor(hdc, plti->cLinkText);

		// GetWindowLong(hwnd, GWL_STYLE) & 0x3
		// CENTER ? LEFT ? RIGHT
		DrawText(hdc, szBuf, lstrlen(szBuf), &rect, GetWindowLong(hwnd, GWL_STYLE) & 0x3);

		EndPaint(hwnd, &ps);

		break;
		}

	case WM_DESTROY:
		if (plti->bIsOverControl) {
			SetCursor(plti->hCursorNormal);
			ReleaseCapture();
		}
		DestroyCursor(plti->hCursorLinkText);
		DeleteObject(plti->hFont);
		free(plti);
		break;
	}
end:
	return CallWindowProc(OldStaticProc, hwnd, msg, wp, lp);
}

static BOOL CALLBACK EnumWindowChildProc(HWND hwnd, LPARAM lParam)
{
	TCHAR szBuf[1024];

	GetClassName(hwnd, szBuf, sizeof(szBuf));
	if (lstrcmp(szBuf, TEXT("Static")) == 0) {
		GetWindowText(hwnd, szBuf, sizeof(szBuf));
		if ((strstr(szBuf, "://") != NULL)
			|| (strncmp(szBuf, "mailto:", 7) == 0)) {
			if (OldStaticProc == NULL)
				OldStaticProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LPARAM)LinkTextProc);
			else
				SetWindowLong(hwnd, GWL_WNDPROC, (LPARAM) LinkTextProc);
		}
	}
	return TRUE;
}

void EnableLinkText(HWND hwnd)
{
	EnumChildWindows(hwnd, EnumWindowChildProc, 0);
}

#ifdef TEST

#include "linktextres.h"

static BOOL CALLBACK DialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS wc;
	INITCOMMONCONTROLSEX cc;

	memset(&wc,0,sizeof(wc));
	wc.lpfnWndProc = DefDlgProc;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hinst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wc.lpszClassName = "linktext";
	RegisterClass(&wc);
	memset(&cc,0,sizeof(cc));
	cc.dwSize = sizeof(cc);
	cc.dwICC = 0xffffffff;
	InitCommonControlsEx(&cc);

	return DialogBox(hinst, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, (DLGPROC) DialogFunc);

}

static int InitializeApp(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	EnableLinkText(hDlg);
	return 1;
}

static BOOL CALLBACK DialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		InitializeApp(hwndDlg, wParam, lParam);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hwndDlg, 1);
			return 1;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return 1;
		}
		break;

	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		return TRUE;

	}
	return FALSE;
}
#endif
