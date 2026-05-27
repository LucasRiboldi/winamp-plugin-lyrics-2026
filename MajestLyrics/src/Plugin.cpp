#include <core/LyricHandler.h>
#include <decoders/LyricDecoder.h>
#include <decoders/MusixmatchDecoder.h>
#include <winamp/EmbedWindow.h>
#include "res/resource.h"

#include <Windows.h>
#include <strsafe.h>
#include <string>
#include <stdint.h>
#include <thread>
#include <mutex>
#include <fstream>
#include <exception>
#include <atomic>

#include <Winamp\wa_ipc.h>
#include <Winamp\gen.h>
#include <Winamp\ipc_pe.h>
#define WA_DLG_IMPLEMENT
#include <Winamp\wa_dlg.h>

#define ENABLE_SCROLLING
#define PLUGIN_NAME          "Majest Lyrics"
#define PLUGIN_VERSION       "v1.0"
#define FILE_INFO_BUFFER_SIZE 1024
#define MAX_THREAD_COUNT      1
#define LYRICS_LABEL_TOP      0
#define SETTINGS_FILE_PATH    ".\\Plugins\\MajestLyrics\\options.txt"

const static GUID wndStateGUID = { 0x3fcd6a40, 0x95d2, 0x4b0a, { 0x8a, 0x96, 0x24, 0x7e, 0xc5, 0xc3, 0x32, 0x9b } };

const std::pair<unsigned, unsigned> BUTTON_OFFSET{ 0, 0 };

LRESULT CALLBACK ChildWndProc    (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WaWndProc       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    CALLBACK OptionWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void GetSongLyrics(HWND hwnd);
void ReadSettingsFile(HWND hwnd);
void SaveSettings(HWND hwnd);

embedWindowState    myWndState = { 0 };
WNDPROC             lpWndProcOld = 0;
HWND                embedWnd = NULL, childWnd = NULL;
WCHAR*              ini_file;
WCHAR               wa_path[MAX_PATH] = { 0 };
UINT                LYRICS_MENUID, EMBEDWND_ID;
std::atomic<int>    activeThreads{};
std::wstring        activeSong, activeSongLyrics;
std::mutex          lyric_mutex;
LyricHandler        handler;
COLORREF            rgbBgColor;
bool                isEnabled = true, isThreadingEnabled = true, isColorChanged = false, isScrollingEnabled = false;
int                 iCurrentLineScrolled = 0;
char                fullFilename[MAX_PATH];
// Lyrics font size (points). Clamped to [6,24]. Persisted to options.txt.
int                 g_fontSize = 9;
HFONT               g_lyricFont = NULL;

// Recreate the lyric label's font from g_fontSize and apply it.
static void ApplyLyricFont()
{
	if (!childWnd) return;
	HDC hdc = GetDC(childWnd);
	int height = -MulDiv(g_fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(childWnd, hdc);
	HFONT hNew = CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft Sans Serif");
	if (!hNew) return;
	SendDlgItemMessage(childWnd, IDC_LYRIC_STRING, WM_SETFONT, (WPARAM)hNew, TRUE);
	if (g_lyricFont) DeleteObject(g_lyricFont);
	g_lyricFont = hNew;
}

void config();
void quit();
int  init();

winampGeneralPurposePlugin plugin =
{
	GPPHDR_VER,
	0,
	init,
	config,
	quit,
};

extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin()
{
	return &plugin;
}

LRESULT CALLBACK WaWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HandleEmbeddedWindowWinampWindowMessages(embedWnd, EMBEDWND_ID, &myWndState, TRUE, hwnd, message, wParam, lParam);

	switch (message)
	{
		case WM_USER:
		{
			if (lParam == IPC_PLAYING_FILE)
			{
				if (isEnabled)
				{
					if (isThreadingEnabled && activeThreads < MAX_THREAD_COUNT)
					{
						++activeThreads;
						std::thread(GetSongLyrics, hwnd).detach();
					}
					else
						GetSongLyrics(hwnd);
				}
			}
			else if (lParam == IPC_FF_ONCOLORTHEMECHANGED)
			{
				InvalidateRect(childWnd, NULL, TRUE);
				WADlg_init(hwnd);
				SetFocus(childWnd);
			}
			break;
		}
	}

	LRESULT res = CallWindowProc(lpWndProcOld, hwnd, message, wParam, lParam);
	HandleEmbeddedWindowWinampWindowMessages(embedWnd, EMBEDWND_ID, &myWndState, FALSE, hwnd, message, wParam, lParam);
	return res;
}

void config()
{
	DialogBox(plugin.hDllInstance, MAKEINTRESOURCE(IDD_FORMVIEW), plugin.hwndParent, (DLGPROC)OptionWindowProc);
}

void quit()
{
	DestroyEmbeddedWindow(&myWndState);
	if (g_lyricFont) { DeleteObject(g_lyricFont); g_lyricFont = NULL; }
}

int init()
{
	WADlg_init(plugin.hwndParent);
	if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVERSION) < 0x5053)
	{
		MessageBoxA(plugin.hwndParent,
			"Majest Lyrics requires Winamp v5.53 or later.",
			PLUGIN_NAME, MB_OK | MB_ICONINFORMATION);
		return 1;
	}

	static char szDescription[256];
	StringCchPrintfA(szDescription, 256, "Majest Lyrics %s", PLUGIN_VERSION);
	plugin.description = szDescription;

	ini_file = (LPWSTR)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETINIFILE);

	if (IsWindowUnicode(plugin.hwndParent))
		lpWndProcOld = (WNDPROC)SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)WaWndProc);
	else
		lpWndProcOld = (WNDPROC)SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)WaWndProc);

	embedWnd = CreateEmbeddedWindow(&myWndState, wndStateGUID);
	SetWindowText(embedWnd, L"Majest Lyrics");

	childWnd = CreateDialog(plugin.hDllInstance, MAKEINTRESOURCE(IDD_DIALOGVIEW), embedWnd, (DLGPROC)ChildWndProc);

	EMBEDWND_ID = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_REGISTER_LOWORD_COMMAND);

	ReadSettingsFile(plugin.hwndParent);
	ApplyLyricFont();
	AddEmbeddedWindowToMenus(TRUE, EMBEDWND_ID, (LPWSTR)L"Majest Lyrics", -1);

	return GEN_INIT_SUCCESS;
}

LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COMMAND:
		{
			WORD ctl = LOWORD(wParam);
			if (ctl == IDC_FONT_MINUS || ctl == IDC_FONT_PLUS)
			{
				g_fontSize += (ctl == IDC_FONT_PLUS) ? 1 : -1;
				if (g_fontSize < 6)  g_fontSize = 6;
				if (g_fontSize > 24) g_fontSize = 24;
				ApplyLyricFont();
				SaveSettings(hwnd);
				return 0;
			}
			break;
		}
		case IDC_REFRESH_BUTTON:
		{
			const wchar_t* filename = (const wchar_t*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_PLAYING_FILENAME);
			wchar_t artist[FILE_INFO_BUFFER_SIZE]{ 0 };
			wchar_t title [FILE_INFO_BUFFER_SIZE]{ 0 };
			extendedFileInfoStructW fi{ filename, L"ARTIST", artist, FILE_INFO_BUFFER_SIZE };
			SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&fi, IPC_GET_EXTENDED_FILE_INFOW_HOOKABLE);
			fi.metadata = L"TITLE"; fi.ret = title;
			SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&fi, IPC_GET_EXTENDED_FILE_INFOW_HOOKABLE);
			handler.ClearSong(SongKey(std::wstring(artist), std::wstring(title)));
			activeSong = L"";
			GetSongLyrics(plugin.hwndParent);
			break;
		}
		case WM_CTLCOLORDLG:
		{
			COLORREF newColor = WADlg_getColor(WADLG_ITEMBG);
			if (isColorChanged || rgbBgColor != newColor)
			{
				rgbBgColor = newColor;
				SetDlgItemText(hwnd, IDC_LYRIC_STRING, activeSongLyrics.c_str());
				isColorChanged = !isColorChanged;
			}
			return (INT_PTR)CreateSolidBrush(rgbBgColor);
		}
		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC)wParam;
			SetTextColor(hdc, WADlg_getColor(WADLG_SELBAR_FGCOLOR));
			SetBkMode(hdc, TRANSPARENT);
			return (INT_PTR)CreateSolidBrush(rgbBgColor);
		}
		case WM_MOUSEWHEEL:
		{
#ifdef ENABLE_SCROLLING
			if (isScrollingEnabled)
			{
				short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
				HWND hwndLabel = GetDlgItem(hwnd, IDC_LYRIC_STRING);
				RECT rect{}, windowRect{};
				GetWindowRect(hwndLabel, &rect);
				MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2);
				GetWindowRect(hwnd, &windowRect);

				auto interval = handler.GetInterval(activeSong, iCurrentLineScrolled, rect.bottom / 12);
				if (zDelta > 0 && iCurrentLineScrolled > 0)
				{
					activeSongLyrics = interval.first;
					--iCurrentLineScrolled;
					SetDlgItemText(childWnd, IDC_LYRIC_STRING, activeSongLyrics.c_str());
				}
				else if (zDelta < 0 && !interval.second)
				{
					activeSongLyrics = interval.first;
					++iCurrentLineScrolled;
					SetDlgItemText(childWnd, IDC_LYRIC_STRING, activeSongLyrics.c_str());
				}
			}
