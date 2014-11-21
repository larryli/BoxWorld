#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <string.h>

#include "boxmap.h"
#include "boxworldres.h"
#include "linktext.h"
#include "coolmenu.h"

#define BOXCX 30
#define BOXCY 30
#define BOXNX 16
#define BOXNY 14

typedef enum {
	NONE = ' ',
	WALL = '=' ,
	FLOOR = '.',
	DEST = 'o',
	BOXIN = 'X',
	BOXOUT = 'x',
	MANFRONT1 = '@',
	MANFRONT2 = 'a',
	MANBACK1 = 'b',
	MANBACK2 = 'c',
	MANLEFT1 = 'd',
	MANLEFT2 = 'e',
	MANRIGHT1 = 'f',
	MANRIGHT2 = 'g',
	MANFRONT1AT = '#',
	MANFRONT2AT = 'A',
	MANBACK1AT = 'B',
	MANBACK2AT = 'C',
	MANLEFT1AT = 'D',
	MANLEFT2AT = 'E',
	MANRIGHT1AT = 'F',
	MANRIGHT2AT = 'G'
} BOXSTATE;

BOXSTATE map[BOXNY][BOXNX];
/*
 = {
{' ', ' ', ' ', '.','.', ' ', ' ', ' ',' ', ' ', ' ', ' ',' ', ' ', ' ', ' '},
{' ', ' ', ' ', '.','.', ' ', ' ', ' ',' ', ' ', ' ', ' ',' ', ' ', ' ', ' '},
{' ', ' ', ' ', '.','.', '.', '.', ' ',' ', ' ', ' ', ' ',' ', ' ', ' ', ' '},
{' ', ' ', ' ', '.','.', '.', '.', '=','=', ' ', ' ', ' ',' ', ' ', ' ', ' '},
{' ', ' ', ' ', '.','.', '.', '.', 'o','=', ' ', ' ', ' ',' ', ' ', ' ', ' '},
{' ', ' ', ' ', '.','.', '.', '.', '.','.', '=', '=', '=',' ', ' ', ' ', ' '},
{' ', ' ', ' ', ' ','=', '.', '.', 'x','.', 'x', 'o', '=',' ', ' ', ' ', ' '},
{' ', ' ', ' ', ' ','=', 'o', '.', 'x','@', '=', '=', '=',' ', ' ', ' ', ' '},
{' ', ' ', ' ', ' ','=', '=', '=', '=','x', '=', ' ', ' ',' ', ' ', ' ', ' '},
{' ', ' ', ' ', ' ',' ', ' ', ' ', '=','o', '=', ' ', ' ',' ', ' ', ' ', ' '},
{' ', ' ', ' ', ' ',' ', ' ', ' ', '=','=', '=', ' ', ' ',' ', ' ', ' ', ' '},
{' ', ' ', ' ', ' ',' ', ' ', ' ', ' ',' ', ' ', ' ', ' ',' ', ' ', ' ', ' '},
{' ', ' ', ' ', ' ',' ', ' ', ' ', ' ',' ', ' ', ' ', ' ',' ', ' ', ' ', ' '},
{' ', ' ', ' ', ' ',' ', ' ', ' ', ' ',' ', ' ', ' ', ' ',' ', ' ', ' ', ' '}};
*/
BOXSTATE mapu[BOXNY][BOXNX];
UINT manxu, manyu;

INT nMap, nMapMax;
UINT manx, many;
UINT sx, sy;
BOOL bUndo;
UINT nStep, nStepMin;

TCHAR szAppName[] = TEXT("Box World!");

HINSTANCE hInst;
HWND hwndMain;
HBITMAP hBmpBoxWorld;
HWND hwndStatus;
void NewGame(HWND hwnd, UINT n);

BOOL LoadMapBitmap(HINSTANCE hInst)
{
	hBmpBoxWorld = LoadBitmap(hInst, TEXT("BOXWORLD"));
	return TRUE;
}

BOOL FreeMapBitmap(VOID)
{
	DeleteObject(hBmpBoxWorld);
	return TRUE;
}

LPCTSTR szRegKeyName = TEXT("Software\\lcc\\Games\\BoxWorld");
LPCTSTR szRegHighestStageName = TEXT("HighestStage");
LPCTSTR szRegBestMovesName0 = TEXT("BestMoves%d");

static TCHAR *GetStringFromRc(UINT uID)
{
	static TCHAR szBuf[1024];

	LoadString(hInst, uID, szBuf, sizeof szBuf);
	return szBuf;
}

