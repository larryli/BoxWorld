#ifndef __COOLMENU_H__
#define __COOLMENU_H__
#include <windows.h>

BOOL CoolMenu_Create(HMENU hmenu, LPCTSTR lpVertLable, LPCTSTR lpFontName);
VOID CoolMenu_Cleanup(HWND hwnd);
VOID CoolMenu_SetCheck(HWND hwnd, UINT idm);
VOID CoolMenu_SetEnabled(HWND hwnd, UINT idm);

LRESULT CoolMenu_PopupContext(HWND hwnd, LPARAM lParam, int idx);
LRESULT CoolMenu_MeasureItem(WPARAM wParam, LPARAM lParam);
LRESULT CoolMenu_DrawItem(WPARAM wParam, LPARAM lParam);

#endif // __COOLMENU_H__
