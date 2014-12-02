/* COOL MENU
// Owner drawn menu adapted by Mike Caetano 1/2002
// from source by James Brown, catch22.uk.net
//
// wndproc messages
//
//	hmenu = GetMenu(hwnd);
//
//	case WM_CREATE :
//		hsub  = GetSubMenu(hmenu, submenuindex);
//		CoolMenu_Create(hsub, vertlable);
//
//	case WM_DESTROY :
//		CoolMenu_Cleanup(hwnd);
//
//	case WM_MEASUREITEM :
//		CoolMenu_MeasureItem(wParam, lParam);
//
//	case WM_DRAWITEM :
//		CoolMenu_DrawItem(wParam, lParam);
//
//	case WM_CONTEXTMENU :
//		CoolMenu_PopupContext(hwnd, lParam, index);
//
//	case WM_COMMAND :
//		case IDM_CMD :
//			CoolMenu_SetCheck(hwnd, IDM_CMD);
*/
#include <windows.h>
#include <tchar.h>
#include "coolmenu.h"
#include "guiddef.h"

#ifdef __LCC__
#define COLOR_HOTLIGHT                   26
#define COLOR_GRADIENTACTIVECAPTION      27
#define COLOR_GRADIENTINACTIVECAPTION    28
#define COLOR_TWENTYFIVE                 25 // not official - darker gray
#endif

#define CXMENUADJUST 2		// border for bitmap in pixels
#define CYMENUADJUST 2

#define MAX_MENUITEM_TEXT 64

static const GUID COOLMENU_GUID = {0x44de039e, 0x73ac, 0x4025, {0xa3, 0x3f, 0x99, 0x7, 0xc, 0x4e, 0x40, 0x23}};

// User defined structure to associate with each menu item
typedef struct {
	GUID guid;			// indicates that a menu has been "coolified"
	HBITMAP hbmp_ref;	// each menu item will share a handle to the same bitmap
	UINT type;			// MF_SEPERATOR etc
	int bitmapy;		// position in the bitmap that we take the picture from
	int bitmapwidth;	// width of the bitmap in the menu
	int itemheight;		// height of this particular item
	int itemwidth;		// width of this particular item
	int menuwidth;		// each item also needs to know how big to make the menu
	TCHAR szText[MAX_MENUITEM_TEXT];
} CoolMenuItemData;

// Local Prototypes
static HFONT CoolMenu_CreateAngledFont(int fontHeight, int fontAngle, LPCTSTR lpfontFace);
static HMENU CoolMenu_FindMenuFromID(HMENU hMenu, UINT id);
static BOOL CoolMenu_PrintLable(HDC hdc, HFONT hFont, RECT rc, LPCTSTR lpszLable);
static BOOL CoolMenu_GradientFill(HDC hdc, int iWidth, int iHeight);
static VOID CoolMenu_DrawCheck(HDC hdc, RECT *rc, BOOL bSelected);
static VOID CoolMenu_RemoveData(HMENU hmenu);

// called remotely from case WM_CONTEXTMENU
// really has nothing to do with the "CoolMenu" effect
LRESULT CoolMenu_PopupContext(HWND hwnd, LPARAM lParam, int index)
{
	if (!IsWindow(hwnd))
		return FALSE;

	int xPos = LOWORD(lParam);
	int yPos = HIWORD(lParam);
	HMENU hmenu = GetMenu(hwnd);
	HMENU hsub = GetSubMenu(hmenu, index);

	return (LRESULT) TrackPopupMenuEx(hsub, TPM_RIGHTBUTTON, xPos, yPos, hwnd, NULL);
}

// case WM_MEASUREITEM
LRESULT CoolMenu_MeasureItem(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 0) {
		MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;

		// check owner drawn type
		if (mis->CtlType == ODT_MENU) {
			// cast mis.dwItemData as CoolMenuItemData
			CoolMenuItemData *cmid = (CoolMenuItemData *)mis->itemData;
			// confirm cool menu guid
			if (CoolMenu_IsEqualGUID(cmid->guid, COOLMENU_GUID)) {
				mis->itemHeight = cmid->itemheight;
				mis->itemWidth  = cmid->itemwidth;
				return TRUE;
			}
		}
	}
	return FALSE;
}