UINT ReadHighestStage(VOID)
{
	HKEY hKey;
	DWORD dwCreate = REG_CREATED_NEW_KEY;
	DWORD dwType = REG_DWORD;
	DWORD dwData = 1;
	DWORD dwDataSize = sizeof(DWORD);

	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szRegKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwCreate)) {
		RegQueryValueEx(hKey, szRegHighestStageName, NULL, &dwType, (LPBYTE)&dwData, &dwDataSize);
		RegCloseKey(hKey);
	}
	return dwData;
}

UINT WriteHighestStage(UINT n)
{
	HKEY hKey;
	DWORD dwCreate = REG_CREATED_NEW_KEY;
	DWORD dwType = REG_DWORD;
	DWORD dwData = n;
	DWORD dwDataSize = sizeof(DWORD);

	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szRegKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwCreate)) {
		RegSetValueEx(hKey, szRegHighestStageName, 0, dwType, (LPBYTE)&dwData, dwDataSize);
		RegCloseKey(hKey);
	}
	return dwData;
}

UINT ReadBestMoves(UINT n)
{
	HKEY hKey;
	DWORD dwCreate = REG_CREATED_NEW_KEY;
	DWORD dwType = REG_DWORD;
	DWORD dwData = -1;
	DWORD dwDataSize = sizeof(DWORD);
	TCHAR szBuf[128];

	wsprintf(szBuf, szRegBestMovesName0, n + 1);
	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szRegKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwCreate)) {
		RegQueryValueEx(hKey, szBuf, NULL, &dwType, (LPBYTE)&dwData, &dwDataSize);
		RegCloseKey(hKey);
	}
	return dwData;
}

UINT WriteBestMoves(UINT n, UINT i)
{
	HKEY hKey;
	DWORD dwCreate = REG_CREATED_NEW_KEY;
	DWORD dwType = REG_DWORD;
	DWORD dwData = i;
	DWORD dwDataSize = sizeof(DWORD);

	TCHAR szBuf[128];

	wsprintf(szBuf, szRegBestMovesName0, n + 1);
	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szRegKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwCreate)) {
		RegSetValueEx(hKey, szBuf, 0, dwType, (CONST BYTE *)&dwData, dwDataSize);
		RegCloseKey(hKey);
	}
	return dwData;
}

LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

static BOOL InitApplication(void)
{
	WNDCLASS wc;

	memset(&wc,0,sizeof(WNDCLASS));
	wc.style = CS_HREDRAW|CS_VREDRAW |CS_DBLCLKS ;
	wc.lpfnWndProc = (WNDPROC)MainWndProc;
	wc.hInstance = hInst;
	wc.hbrBackground = GetStockObject(BLACK_BRUSH);//(HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = szAppName;
	wc.lpszMenuName = TEXT("MENU_GAME");//MAKEINTRESOURCE(IDMAINMENU);
	wc.hCursor = LoadCursor(NULL,IDC_ARROW);
	wc.hIcon = LoadIcon(hInst, TEXT("BoxOutIcon"));
	if (!RegisterClass(&wc))
		return 0;
	return 1;
}

HWND CreateboxworldWndClassWnd(void)
{
	return CreateWindow(szAppName, szAppName,
		WS_MINIMIZEBOX|WS_VISIBLE|WS_CAPTION|WS_BORDER|WS_SYSMENU,
		CW_USEDEFAULT,0,0,0,NULL,NULL,hInst,NULL);
}

BOOL CALLBACK AboutDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		EnableLinkText(hwnd);
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	case WM_CLOSE:
		EndDialog(hwnd,0);
		return 1;
	}
	return FALSE;
}

BOOL CALLBACK JumpDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int n;

	switch (msg) {
	case WM_INITDIALOG:
		EnableLinkText(hwnd);
		SetDlgItemInt(hwnd, IDD_NUMBER, nMap + 1, FALSE);
		SendDlgItemMessage(hwnd, IDD_NUMBER + 1, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwnd, IDD_NUMBER), 0);
		SendDlgItemMessage(hwnd, IDD_NUMBER + 1, UDM_SETRANGE, 0, MAKELONG(nMapMax + 1, 1));
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
			n = GetDlgItemInt(hwnd, IDD_NUMBER, NULL, FALSE);
			if (n >= 1 && n <= nMapMax + 1) {
				nMap = n - 1;
				NewGame(hwndMain, nMap);
			}
			;	// no break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	case WM_CLOSE:
		EndDialog(hwnd,0);
		return 1;
	}
	return FALSE;
}