#endif
			break;
		}
		case WM_SIZE:
		{
			SetWindowPos(GetDlgItem(hwnd, IDC_REFRESH_BUTTON), 0,
				LOWORD(lParam) - BUTTON_OFFSET.first,
				HIWORD(lParam) - BUTTON_OFFSET.second,
				0, 0, SWP_NOSIZE);
			SetWindowPos(GetDlgItem(hwnd, IDC_LYRIC_STRING), 0,
				0, 0,
				LOWORD(lParam),
				HIWORD(lParam) - BUTTON_OFFSET.second - 5,
				SWP_NOMOVE);
			break;
		}
		default: break;
	}
	return HandleEmbeddedWindowChildMessages(embedWnd, EMBEDWND_ID, hwnd, message, wParam, lParam);
}

BOOL CALLBACK OptionWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hwnd, IDC_DISABLE_CHECK,   BM_SETCHECK, isEnabled          ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwnd, IDC_THREADING_CHECK, BM_SETCHECK, isThreadingEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(hwnd, IDC_SCROLLING_CHECK, BM_SETCHECK, isScrollingEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
			// fall through
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_DISABLE_CHECK:   isEnabled          = (bool)SendDlgItemMessage(hwnd, IDC_DISABLE_CHECK,   BM_GETCHECK, 0, 0); break;
				case IDC_THREADING_CHECK: isThreadingEnabled = (bool)SendDlgItemMessage(hwnd, IDC_THREADING_CHECK, BM_GETCHECK, 0, 0); break;
				case IDC_SCROLLING_CHECK: isScrollingEnabled = (bool)SendDlgItemMessage(hwnd, IDC_SCROLLING_CHECK, BM_GETCHECK, 0, 0); break;
				case IDC_EXIT_BUTTON: EndDialog(hwnd, wParam); break;
				case IDC_SAVE_BUTTON: SaveSettings(hwnd); EndDialog(hwnd, wParam); return TRUE;
				case IDOK:     EndDialog(hwnd, IDOK);     break;
				case IDCANCEL: EndDialog(hwnd, IDCANCEL); break;
			}
			break;
		}
		default: return FALSE;
	}
	return FALSE;
}