static VOID CoolMenu_RemoveData(HMENU hmenu)
{
	MENUITEMINFO mii;

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_DATA;

	if (GetMenuItemInfo(hmenu, 0, TRUE, &mii)) {
		// if flag indicates owner draw
		if (mii.fType == MFT_OWNERDRAW) {
			// cast mii.dwItemData as CoolMenuItemData
			CoolMenuItemData *cmid = (CoolMenuItemData *)mii.dwItemData;
			// check for cool menu guid
			if (CoolMenu_IsEqualGUID(cmid->guid, COOLMENU_GUID)) {
				// delete the bitmap
				DeleteObject(cmid->hbmp_ref);
				// release memory
				HeapFree(GetProcessHeap(), 0, cmid);
			}
		}
	}
}

VOID CoolMenu_Cleanup(HWND hwnd)
{
	if (!IsWindow(hwnd)) {
		return;
	}

	HMENU hMenu = GetMenu(hwnd);
	HMENU hSubMenu = NULL;
	int MenuCount =	GetMenuItemCount(hMenu);

	for (int i = 0; i < MenuCount; i++) {
		hSubMenu = GetSubMenu(hMenu, i);
		if (NULL != hSubMenu)
			CoolMenu_RemoveData(hSubMenu);
	}
}

// recursive function
static HMENU CoolMenu_FindMenuFromID(HMENU hMenu, UINT id)
{
	HMENU hSubMenu = NULL;
	int iCount = GetMenuItemCount(hMenu);

	for (int i = 0; i < iCount; i++) {
		if (id == GetMenuItemID(hMenu, i))
			return hMenu;
		else {
			hSubMenu = GetSubMenu(hMenu, i);
			if (NULL != hSubMenu) {
				// recurse
				hSubMenu = CoolMenu_FindMenuFromID(hSubMenu, id);
				if (NULL != hSubMenu) {
					return hSubMenu;
				}
			}
		}
	}
	// no match
	return NULL;
}

// called remotely from case WM_COMMAND subcase idm
VOID CoolMenu_SetCheck(HWND hwnd, UINT idm)
{
	HMENU hTargetMenu = NULL;
	HMENU hMenu = GetMenu(hwnd);

	if (NULL != hMenu)
		hTargetMenu = CoolMenu_FindMenuFromID(hMenu, idm);

	UINT uFlags = MF_BYCOMMAND;
	UINT uState = GetMenuState(hTargetMenu, idm, uFlags);
	uFlags |= ( MF_CHECKED & uState ) ? MF_UNCHECKED : MF_CHECKED;
	CheckMenuItem(hTargetMenu, idm, uFlags);
}

// called remotely from case WM_COMMAND
VOID CoolMenu_SetEnabled(HWND hwnd, UINT idm)
{
	HMENU hTargetMenu = NULL;
	HMENU hMenu = GetMenu(hwnd);

	if (NULL != hMenu)
		hTargetMenu = CoolMenu_FindMenuFromID(hMenu, idm);

	UINT uFlags = MF_BYCOMMAND;
	UINT uState = GetMenuState(hTargetMenu, idm, uFlags);
	uFlags |= ( MF_GRAYED & uState ) ? MF_ENABLED : MF_GRAYED;
	EnableMenuItem(hTargetMenu, idm, uFlags);
}

static HFONT CoolMenu_CreateAngledFont(int fontHeight, int fontAngle, LPCTSTR lpfontFace)
{
	// prep logical font
	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	lf.lfHeight = fontHeight;
	lf.lfEscapement = (10 * fontAngle);   // 90' ccw
	lf.lfOrientation = (10 * fontAngle);  // 90' ccw
	lf.lfWeight = FW_SEMIBOLD;	// 600
	lf.lfCharSet = DEFAULT_CHARSET; // ANSI_CHARSET; //Larry Li[Wuhan], 2002.12.22
	lf.lfQuality = ANTIALIASED_QUALITY;
	lstrcpy(lf.lfFaceName, lpfontFace);
	// instance logical font
	return CreateFontIndirect(&lf);
}