void MainWndProc_OnPaint(HWND hwnd)
{
	HDC hdc, hdcBmp;
	PAINTSTRUCT ps;
	UINT x, y, dx;

	hdc = BeginPaint(hwnd, &ps);
	hdcBmp = CreateCompatibleDC(hdc);
	SelectObject(hdcBmp, hBmpBoxWorld);
	for (y = 0; y < BOXNY; y++)
		for (x = 0; x < BOXNX; x++) {
			switch (map[y][x]) {
			case WALL:
				dx = BOXCX * 1;
				break;
			case FLOOR:
				dx = BOXCX * 2;
				break;
			case DEST:
				dx = BOXCX * 3;
				break;
			case BOXIN:
				dx = BOXCX * 4;
				break;
			case BOXOUT:
				dx = BOXCX * 5;
				break;
			case MANFRONT1:
			case MANFRONT1AT:
				dx = BOXCX * 6;
				break;
			case MANBACK1:
			case MANBACK1AT:
				dx = BOXCX * 7;
				break;
			case MANLEFT1:
			case MANLEFT1AT:
				dx = BOXCX * 8;
				break;
			case MANRIGHT1:
			case MANRIGHT1AT:
				dx = BOXCX * 9;
				break;
			case MANFRONT2:
			case MANFRONT2AT:
				dx = BOXCX * 10;
				break;
			case MANBACK2:
			case MANBACK2AT:
				dx = BOXCX * 11;
				break;
			case MANLEFT2:
			case MANLEFT2AT:
				dx = BOXCX * 12;
				break;
			case MANRIGHT2:
			case MANRIGHT2AT:
				dx = BOXCX * 13;
				break;
			default:
				continue;
			}
			BitBlt(hdc, BOXCX * x + sx, BOXCY * y + sy, BOXCX, BOXCY, hdcBmp, dx, 0, SRCCOPY);
		}
	DeleteDC(hdcBmp);
	EndPaint(hwnd, &ps);
}

void CheckUndoMenu(HWND hwnd, BOOL bEnable)
{
	bUndo = bEnable;
	EnableMenuItem(GetMenu(hwnd), IDM_UNDO, MF_BYCOMMAND | (bEnable ? MF_ENABLED : MF_GRAYED));
}

void ShowStepNumber(UINT n)
{
	TCHAR szBuf[128];

	wsprintf(szBuf, TEXT(GetStringFromRc(IDS_STEPNUMBER)), n);
	SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)&szBuf);
}

void ShowStepMinNumber(UINT n)
{
	TCHAR szBuf[128];

	if (n == (UINT)-1)
		wsprintf(szBuf, TEXT(GetStringFromRc(IDS_NONESTEPMINNUMBER)));
	else
		wsprintf(szBuf, TEXT(GetStringFromRc(IDS_STEPMINNUMBER)), n);
	SendMessage(hwndStatus, SB_SETTEXT, 2, (LPARAM)&szBuf);
}

void NewGame(HWND hwnd, UINT n)
{
	CHAR mapbuf[BOXNY][BOXNX];
#ifndef debug_msg
#define debug_msg(...)
#endif
#ifndef TEXT
#ifdef UNICODE
#define TEXT(s) L##s
#else
#define TEXT(s) s
#endif
#endif

	CHAR *s;
	UINT x, nx, ny;
	UINT man, dest, box;
	UINT i, j, y;

	if (n >= NMAP)
		n = 0;

	for (j = 0; j < BOXNY; j++)
		for (i = 0; i < BOXNX; i++)
			mapbuf[j][i] = NONE;

	s = boxmap[n];
	x = nx = ny = 0;
	man = dest = box = 0;

//	x = nx / ny; //???

	while (*s != TEXT('\0')) {
		switch (*s) {
		case DEST:
			dest++;
			goto normal;
		case BOXOUT:
			box++;
			goto normal;
		case BOXIN:
			dest++;
			box++;
			goto normal;
		case MANFRONT1AT:
			dest++;
			; // no break;
		case MANFRONT1:
			man++;
			manx = x;
			many = ny;
			if (man > 1) {
				debug_msg(TEXT("\007error: too many man at boxmap[%d] line %d, %d\n"), n, ny, x);
				return;
			}
			goto normal;
		case NONE:
		case WALL:
		case FLOOR:
normal:
			mapbuf[ny][x] = *s;
			x++;
			if (x > BOXNX) {
				debug_msg(TEXT("\007error: too many line dates at boxmap[%d] line %d\n"), n, ny);
				return;
			}
			break;
		case '\n':
			if (x > nx)
				nx = x;
			x = 0;
			ny++;
			if (ny > BOXNY) {
				debug_msg(TEXT("\007error: too many lines at boxmap[%d]\n"), n);
				return;
			}
			break;
		default:
			debug_msg(TEXT("\007error: char '%c' in boxmap[%d] line %d, %d\n"), *s, n, ny, x);
			return;
			break;
		}
		s++;
	}
	ny++;
	if (box != dest) {
		debug_msg(TEXT("\007error: boxmap[%d] has differict %d boxes and %d dest\n"), n, box, dest);
		return;
	}
	debug_msg(("OK! boxmap[%d] has %d lines with %d dates, and %d boxes, %d dest\n"), n, ny, nx, box, dest);

	for (j = 0; j < BOXNY; j++)
		for (i = 0; i < BOXNX; i++)
			map[j][i] = NONE;

	i = (BOXNX - nx) / 2;
	j = (BOXNY - ny) / 2;

	manx += i;
	many += j;

	sx = (nx % 2) ? (BOXCX / 2) : 0;
	sy = (ny % 2) ? (BOXCY / 2) : 0;

if (0) {
	TCHAR szBuf[100];

	wsprintf(szBuf, TEXT("nx = %d, ny = %d, sx = %d, sy = %d, i = %d, j = %d"), nx, ny, sx, sy, i, j);
	MessageBox(hwnd, szBuf, 0, 0);
}

	for (y = 0; y < ny; y++)
		for (x = 0; x < nx; x++)
			map[j + y][i + x] = mapbuf[y][x];

	nStep = 0;

	{
		TCHAR szBuf[64];

		wsprintf(szBuf, TEXT(GetStringFromRc(IDS_LEVELTITLE)), n + 1);
		SetWindowText(hwnd, szBuf);
		InvalidateRect(hwnd, NULL, TRUE);
		CheckUndoMenu(hwnd, FALSE);
		nStepMin = ReadBestMoves(nMap);
		ShowStepNumber(nStep);
		ShowStepMinNumber(nStepMin);
	}
}