void GetSongLyrics(HWND hwnd)
{
	struct ScopeGuard {
		std::mutex& mtx;
		bool locked = false;
		explicit ScopeGuard(std::mutex& m) : mtx(m) { mtx.lock(); locked = true; }
		~ScopeGuard() { if (locked) mtx.unlock(); --activeThreads; }
	} guard(lyric_mutex);

	const wchar_t* filename = (const wchar_t*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_PLAYING_FILENAME);
	wchar_t artist[FILE_INFO_BUFFER_SIZE]{ 0 };
	wchar_t title [FILE_INFO_BUFFER_SIZE]{ 0 };

	extendedFileInfoStructW fi{ filename, L"ARTIST", artist, FILE_INFO_BUFFER_SIZE };
	SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&fi, IPC_GET_EXTENDED_FILE_INFOW_HOOKABLE);
	fi.metadata = L"TITLE"; fi.ret = title;
	SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&fi, IPC_GET_EXTENDED_FILE_INFOW_HOOKABLE);

	std::wstring songKey = SongKey(std::wstring(artist), std::wstring(title));
	if (activeSong != songKey)
	{
		try
		{
			if (!handler.HasSong(songKey))
			{
				handler.GetLyrics(
					MajestLyrics::WideToUTF8(std::wstring(artist)),
					MajestLyrics::WideToUTF8(std::wstring(title)),
					MajestLyrics::TryDecode);
			}

			activeSong           = songKey;
			activeSongLyrics     = handler.GetSong(songKey);
			iCurrentLineScrolled = 0;

			SetDlgItemText(childWnd, IDC_LYRIC_STRING,
				activeSongLyrics.empty() ? L"Lyrics not found." : activeSongLyrics.c_str());
		}
		catch (std::exception& e)
		{
			MessageBoxA(hwnd, e.what(), "Majest Lyrics - Error", MB_OK | MB_ICONERROR);
		}
	}
}

void ReadSettingsFile(HWND hwnd)
{
	char dllPath[MAX_PATH] = { 0 };
	GetModuleFileNameA(plugin.hDllInstance, dllPath, MAX_PATH);
	char* lastSlash = strrchr(dllPath, '\\');
	if (lastSlash) *lastSlash = '\0';
	StringCchPrintfA(fullFilename, MAX_PATH, "%s\\MajestLyrics\\options.txt", dllPath);
	// Tell MusixmatchDecoder where to persist its anonymous usertoken between sessions.
	MusixmatchDecoder::s_plugin_dir = std::string(dllPath) + "\\MajestLyrics";
	std::ifstream inStream{ fullFilename };

	if (!inStream.is_open())
	{
		std::string folderPath{ fullFilename };
		CreateDirectoryA(folderPath.substr(0, folderPath.find_last_of('\\')).c_str(), NULL);
		std::ofstream outStream{ fullFilename };
		if (outStream.is_open())
			outStream << "enable=1\nthreading=1\nscrolling=0\nfontsize=9";
		else
			MessageBoxA(hwnd, "Failed to create settings file.", "Majest Lyrics", MB_OK | MB_ICONERROR);
		return;
	}

	try
	{
		std::string line;
		while (std::getline(inStream, line))
		{
			auto token = Split(line, "=");
			if (token.size() < 2) continue;
			if      (token[0] == "enable")         isEnabled                   = atoi(token[1].c_str()) != 0;
			else if (token[0] == "threading")      isThreadingEnabled          = atoi(token[1].c_str()) != 0;
			else if (token[0] == "fontsize")
			{
				g_fontSize = atoi(token[1].c_str());
				if (g_fontSize < 6)  g_fontSize = 6;
				if (g_fontSize > 24) g_fontSize = 24;
			}
			else if (token[0] == "scrolling")
			{
#ifdef ENABLE_SCROLLING
				isScrollingEnabled = atoi(token[1].c_str()) != 0;
#else
				isScrollingEnabled = false;
#endif
			}
		}
	}
	catch (std::exception& e)
	{
		MessageBoxA(hwnd, e.what(), "Majest Lyrics", MB_OK | MB_ICONERROR);
	}
}

void SaveSettings(HWND hwnd)
{
	std::ofstream outStream{ fullFilename };
	if (outStream.is_open())
		outStream << "enable="         << isEnabled
		          << "\nthreading="    << isThreadingEnabled
		          << "\nscrolling="    << isScrollingEnabled
		          << "\nfontsize="     << g_fontSize;
	else
		MessageBoxA(hwnd, "Failed to save settings.", "Majest Lyrics", MB_OK | MB_ICONERROR);
}