static BOOL CoolMenu_PrintLable(HDC hdc, HFONT hFont, RECT rc, LPCTSTR lpszLable)
{
	BOOL ok = FALSE;
	HFONT hfntPrev;
	COLORREF prevColor;

	// set the background mode to transparent
	SetBkMode(hdc, TRANSPARENT);
	// set text color
	prevColor = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
	// select font into dc
	hfntPrev = SelectObject(hdc, hFont);

	ok = TextOut(hdc, CXMENUADJUST, rc.bottom - CXMENUADJUST - CXMENUADJUST, lpszLable, lstrlen(lpszLable));

	// restore previous font
	SelectObject(hdc, hfntPrev);
	// restore previous text color
	SetTextColor(hdc, prevColor);
	// restore previous background mode
	SetBkMode(hdc, OPAQUE);

	return ok;
}

// Additional Considerations
// maybe pass in the HDC instead of the HWND
// pass in starting and ending colors
// pass in string lable
static BOOL CoolMenu_GradientFill(HDC hdc, int iWidth, int iHeight)
{
	BOOL ok = FALSE;
#ifndef STDCALL
#define STDCALL __stdcall
#endif
	BOOL (STDCALL *pfnGradientFill)(HDC,PTRIVERTEX,ULONG,PVOID,ULONG,ULONG);

	// "msimg32.dll", "GradientFill", Not available on W95/NT4 -> W98/W2K Only
	HINSTANCE hLibrary = LoadLibrary("msimg32.dll");
	if (NULL == hLibrary)
		return ok;

	pfnGradientFill = GetProcAddress(hLibrary,"GradientFill");
	if (NULL == pfnGradientFill) {
		FreeLibrary(hLibrary);
		return ok;
	}

	// establish starting color
	DWORD rgb0 = GetSysColor(COLOR_GRADIENTACTIVECAPTION);

	// establish ending color
	DWORD rgb1 = GetSysColor(COLOR_ACTIVECAPTION);

	// setup gradient fill array
	TRIVERTEX vert[2];

	// starting
	vert[0].x      = 0;
	vert[0].y      = 0;
	vert[0].Red    = MAKEWORD(0,GetRValue(rgb0));
	vert[0].Green  = MAKEWORD(0,GetGValue(rgb0));
	vert[0].Blue   = MAKEWORD(0,GetBValue(rgb0));
	vert[0].Alpha  = 0x0000;

	// ending
	vert[1].x      = iWidth;
	vert[1].y      = iHeight;
	vert[1].Red    = MAKEWORD(0,GetRValue(rgb1));
	vert[1].Green  = MAKEWORD(0,GetGValue(rgb1));
	vert[1].Blue   = MAKEWORD(0,GetBValue(rgb1));
	vert[1].Alpha  = 0x0000;

	GRADIENT_RECT gRect;
	gRect.UpperLeft  = 0;
	gRect.LowerRight = 1;

	ok = pfnGradientFill(hdc, vert, 2, &gRect, 1, GRADIENT_FILL_RECT_V);

	ok = FreeLibrary(hLibrary);

	return ok;

}