void DoUndo(HWND hwnd)
{
	CheckUndoMenu(hwnd, FALSE);
	CopyMemory(map, mapu, sizeof(map));
	manx = manxu;
	many = manyu;
	ShowStepNumber(--nStep);
	InvalidateRect(hwnd, NULL, TRUE);
}

void MainWndProc_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id) {
	case IDM_UNDO:
		if (bUndo == TRUE)
			DoUndo(hwnd);
		break;
	case IDM_RESET:
		NewGame(hwnd, nMap);
		break;
	case IDM_JUMP:
		DialogBox(hInst, TEXT("JUMP"), hwnd, (DLGPROC)JumpDialogProc);
		break;
	case IDM_ABOUT:
		DialogBox(hInst, TEXT("ABOUT"), hwnd, (DLGPROC)AboutDialogProc);
		break;
	case IDM_EXIT:
		PostMessage(hwnd,WM_CLOSE,0,0);
		break;
	}
}

void MainWndProc_OnTimer(HWND hwnd, UINT idEvent)
{
	RECT rc;
	switch (map[many][manx]) {
	case MANFRONT1:
		map[many][manx] = MANFRONT2;
		break;
	case MANFRONT1AT:
		map[many][manx] = MANFRONT2AT;
		break;
	case MANBACK1:
		map[many][manx] = MANBACK2;
		break;
	case MANBACK1AT:
		map[many][manx] = MANBACK2AT;
		break;
	case MANLEFT1:
		map[many][manx] = MANLEFT2;
		break;
	case MANLEFT1AT:
		map[many][manx] = MANLEFT2AT;
		break;
	case MANRIGHT1:
		map[many][manx] = MANRIGHT2;
		break;
	case MANRIGHT1AT:
		map[many][manx] = MANRIGHT2AT;
		break;
	case MANFRONT2:
		map[many][manx] = MANFRONT1;
		break;
	case MANFRONT2AT:
		map[many][manx] = MANFRONT1AT;
		break;
	case MANBACK2:
		map[many][manx] = MANBACK1;
		break;
	case MANBACK2AT:
		map[many][manx] = MANBACK1AT;
		break;
	case MANLEFT2:
		map[many][manx] = MANLEFT1;
		break;
	case MANLEFT2AT:
		map[many][manx] = MANLEFT1AT;
		break;
	case MANRIGHT2:
		map[many][manx] = MANRIGHT1;
		break;
	case MANRIGHT2AT:
		map[many][manx] = MANRIGHT1AT;
		break;
	default:
		return;
	}
	SetRect(&rc, manx * BOXCX + sx, many * BOXCY + sy, (manx + 1) * BOXCX + sx, (many + 1) * BOXCY + sy);
	InvalidateRect(hwnd, &rc, TRUE);
}

BOOL doWin(HWND hwnd)
{
	UINT x, y;
	BOOL bWin = TRUE;

	for (y = 0; y < BOXNY && bWin == TRUE; y++)
		for (x = 0; x < BOXNX && bWin == TRUE; x++)
			if (map[y][x] == BOXOUT)
				bWin = FALSE;

	if (bWin == TRUE) {
		CheckUndoMenu(hwnd, FALSE);
#if 0
		MessageBox(hwnd, "WIN", GetStringFromRc(STR_00017), MB_ICONINFORMATION);
#endif
	}

	return bWin;
}

