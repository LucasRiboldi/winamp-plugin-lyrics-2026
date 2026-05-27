#include <windows.h>
#include <strsafe.h>
#include <winamp/EmbedWindow.h>

HMENU main_menu = 0, windows_menu = 0;
int height = 0, width = 0;
BOOL visible = FALSE, old_visible = FALSE;
RECT initial[2] = {0};

HWND CreateEmbeddedWindow(embedWindowState* embedWindow, const GUID embedWindowGUID)
{
	SET_EMBED_GUID((embedWindow), embedWindowGUID);

	embedWindow->r.left = GetPrivateProfileInt(INI_FILE_SECTION, L"PosX", 275, ini_file);
	embedWindow->r.top  = GetPrivateProfileInt(INI_FILE_SECTION, L"PosY", 406, ini_file);

	int right  = GetPrivateProfileInt(INI_FILE_SECTION, L"wnd_right",  -1, ini_file);
	int bottom = GetPrivateProfileInt(INI_FILE_SECTION, L"wnd_bottom", -1, ini_file);

	if (right != -1)
	{
		embedWindow->r.right = right;
		WritePrivateProfileString(INI_FILE_SECTION, L"wnd_right", 0, ini_file);
	}
	else
		embedWindow->r.right = embedWindow->r.left + GetPrivateProfileInt(INI_FILE_SECTION, L"SizeX", 275, ini_file);

	if (bottom != -1)
	{
		embedWindow->r.bottom = bottom;
		WritePrivateProfileString(INI_FILE_SECTION, L"wnd_bottom", 0, ini_file);
	}
	else
		embedWindow->r.bottom = embedWindow->r.top + GetPrivateProfileInt(INI_FILE_SECTION, L"SizeY", 232, ini_file);

	CopyRect(&initial[0], &embedWindow->r);

	initial[1].top  = height = GetPrivateProfileInt(INI_FILE_SECTION, L"ff_height", height, ini_file);
	initial[1].left = width  = GetPrivateProfileInt(INI_FILE_SECTION, L"ff_width",  width,  ini_file);

	embedWindow->flags |= EMBED_FLAGS_NOWINDOWMENU;

	HWND frame = (HWND)SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)embedWindow, IPC_GET_EMBEDIF);
	return frame;
}

BOOL WritePrivateProfileInt(LPCWSTR lpAppName, LPCWSTR lpKeyName, int value, LPCWSTR lpFileName)
{
	WCHAR string[32] = {0};
	StringCchPrintf(string, 32, L"%d", value);
	return WritePrivateProfileString(INI_FILE_SECTION, lpKeyName, string, ini_file);
}

void DestroyEmbeddedWindow(embedWindowState* embedWindow)
{
	if (!EqualRect(&initial[0], &embedWindow->r))
	{
		WritePrivateProfileInt(INI_FILE_SECTION, L"PosX",  embedWindow->r.left, ini_file);
		WritePrivateProfileInt(INI_FILE_SECTION, L"PosY",  embedWindow->r.top,  ini_file);
		WritePrivateProfileInt(INI_FILE_SECTION, L"SizeX", embedWindow->r.right  - embedWindow->r.left, ini_file);
		WritePrivateProfileInt(INI_FILE_SECTION, L"SizeY", embedWindow->r.bottom - embedWindow->r.top,  ini_file);
	}
	if (old_visible != visible)
		WritePrivateProfileInt(INI_FILE_SECTION, L"wnd_open", visible, ini_file);
	if (initial[1].top != height || initial[1].left != width)
	{
		WritePrivateProfileInt(INI_FILE_SECTION, L"ff_height", height, ini_file);
		WritePrivateProfileInt(INI_FILE_SECTION, L"ff_width",  width,  ini_file);
	}
}