// CoolMenu_Create(hwnd, lptstr, index);
//	Convert the specified menu popup into a "cool menu"
//	Bitmap can be any colour depth, but no palette support is provided
//	Better use 16 colour (4bit) for now
BOOL CoolMenu_Create(HMENU hmenu, LPCTSTR lpVertLable, LPCTSTR lpFontName)	//Larry Li[Wuhan], 2002.12.22
{

	MENUITEMINFO mii;

	HANDLE hOldFontObject;
	SIZE sz;
	HDC hdc;

	int nMaxWidthL = 0;		// track maximum width values
	int nMaxWidthR = 0;

	int menuheight = 0;		// start menu height at zero
	int menucount = 0;		// count items on menu

	int bmp_height = 0;	// derive these
	int bmp_width = 0;

	// these remain constant...
	int MenuCheck_UnitWidth = GetSystemMetrics(SM_CXMENUCHECK);
	MenuCheck_UnitWidth += ( CXMENUADJUST + CXMENUADJUST );

	int MenuItem_UnitHeight = GetSystemMetrics(SM_CYMENU);
	MenuItem_UnitHeight += CYMENUADJUST;

	int MenuItem_HalfHeight = MenuItem_UnitHeight >> 1;

	// if there are no menu items we ought to bail...
	menucount = GetMenuItemCount(hmenu);
	// dwErrorCode = ( menucount == -1 ) ? GetLastError() : 0;

	// Primary Activity Of This Function
	// Allocate an ARRAY of custom menu structures.
	// store a pointer to each item in each menu's dwItemData area.

	CoolMenuItemData *cmid = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CoolMenuItemData) * menucount);

	// get default font
	hdc = GetDC(NULL);
	hOldFontObject = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

	// Pointers for managing accelerator text
	TCHAR *szText, *szPtr;

	// store information for each menu item
	for (int i = 0; i < menucount; i++) {
		// mymi[i].szText[0] = _T('\0');
		cmid[i].guid = COOLMENU_GUID;

		mii.cbSize		= sizeof(mii);
		mii.cch			= MAX_MENUITEM_TEXT;
		mii.fMask		= MIIM_TYPE;
		mii.fType		= MFT_STRING;
		mii.dwTypeData	= cmid[i].szText;

		//get the menu text
		GetMenuItemInfo(hmenu, i, TRUE, &mii);

		// store item type to use in second loop
		cmid[i].type = mii.fType;

		// store the height value for the item in the cmid struct
		// if we have a separator, reduce the increment
		cmid[i].itemheight = (mii.fType & MF_SEPARATOR) ? MenuItem_HalfHeight : MenuItem_UnitHeight;

		// accumulate menuheight
		menuheight += cmid[i].itemheight;

		// distinguish between menu text and accelerator text

		// a TAB character in the menu string marks the split
		// set accelerator text to right aligned

		// work out the maximum width of the menu texts
		// work out the maximum width of the accelerator text
		// keep those values separately

		// consider incorporating the assumption that there are two parts into the cmid structure
		// that is - determine all of the future parameters at creation leaving only those
		// generated during usage to the usage function
		// ie. "touch once"

		szText = cmid[i].szText;
		szPtr  = strchr(szText, _T('\t'));

		// work out the longest string of text
		if (szPtr) {
			// calculate the size of the text (left hand part of split)
			GetTextExtentPoint32(hdc, szText, szPtr - szText, &sz);
			nMaxWidthL = max(nMaxWidthL, sz.cx);

			// calculate the size of the accelerator text (right aligned)
			GetTextExtentPoint32(hdc, szPtr + 1, lstrlen(szPtr + 1), &sz);
			nMaxWidthR = max(nMaxWidthR, sz.cx);
		} else {
			//if there is no accelerator, just work out the size of the text as it is.
			GetTextExtentPoint32(hdc, szText, lstrlen(szText), &sz);
			nMaxWidthL = max(nMaxWidthL, sz.cx);
		}

	}

	// restore previous dc font
	SelectObject(hdc, hOldFontObject);

	// bitmap height should now be available
	bmp_height = menuheight;

	int PointSize = 16;
	int DPI = 72;
	int lfHeight = -MulDiv(PointSize, GetDeviceCaps(hdc, LOGPIXELSY), DPI);

	//Larry Li[Wuhan], 2002.12.22
	if (NULL == lpFontName)
		lpFontName = "Microsoft Sans Serif";