BOOL doMove(HWND hwnd, BOXSTATE *man, BOXSTATE *next, BOXSTATE *dest, UINT vk)
{
	BOOL bManMove = FALSE;
	BOOL bBoxMove = FALSE;
	BOOL bWin = FALSE;
	RECT rc;

	switch (*next) {
	case FLOOR:
	case DEST:
		bManMove = TRUE;
		break;
	case BOXIN:
	case BOXOUT:
		switch (*dest) {
		case FLOOR:
		case DEST:
			bBoxMove = TRUE;
			bManMove = TRUE;
			break;
		}
	}

	if (bBoxMove == TRUE) {
		CheckUndoMenu(hwnd, TRUE);
		CopyMemory(mapu, map, sizeof(map));
		manxu = manx;
		manyu = many;
		ShowStepNumber(++nStep);
	}

	if (bBoxMove == TRUE) {
		if (*dest == DEST)
			*dest = BOXIN;
		else
			*dest = BOXOUT;
		if (*next == BOXIN)
			*next = DEST;
		else
			*next = FLOOR;
	}

	if (bManMove == TRUE) {
		if (*next == DEST)
			switch (vk) {
			case VK_UP:
				if (*man == MANBACK1 || *man == MANBACK1AT)
					*next = MANBACK2AT;
				else
					*next = MANBACK1AT;
				many--;
				break;
			case VK_DOWN:
				if (*man == MANFRONT1 || *man == MANFRONT1AT)
					*next = MANFRONT2AT;
				else
					*next = MANFRONT1AT;
				many++;
				break;
			case VK_LEFT:
				if (*man == MANLEFT1 || *man == MANLEFT1AT)
					*next = MANLEFT2AT;
				else
					*next = MANLEFT1AT;
				manx--;
				break;
			case VK_RIGHT:
				if (*man == MANRIGHT1 || *man == MANRIGHT1AT)
					*next = MANRIGHT2AT;
				else
					*next = MANRIGHT1AT;
				manx++;
				break;
			}
		else
			switch (vk) {
			case VK_UP:
				if (*man == MANBACK1 || *man == MANBACK1AT)
					*next = MANBACK2;
				else
					*next = MANBACK1;
				many--;
				break;
			case VK_DOWN:
				if (*man == MANFRONT1 || *man == MANFRONT1AT)
					*next = MANFRONT2;
				else
					*next = MANFRONT1;
				many++;
				break;
			case VK_LEFT:
				if (*man == MANLEFT1 || *man == MANLEFT1AT)
					*next = MANLEFT2;
				else
					*next = MANLEFT1;
				manx--;
				break;
			case VK_RIGHT:
				if (*man == MANRIGHT1 || *man == MANRIGHT1AT)
					*next = MANRIGHT2;
				else
					*next = MANRIGHT1;
				manx++;
				break;
			}
		switch (*man) {
		case MANFRONT1:
		case MANBACK1:
		case MANLEFT1:
		case MANRIGHT1:
		case MANFRONT2:
		case MANBACK2:
		case MANLEFT2:
		case MANRIGHT2:
			*man = FLOOR;
			break;
		case MANFRONT1AT:
		case MANBACK1AT:
		case MANLEFT1AT:
		case MANRIGHT1AT:
		case MANFRONT2AT:
		case MANBACK2AT:
		case MANLEFT2AT:
		case MANRIGHT2AT:
			*man = DEST;
			break;
		}
	} else
		switch (*man) {
		case MANFRONT1:
		case MANBACK1:
		case MANLEFT1:
		case MANRIGHT1:
		case MANFRONT2:
		case MANBACK2:
		case MANLEFT2:
		case MANRIGHT2:
			switch (vk) {
			case VK_UP:
				*man = MANBACK1;
				break;
			case VK_DOWN:
				*man = MANFRONT1;
				break;
			case VK_LEFT:
				*man = MANLEFT1;
				break;
			case VK_RIGHT:
				*man = MANRIGHT1;
				break;
			}
			break;
		case MANFRONT1AT:
		case MANBACK1AT:
		case MANLEFT1AT:
		case MANRIGHT1AT:
		case MANFRONT2AT:
		case MANBACK2AT:
		case MANLEFT2AT:
		case MANRIGHT2AT:
			switch (vk) {
			case VK_UP:
				*man = MANBACK1AT;
				break;
			case VK_DOWN:
				*man = MANFRONT1AT;
				break;
			case VK_LEFT:
				*man = MANLEFT1AT;
				break;
			case VK_RIGHT:
				*man = MANRIGHT1AT;
				break;
			}
			break;
		}

	if (bManMove == TRUE)
		if (bBoxMove == TRUE) {
			switch (vk) {
			case VK_UP:
			case VK_DOWN:
				SetRect(&rc, manx * BOXCX + sx, (many - 1) * BOXCY + sy, (manx + 1) * BOXCX + sx, (many + 2) * BOXCY + sy);
				break;
			case VK_LEFT:
			case VK_RIGHT:
				SetRect(&rc, (manx - 1) * BOXCX + sx, many * BOXCY + sy, (manx + 2) * BOXCX + sx, (many + 1) * BOXCY + sy);
				break;
			}
		} else {
			switch (vk) {
			case VK_UP:
				SetRect(&rc, manx * BOXCX + sx, many * BOXCY + sy, (manx + 1) * BOXCX + sx, (many + 2) * BOXCY + sy);
				break;
			case VK_DOWN:
				SetRect(&rc, manx * BOXCX + sx, (many - 1) * BOXCY + sy, (manx + 1) * BOXCX + sx, (many + 1) * BOXCY + sy);
				break;
			case VK_LEFT:
				SetRect(&rc, manx * BOXCX + sx, many * BOXCY + sy, (manx + 2) * BOXCX + sx, (many + 1) * BOXCY + sy);
				break;
			case VK_RIGHT:
				SetRect(&rc, (manx - 1) * BOXCX + sx, many * BOXCY + sy, (manx + 1) * BOXCX + sx, (many + 1) * BOXCY + sy);
				break;
			}
		}
	else
		SetRect(&rc, manx * BOXCX + sx, many * BOXCY + sy, (manx + 1) * BOXCX + sx, (many + 1) * BOXCY + sy);
	InvalidateRect(hwnd, &rc, TRUE);

	if (bBoxMove == TRUE)
		bWin = doWin(hwnd);
	return bWin;
}