void AddEmbeddedWindowToMenus(BOOL add, UINT menuId, LPWSTR menuString, BOOL setVisible)
{
	if (add)
	{
		main_menu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);
		int prior = GetMenuItemID(main_menu, 9);
		if (prior <= 0) prior = GetMenuItemID(main_menu, 8);
		MENUITEMINFO mii = { sizeof(MENUITEMINFO), MIIM_ID | MIIM_TYPE | MIIM_DATA,
			MFT_STRING, 0, menuId, 0, 0, 0, 0, menuString, (UINT)lstrlen(menuString) };
		InsertMenuItem(main_menu, prior, FALSE, &mii);
		CheckMenuItem(main_menu, menuId, MF_BYCOMMAND |
			((setVisible == -1 ? visible : setVisible) ? MF_CHECKED : MF_UNCHECKED));
	}
	else DeleteMenu(main_menu, menuId, MF_BYCOMMAND);

	SendMessage(plugin.hwndParent, WM_WA_IPC, (add ? 1 : -1), IPC_ADJUST_OPTIONSMENUPOS);

	if (add)
	{
		windows_menu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 4, IPC_GET_HMENU);
		int prior = GetMenuItemID(windows_menu, 3);
		if (prior <= 0) prior = GetMenuItemID(windows_menu, 2);
		MENUITEMINFO mii = { sizeof(MENUITEMINFO), MIIM_ID | MIIM_TYPE | MIIM_DATA,
			MFT_STRING, 0, menuId, 0, 0, 0, 0, menuString, (UINT)lstrlen(menuString) };
		InsertMenuItem(windows_menu, prior, FALSE, &mii);
		CheckMenuItem(windows_menu, menuId, MF_BYCOMMAND |
			((setVisible == -1 ? visible : setVisible) ? MF_CHECKED : MF_UNCHECKED));
	}
	else DeleteMenu(windows_menu, menuId, MF_BYCOMMAND);

	SendMessage(plugin.hwndParent, WM_WA_IPC, (add ? 1 : -1), IPC_ADJUST_FFWINDOWSMENUPOS);
}

void UpdateEmbeddedWindowsMenu(UINT menuId)
{
	UINT check = MF_BYCOMMAND | (visible ? MF_CHECKED : MF_UNCHECKED);
	if (main_menu)    CheckMenuItem(main_menu,    menuId, check);
	if (windows_menu) CheckMenuItem(windows_menu, menuId, check);
}

BOOL SetEmbeddedWindowMinimizedMode(HWND embeddedWindow, BOOL fMinimized)
{
	if (fMinimized) return SetProp(embeddedWindow, MINIMISED_FLAG, (HANDLE)1);
	RemoveProp(embeddedWindow, MINIMISED_FLAG);
	return TRUE;
}

BOOL EmbeddedWindowIsMinimizedMode(HWND embeddedWindow)
{
	return (GetProp(embeddedWindow, MINIMISED_FLAG) != 0);
}

LRESULT HandleEmbeddedWindowChildMessages(
	HWND embedWnd, UINT menuId, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if ((message == WM_SYSCOMMAND || message == WM_COMMAND) && LOWORD(wParam) == menuId)
	{
		ShowWindow(embedWnd, IsWindowVisible(embedWnd) ? SW_HIDE : SW_SHOW);
		visible = !visible;
		UpdateEmbeddedWindowsMenu(menuId);
		return 1;
	}
	else if (message == WM_CLOSE)
	{
		ShowWindow(embedWnd, SW_HIDE);
		visible = 0;
		UpdateEmbeddedWindowsMenu(menuId);
		SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(WINAMP_NEXT_WINDOW, 0), 0);
	}
	else if (message == WM_WINDOWPOSCHANGING)
	{
		embedWindowState* state = (embedWindowState*)GetWindowLongPtr(embedWnd, GWLP_USERDATA);
		if (state && !GetParent(embedWnd))
			SetWindowPos(embedWnd, 0, 0, 0, width, height,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS);
	}
	return 0;
}

LRESULT HandleEmbeddedWindowWinampWindowMessages(
	HWND embedWnd, UINT menuId, embedWindowState* embedWindow,
	BOOL preSubclass, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (preSubclass)
	{
		if ((message == WM_SYSCOMMAND || message == WM_COMMAND) && LOWORD(wParam) == menuId)
		{
			ShowWindow(embedWnd, IsWindowVisible(embedWnd) ? SW_HIDE : SW_SHOW);
			visible = !visible;
			UpdateEmbeddedWindowsMenu(menuId);
			return 1;
		}
		else if (message == WM_COMMAND && LOWORD(wParam) == WINAMP_REFRESHSKIN)
		{
			if (!GetParent(embedWnd))
			{
				width  = embedWindow->r.right  - embedWindow->r.left;
				height = embedWindow->r.bottom - embedWindow->r.top;
			}
		}
	}
	else
	{
		if (message == WM_SIZE && wParam == SIZE_RESTORED && EmbeddedWindowIsMinimizedMode(embedWnd))
		{
			ShowWindow(embedWnd, visible ? SW_SHOWNA : SW_HIDE);
			SetEmbeddedWindowMinimizedMode(embedWnd, FALSE);
		}
	}
	return NULL;
}