//	HFONT hfnt = CoolMenu_CreateAngledFont(lfHeight, 90, "Microsoft Sans Serif");
	HFONT hfnt = CoolMenu_CreateAngledFont(lfHeight, 90, lpFontName);

	bmp_width = -lfHeight;
	bmp_width += CXMENUADJUST;
	bmp_width += CXMENUADJUST;

	// create memdc and build bitmap from it
	HDC memdc = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, bmp_width, bmp_height);
	HBITMAP hOldBmp = (HBITMAP)SelectObject(memdc, hBitmap);
	BitBlt(memdc, 0, 0, bmp_width, bmp_height, hdc, 0, 0, SRCCOPY);

	// fill bmp with gradient
	BOOL grad_ok = CoolMenu_GradientFill(memdc, bmp_width, bmp_height);

	// rotate and draw text string
	RECT rc;
	ZeroMemory(&rc, sizeof(rc));
	rc.right = bmp_width;
	rc.bottom = bmp_height;

	// print vertical lable on top of gradient
	BOOL lable_ok = CoolMenu_PrintLable(memdc, hfnt, rc, lpVertLable);

	// delete font
	DeleteObject(hfnt);

	// clean up gdi
	hBitmap = (HBITMAP)SelectObject(memdc, hOldBmp);
	DeleteDC(memdc);
	DeleteObject(hOldBmp);
	ReleaseDC(NULL, hdc);

	if (!grad_ok && !lable_ok) {
		DeleteObject(hBitmap);
		hBitmap = NULL;
	}

	// a NULL bitmap should not be displayed...
	bmp_width = (NULL != hBitmap) ? bmp_width : 0;


	// in order to determine maxL and maxR we have to loop through every menuitem
	// in order to set each menuitem we have to loop through them all again
	for (int i = 0; i < menucount; i++) {

		cmid[i].hbmp_ref = hBitmap;
		cmid[i].bitmapwidth = bmp_width;
		cmid[i].menuwidth = 1 + MenuCheck_UnitWidth + bmp_width;

		// work out the source y-coordinate of each item in the bitmap
		cmid[i].bitmapy = bmp_height - menuheight;
		menuheight -= cmid[i].itemheight;

		// maximum values
		cmid[i].menuwidth += ( nMaxWidthL + nMaxWidthR );
		cmid[i].itemwidth = ( cmid[i].type & MF_SEPARATOR ) ? 0 : cmid[i].menuwidth;

		// make each menu item owner-drawn
		mii.fMask = MIIM_TYPE | MIIM_DATA;
		mii.fType = MFT_OWNERDRAW;
		mii.dwItemData = (ULONG)&cmid[i];

		SetMenuItemInfo(hmenu, i, TRUE, &mii);

	} // end loop

	return TRUE;
}

// called by DrawItem
static VOID CoolMenu_DrawCheck(HDC hdc, RECT *rc, BOOL bSelected)
{
	HBITMAP hbmCheck;
	int cx, cy;
	BITMAP bm;
	RECT  rcDest = *rc;
	POINT p;
	SIZE  delta;

	HDC memdc;
	HBITMAP hOldBM;
	COLORREF colorOld;
#ifndef OBM_CHECK
#define OBM_CHECK 32760
#endif
	hbmCheck = LoadBitmap(NULL, MAKEINTRESOURCE(OBM_CHECK));

	// center bitmap in caller's rectangle
	GetObject(hbmCheck, sizeof(bm), &bm);

	cx = bm.bmWidth;
	cy = bm.bmHeight;

	delta.cx = ( rc->right - rc->left - cx ) / 2;
	delta.cy = ( rc->bottom - rc->top - cy ) / 2;

	if (rc->right - rc->left > cx) {
		SetRect(&rcDest, rc->left + delta.cx, rc->top + delta.cy, 0, 0);
		rcDest.right = rcDest.left + cx;
		rcDest.bottom = rcDest.top + cy;
		p.x = 0;
		p.y = 0;
	} else {
		p.x = -delta.cx;
		p.y = -delta.cy;
	}

	// select checkmark into memory DC
	memdc = CreateCompatibleDC(hdc);
	hOldBM = (HBITMAP)SelectObject(memdc, hbmCheck);

	// set BG color based on selected state
	colorOld = SetBkColor(hdc, GetSysColor( bSelected ? COLOR_ACTIVECAPTION : COLOR_MENU ) );

	BitBlt(hdc, rcDest.left, rcDest.top, rcDest.right - rcDest.left, rcDest.bottom - rcDest.top, memdc, p.x, p.y, SRCCOPY);

	// restore background color
	SetBkColor(hdc, colorOld);
	// clean up memory device context
	SelectObject(memdc, hOldBM);
	DeleteDC(memdc);
	DeleteObject(hbmCheck);

	// draw pushed-in highlight.
#ifdef MENUCHECKBORDERS
	InflateRect(&rcDest, 0,-1);
	DrawEdge(hdc, &rcDest, BDR_SUNKENOUTER, BF_RECT);
#endif

}