void MainWndProc_OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	BOOL bWin;

	switch (vk) {
	case VK_ESCAPE:
		ShowWindow(hwnd, SW_MINIMIZE);
		break;
	case VK_HOME:
		SendMessage(hwnd, WM_COMMAND, IDM_RESET, 0);
		break;
	case VK_BACK:
	case VK_DELETE:
		SendMessage(hwnd, WM_COMMAND, IDM_UNDO, 0);
		break;
	case VK_F1:
		SendMessage(hwnd, WM_COMMAND, IDM_ABOUT, 0);
		break;
	case VK_F12:
		SendMessage(hwnd, WM_COMMAND, IDM_EXIT, 0);
		break;
	case VK_NEXT:
		nMap++;
		if (nMap > nMapMax)
			nMap = 0;
		NewGame(hwnd, nMap);
		break;
	case VK_PRIOR:
		nMap--;
		if (nMap < 0)
			nMap = nMapMax;
		NewGame(hwnd, nMap);
		break;
	case VK_UP:
		bWin = doMove(hwnd, &map[many][manx], &map[many - 1][manx], &map[many - 2][manx], VK_UP);
		break;
	case VK_DOWN:
		bWin = doMove(hwnd, &map[many][manx], &map[many + 1][manx], &map[many + 2][manx], VK_DOWN);
		break;
	case VK_LEFT:
		bWin = doMove(hwnd, &map[many][manx], &map[many][manx - 1], &map[many][manx - 2], VK_LEFT);
		break;
	case VK_RIGHT:
		bWin = doMove(hwnd, &map[many][manx], &map[many][manx + 1], &map[many][manx + 2], VK_RIGHT);
		break;
	}

	if (bWin == TRUE) {
		if (nStep < nStepMin)
			WriteBestMoves(nMap, nStep);
		nMap++;
		if (nMap >= NMAP)
			nMap = 0;
		if (nMap > nMapMax) {
			nMapMax = nMap;
			WriteHighestStage(nMapMax + 1);
		}
		MessageBox(hwnd, TEXT(GetStringFromRc(IDS_WIN)), TEXT("Win"), MB_ICONINFORMATION);
		NewGame(hwnd, nMap);
	}
}

void MainWndProc_OnLButtonDown(HWND hwnd, BOOL bFlag, SHORT xPos, SHORT yPos, UINT fwKeys)
{
	INT mx, my;

	mx = xPos / BOXCX - manx;
	my = yPos / BOXCY - many;
	if (abs(my) > abs(mx)) {
		if (my > 0) {
			SendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 0);
		} else if (my < 0) {
			SendMessage(hwnd, WM_KEYDOWN, VK_UP, 0);
		}
	} else {
		if (mx > 0) {
			SendMessage(hwnd, WM_KEYDOWN, VK_RIGHT, 0);
		} else if (mx < 0) {
			SendMessage(hwnd, WM_KEYDOWN, VK_LEFT, 0);
		}
	}
}

