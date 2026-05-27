#ifndef MAJEST_EMBED_WINDOW_H
#define MAJEST_EMBED_WINDOW_H

#include <Winamp\gen.h>
#include <Winamp\wa_ipc.h>
#include <winamp/WinampApi.h>

#ifndef WINAMP_NEXT_WINDOW
#define WINAMP_NEXT_WINDOW 40063
#endif

#ifndef WINAMP_REFRESHSKIN
#define WINAMP_REFRESHSKIN 40291
#endif

#define INI_FILE_SECTION L"MajestLyrics"
#define MINIMISED_FLAG   L"MLMinMode"

HWND CreateEmbeddedWindow(embedWindowState* embedWindow, const GUID embedWindowGUID);
void AddEmbeddedWindowToMenus(BOOL add, UINT menuId, LPWSTR menuString, BOOL visible);
void UpdateEmbeddedWindowsMenu(UINT menuId);
void DestroyEmbeddedWindow(embedWindowState* embedWindow);
BOOL SetEmbeddedWindowMinimizedMode(HWND embeddedWindow, BOOL fMinimized);
BOOL EmbeddedWindowIsMinimizedMode(HWND embeddedWindow);

BOOL WritePrivateProfileInt(LPCWSTR lpAppName, LPCWSTR lpKeyName, int value, LPCWSTR lpFileName);

LRESULT HandleEmbeddedWindowChildMessages(
	HWND embedWnd, UINT menuId, HWND hwnd,
	UINT message, WPARAM wParam, LPARAM lParam);

LRESULT HandleEmbeddedWindowWinampWindowMessages(
	HWND embedWnd, UINT menuId, embedWindowState* embedWindow,
	BOOL preSubclass, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

extern LPWSTR ini_file;
extern winampGeneralPurposePlugin plugin;
extern BOOL visible, old_visible;

#endif