// case WM_DRAWITEM
// This function is a mess - isolate the changes for each condition
LRESULT CoolMenu_DrawItem(WPARAM wParam, LPARAM lParam)
{
	if(wParam == 0) {
		// cast
		DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;

		// check owner drawn type
		if (dis->CtlType == ODT_MENU) {

			// cast dis->dwItemData as CoolMenuItemData
			CoolMenuItemData *cmid = (CoolMenuItemData *)dis->itemData;

			// check for cool menu guid
			if (CoolMenu_IsEqualGUID(cmid->guid, COOLMENU_GUID)) {

				HDC hdcmem;
				HDC hOldDC;
				RECT rect;
				RECT r;

				int MenuCheck_UnitWidth = 0;

				// set initial color ids
				int iTargetTextColor = 0;
				int iTargetBkColor = 0;

				BOOL bSelected = FALSE;
				BOOL bGrayed = FALSE;

				// these remain constant...
				MenuCheck_UnitWidth = GetSystemMetrics(SM_CXMENUCHECK);

				// create a mem dc
				hdcmem = CreateCompatibleDC(NULL);
				hOldDC = (HDC)SelectObject(hdcmem, cmid->hbmp_ref);

				//rect.left might not be zero, because of multi-column menus!
				rect = dis->rcItem;
				rect.right = rect.left + cmid->bitmapwidth;

				if (cmid->bitmapy < 0) {
					r = dis->rcItem;
					r.right = cmid->bitmapwidth;
					FillRect(dis->hDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
				} else {

					// blit memdc to hdc
					BitBlt(dis->hDC
						, rect.left, rect.top
						, cmid->bitmapwidth, rect.bottom - rect.top
						, hdcmem
						, 0
						, cmid->bitmapy
						, SRCCOPY);

				}

				SelectObject(hdcmem, hOldDC);
				DeleteDC(hdcmem);

				// does this menu item have mouse focus ?
				bSelected = (BOOL)( dis->itemState & ODS_SELECTED );

				// is this menu item disabled ?
				bGrayed = (BOOL)( dis->itemState & ODS_GRAYED );

				// SEPARATOR
				if (cmid->type & MFT_SEPARATOR) {
					rect.left = rect.right;
					rect.right = dis->rcItem.right;
					rect.top += ( rect.bottom - rect.top ) / 2 - 1;
					DrawEdge(dis->hDC, &rect, EDGE_ETCHED, BF_TOP);
				} else {

					// NOT SEPARATOR

					iTargetBkColor = bSelected ? COLOR_ACTIVECAPTION : COLOR_MENU;

					if (bSelected) {
						iTargetTextColor = bGrayed ? COLOR_MENU : COLOR_HIGHLIGHTTEXT;
					} else {
						iTargetTextColor = bGrayed ? COLOR_3DHIGHLIGHT : COLOR_MENUTEXT;
					}

					// set menu item text color
					SetTextColor(dis->hDC, GetSysColor(iTargetTextColor));
					// set menu item background color
					SetBkColor(dis->hDC, GetSysColor(iTargetBkColor));
					// set background mode
					SetBkMode(dis->hDC, OPAQUE);

					// these must preceed ExtTextOut
					rect.left = rect.right;
					rect.right = dis->rcItem.right;

					ExtTextOut(dis->hDC, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);

					// adjust for menu check
					rect.left += ( 1 + MenuCheck_UnitWidth + CXMENUADJUST );
					rect.top += 1;

					// search for a TAB character in the menu text.
					// If we find one, then this item has a menu accelerator
					TCHAR *tabptr = strchr(cmid->szText, _T('\t'));

					if (tabptr != 0) {
						// TAB
						//need to alter this code. Presently, accelerators
						//are right-aligned. Really, they should be left-aligned, but
						//on the right-hand side, after the normal menu text.

						// SELECTED OR NOT GRAYED
						// EVALUATES TRUE WHEN
						// SEL && NOT GRAYED
						// SEL && GRAYED
						// NOT SEL && NOT GRAYED
						// EVALUATES FALSE WHEN
						// NOT SEL && GRAY

						if (bSelected || !bGrayed) {
							// normal menu text
							DrawTextEx(dis->hDC, cmid->szText, tabptr - cmid->szText, &rect, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS, NULL);

							// adjust right margin to size of menu check
							rect.right -= MenuCheck_UnitWidth;

							// accelerator text
							DrawTextEx(dis->hDC, tabptr + 1, -1, &rect, DT_RIGHT | DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS, NULL);

						} else {

							// if ( !bODSSelected && bODSGrayed ) {
							// NOT SELECTED AND GRAYED

							// !bODSSelected || bODSGrayed
							// Drop Shadow Text

							// offset
							OffsetRect(&rect, 1, 1);
							// color
							SetTextColor(dis->hDC, GetSysColor(COLOR_3DHIGHLIGHT));
							// modality
							SetBkMode(dis->hDC, OPAQUE);

							// normal menu text
							DrawTextEx(dis->hDC, cmid->szText, tabptr - cmid->szText, &rect, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS, NULL);

							rect.right -= MenuCheck_UnitWidth;

							// accelerator text
							DrawTextEx(dis->hDC, tabptr + 1, -1, &rect, DT_RIGHT | DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS, NULL);
							// DT_RIGHT |

							// Foreground Text

							rect.right += MenuCheck_UnitWidth;

							// offset
							OffsetRect(&rect, -1, -1);
							// color
							SetTextColor(dis->hDC, GetSysColor(COLOR_3DSHADOW));

							// modality
							SetBkMode(dis->hDC, TRANSPARENT);

							// normal menu text
							DrawTextEx(dis->hDC, cmid->szText, tabptr - cmid->szText, &rect, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS, NULL);

							rect.right -= MenuCheck_UnitWidth;

							// accelerator text
							DrawTextEx(dis->hDC, tabptr + 1, -1, &rect, DT_RIGHT | DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS, NULL);
							// DT_RIGHT |
							// modality
							SetBkMode(dis->hDC, OPAQUE);

						}

					} else {

						// NO TAB
						// otherwise we have a standard menu item
						// standard menu item has no tab char
						// SELECTED OR NOT GRAYED
						// EVALUATES TRUE WHEN
						// SEL && NOT GRAYED
						// SEL && GRAYED
						// NOT SEL && NOT GRAYED
						// EVALUATES FALSE WHEN
						// NOT SEL && GRAY

						if (bSelected || !bGrayed) {
							// SELECTED OR NOT GRAYED
							DrawTextEx(dis->hDC, cmid->szText, -1, &rect, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS, NULL);

						} else  {
							// GRAYED

							// hdc, offset, color, mode, string, rect

							// offset
							OffsetRect(&rect, 1, 1);
							// color
							SetTextColor(dis->hDC, GetSysColor(COLOR_3DHIGHLIGHT));
							// modality
							SetBkMode(dis->hDC, OPAQUE);

							// drop shadow
							DrawTextEx(dis->hDC, cmid->szText, -1, &rect, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS, NULL);

							// offset
							OffsetRect(&rect, -1, -1);
							// color
							SetTextColor(dis->hDC, GetSysColor(COLOR_3DSHADOW));
							// modality
							SetBkMode(dis->hDC, TRANSPARENT);

							// foreground
							DrawTextEx(dis->hDC, cmid->szText, -1, &rect, DT_SINGLELINE | DT_VCENTER | DT_EXPANDTABS, NULL);

							// modality
							SetBkMode(dis->hDC, OPAQUE);

						}

					} // tab split

					// finally draw the check mark
					if (MF_CHECKED & GetMenuState((HMENU)dis->hwndItem, dis->itemID, MF_BYCOMMAND)) {

						r = dis->rcItem;
						r.left += cmid->bitmapwidth;
						r.left += CXMENUADJUST;
						r.right = r.left + MenuCheck_UnitWidth;

						CoolMenu_DrawCheck(dis->hDC, &r, bSelected );
					}
				} // separator
				return TRUE;
			} // guid
		} // odt
	} // wParam
	return FALSE;
}