BOOL SetWindowSize(HWND hwnd, UINT cx, UINT cy)
{
	RECT rc1, rc2;

	SetRect(&rc1, 0, 0, cx, cy);
	AdjustWindowRectEx(&rc1, WS_CAPTION | WS_BORDER, TRUE, 0);
	GetWindowRect(hwnd, &rc2);
	MoveWindow(hwnd, rc2.left, rc2.top, rc1.right - rc1.left, rc1.bottom -  rc1.top, TRUE);
	return TRUE;
}

BOOL MainWndProc_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	RECT rc;
	int ptArray[3];
#define NAME_WIDTH 200

	CoolMenu_Create(GetSubMenu(GetMenu(hwnd), 0), szAppName, NULL);
	hwndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_BORDER, NULL, hwnd, 0);
	GetClientRect(hwndStatus, &rc);
	SetWindowSize(hwnd, BOXCX * BOXNX, BOXCY * BOXNY + rc.bottom);
	MoveWindow(hwndStatus, 0, BOXCY * BOXNY, BOXCX * BOXNX, rc.bottom, TRUE);
	ptArray[0] = NAME_WIDTH;
	ptArray[1] = NAME_WIDTH + (BOXCX * BOXNX - NAME_WIDTH) / 2 - 16;
	ptArray[2] = -1;
	SendMessage(hwndStatus, SB_SETPARTS, sizeof(ptArray) / sizeof(ptArray[0]), (LPARAM)(LPINT)ptArray);
	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM)TEXT(GetStringFromRc(IDS_COPYRIGHT)));// <Box World! by Larry Li 2004.>
	LoadMapBitmap(hInst);
	SetTimer(hwnd, 100, 600, NULL);
	nMap = nMapMax = ReadHighestStage() - 1;
	NewGame(hwnd, nMap);
	return TRUE;
}

void MainWndProc_OnDestroy(HWND hwnd)
{
	CoolMenu_Cleanup(hwnd);
	KillTimer(hwnd, 100);
	FreeMapBitmap();
	PostQuitMessage(0);
}

LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch (msg) {
#ifdef SWITCH_MSG
#undef SWITCH_MSG
#endif
#define SWITCH_MSG(m,f) HANDLE_MSG(hwnd,WM_##m,MainWndProc_On##f)
	SWITCH_MSG(CREATE, Create);
	SWITCH_MSG(KEYDOWN, KeyDown);
	SWITCH_MSG(LBUTTONDOWN, LButtonDown);
	SWITCH_MSG(TIMER, Timer);
	SWITCH_MSG(PAINT, Paint);
	SWITCH_MSG(COMMAND, Command);
	SWITCH_MSG(DESTROY, Destroy);
#undef SWITCH_MSG
	case WM_MEASUREITEM:
		CoolMenu_MeasureItem(wParam, lParam);
		return 0;
	case WM_DRAWITEM:
		CoolMenu_DrawItem(wParam, lParam);
		return 0;
	case WM_CONTEXTMENU:
		CoolMenu_PopupContext(hwnd, lParam, 0);
		return 0;
	default:
		return DefWindowProc(hwnd,msg,wParam,lParam);
	}
}

HRESULT ActivatePreviousInstance(const TCHAR* pszClass, const TCHAR* pszTitle, BOOL* pfActivated)
{
	HRESULT hr = S_OK;
	int cTries;
	HANDLE hMutex = NULL;

	*pfActivated = FALSE;
	cTries = 5;
	while (cTries > 0) {
		hMutex = CreateMutex(NULL, FALSE, pszClass);
		if (NULL == hMutex) {
			hr = E_FAIL;
			goto Exit;
		}

		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			HWND hwnd;

			CloseHandle(hMutex);
			hMutex = NULL;

			hwnd = FindWindow(pszClass, pszTitle);
			if (NULL == hwnd) {
				Sleep(500);
				hwnd = FindWindow(pszClass, pszTitle);
			}

			if (NULL != hwnd) {
				if (IsIconic(hwnd))
					ShowWindow(hwnd, SW_RESTORE);
				SetForegroundWindow(hwnd);
				*pfActivated = TRUE;
				break;
			}

			cTries--;
		} else
			break;
	}

	if (cTries <= 0) {
		hr = E_FAIL;
		goto Exit;
	}

Exit:
	return hr;
}

struct ExceptDialogParam {
	LPSTR szExceptionInfo;
	PVOID pExceptionAddress;
};

static BOOL CALLBACK ExceptDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct ExceptDialogParam *e = (struct ExceptDialogParam *)lParam;
	TCHAR szBuf[512];

	switch (msg) {
	case WM_INITDIALOG:
		SetDlgItemText(hwnd, 100, IDI_HAND);
		SetDlgItemText(hwnd, 101, e->szExceptionInfo);
		wsprintf(szBuf, TEXT("0x%p"), e->pExceptionAddress);
		SetDlgItemText(hwnd, 102, szBuf);
		EnableLinkText(hwnd);
		MessageBeep(MB_ICONHAND);
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	case WM_CLOSE:
		EndDialog(hwnd,0);
		return 1;
	}
	return FALSE;
}
#ifndef STDCALL
#define STDCALL __stdcall
#endif
static LONG STDCALL MyExceptionFilter(LPEXCEPTION_POINTERS p)
{
	struct ExceptDialogParam e;
	#define s e.szExceptionInfo

	switch (p->ExceptionRecord->ExceptionCode) {
	case EXCEPTION_ACCESS_VIOLATION:
		s = "The thread tried to read from or write to a virtual "
 			"address for which it does not have the appropriate access.";
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		s = "The thread tried to access an array element that is out of "
			"bounds and the underlying hardware supports bounds checking.";
		break;
	case EXCEPTION_BREAKPOINT:
		s = "A breakpoint was encountered.";
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		s = "The thread tried to read or write data that is misaligned "
			"on hardware that does not provide alignment. For example, "
			"16-bit values must be aligned on 2-byte boundaries; 32-bit "
			"values on 4-byte boundaries, and so on.";
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		s = "One of the operands in a floating-point operation is "
			"denormal. A denormal value is one that is too small to "
			"represent as a standard floating-point value.";
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		s = "The thread tried to divide a floating-point value by "
			"a floating-point divisor of zero.";
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		s = "The result of a floating-point operation cannot be "
			"represented exactly as a decimal fraction.";
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		s = "This exception represents any floating-point exception "
			"not included in this list.";
		break;
	case EXCEPTION_FLT_OVERFLOW:
		s = "The exponent of a floating-point operation is greater "
			"than the magnitude allowed by the corresponding type.";
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		s = "The stack overflowed or underflowed as the result of "
			"a floating-point operation.";
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		s = "The exponent of a floating-point operation is less "
			"than the magnitude allowed by the corresponding type.";
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		s = "The thread tried to execute an invalid instruction.";
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		s = "The thread tried to access a page that was not "
			"present, and the system was unable to load the page. "
			"For example, this exception might occur if a network "
			"connection is lost while running a program over the network.";
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		s = "The thread tried to divide an integer value by an "
			"integer divisor of zero.";
		break;
	case EXCEPTION_INT_OVERFLOW:
		s = "The result of an integer operation caused a carry "
			"out of the most significant bit of the result.";
		break;
	case EXCEPTION_INVALID_DISPOSITION:
		s = "An exception handler returned an invalid disposition "
			"to the exception dispatcher. Programmers using a "
			"high-level language such as C should never encounter this exception.";
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		s = "The thread tried to execute an instruction whose "
			"operation is not allowed in the current machine mode.";
		break;
	case EXCEPTION_SINGLE_STEP:
		s = "A trace trap or other single-instruction mechanism "
			"signaled that one instruction has been executed.";
		break;
	case EXCEPTION_STACK_OVERFLOW:
		s = "The thread used up its stack.";
		break;
	default:
		s = "Unkown exception";
		break;
	}

	e.pExceptionAddress = p->ExceptionRecord->ExceptionAddress;

	{
		WNDCLASS wc;

		memset(&wc, 0, sizeof(WNDCLASS));
		wc.lpfnWndProc = DefDlgProc;
		wc.cbWndExtra = DLGWINDOWEXTRA;
		wc.hInstance = hInst;
		wc.lpszClassName = TEXT("EXCEPT");
		wc.hCursor = LoadCursor(NULL,IDC_ARROW);
		wc.hIcon = LoadIcon(NULL, IDI_HAND);
		RegisterClass(&wc);
		DialogBoxParam(hInst, TEXT("EXCEPT"), NULL, (DLGPROC)ExceptDialogProc, (LPARAM)&e);
	}

	return EXCEPTION_EXECUTE_HANDLER;
//	return EXCEPTION_CONTINUE_EXECUTION;
//	return EXCEPTION_CONTINUE_SEARCH;

	#undef e
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	MSG msg;
//	HANDLE hAccelTable;
    BOOL fActivated;

	SetUnhandledExceptionFilter(&MyExceptionFilter);

	hInst = hInstance;
    if(FAILED(ActivatePreviousInstance(szAppName, NULL, &fActivated)) || fActivated)
        return 0;

	InitCommonControls();

	if (!InitApplication())
		return 0;
//	hAccelTable = LoadAccelerators(hInst,MAKEINTRESOURCE(IDACCEL));
	if ((hwndMain = CreateboxworldWndClassWnd()) == (HWND)0)
		return 0;
	ShowWindow(hwndMain, nCmdShow);
	UpdateWindow(hwndMain);
	while (GetMessage(&msg,NULL,0,0)) {
//		if (!TranslateAccelerator(msg.hwnd,hAccelTable,&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
//		}
	}
	return msg.wParam;
}

