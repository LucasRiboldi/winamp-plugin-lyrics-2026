#include <core/LyricHandler.h>
#include <decoders/LyricDecoder.h>
#include <winamp/WinampApi.h>
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

#include <api\service\waservicefactory.h>
#include <api\application\api_application.h>
#include <bfc\platform\types.h>
#include <rpcdce.h>
#include <Agave\Language\api_language.h>
#include <Agave\Language\lang.h>
#include <Winamp\wa_ipc.h>
#include <Winamp\gen.h>
#include <Winamp\ipc_pe.h>
#define WA_DLG_IMPLEMENT
#include <Winamp\wa_dlg.h>

#define ENABLE_SCROLLING
#define PLUGIN_NAME          "Majest Lyrics"
#define PLUGIN_VERSION       "v1.0"
#define FILE_INFO_BUFFER_SIZE 128
#define MAX_THREAD_COUNT      1
#define LYRICS_LABEL_TOP      0
#define SETTINGS_FILE_PATH    ".\\Plugins\\MajestLyrics\\options.txt"

const static GUID wndStateGUID = { 0x3fcd6a40, 0x95d2, 0x4b0a, { 0x8a, 0x96, 0x24, 0x7e, 0xc5, 0xc3, 0x32, 0x9b } };
const static GUID wndLangGUID  = { 0x486676e6, 0x9306, 0x4fcf, { 0x9d, 0x9b, 0x76, 0x5a, 0x65, 0xf9, 0xfe, 0xb8 } };

const std::pair<unsigned, unsigned> BUTTON_OFFSET{ 0, 0 };

api_service     *WASABI_API_SVC = 0;
api_language    *WASABI_API_LNG = 0;
api_application *WASABI_API_APP = 0;

LRESULT CALLBACK ChildWndProc    (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WaWndProc       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    CALLBACK OptionWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void GetSongLyrics(HWND hwnd);
void ReadSettingsFile(HWND hwnd);
void SaveSettings(HWND hwnd);

HINSTANCE           WASABI_API_LNG_HINST = 0, WASABI_API_ORIG_HINST = 0;
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

template <class api_T>
void ServiceBuild(api_T*& api_t, GUID factoryGUID_t)
{
	if (WASABI_API_SVC)
	{
		waServiceFactory* factory = WASABI_API_SVC->service_getServiceByGuid(factoryGUID_t);
		if (factory) api_t = (api_T*)factory->getInterface();
	}
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
						std::thread(GetSongLyrics, hwnd).detach();
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

	WASABI_API_SVC = (api_service*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_API_SERVICE);
	if (WASABI_API_SVC == (api_service*)1) WASABI_API_SVC = nullptr;

	if (!WASABI_API_SVC) return 1;

	ServiceBuild(WASABI_API_LNG, languageApiGUID);
	WASABI_API_START_LANG(plugin.hDllInstance, wndLangGUID);

	static char szDescription[256];
	StringCchPrintfA(szDescription, 256, WASABI_API_LNGSTRING(IDS_LANGUAGE_EXAMPLE), PLUGIN_VERSION);
	plugin.description = szDescription;

	ini_file = (LPWSTR)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETINIFILE);

	if (IsWindowUnicode(plugin.hwndParent))
		lpWndProcOld = (WNDPROC)SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)WaWndProc);
	else
		lpWndProcOld = (WNDPROC)SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)WaWndProc);

	ServiceBuild(WASABI_API_APP, applicationApiServiceGuid);

	embedWnd = CreateEmbeddedWindow(&myWndState, wndStateGUID);
	SetWindowText(embedWnd, WASABI_API_LNGSTRINGW(IDS_PLUGIN_NAME));

	childWnd = WASABI_API_CREATEDIALOG(IDD_DIALOGVIEW, embedWnd, ChildWndProc);
	if (childWnd && WASABI_API_APP) WASABI_API_APP->app_addModelessDialog(childWnd);

	EMBEDWND_ID = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_REGISTER_LOWORD_COMMAND);

	ACCEL accel = { FVIRTKEY | FALT, '1', EMBEDWND_ID };
	HACCEL hAccel = CreateAcceleratorTable(&accel, 1);
	if (hAccel) WASABI_API_APP->app_addAccelerators(childWnd, &hAccel, 1, TRANSLATE_MODE_NORMAL);

	ReadSettingsFile(plugin.hwndParent);
	AddEmbeddedWindowToMenus(TRUE, EMBEDWND_ID, WASABI_API_LNGSTRINGW(IDS_PLUGIN_NAME), -1);

	return GEN_INIT_SUCCESS;
}

LRESULT CALLBACK ChildWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case IDC_REFRESH_BUTTON:
		{
			const wchar_t* filename = (const wchar_t*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_PLAYING_FILENAME);
			wchar_t title[FILE_INFO_BUFFER_SIZE]{ 0 };
			extendedFileInfoStructW fi{ filename, L"TITLE", title, FILE_INFO_BUFFER_SIZE };
			SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&fi, IPC_GET_EXTENDED_FILE_INFOW_HOOKABLE);
			handler.ClearSong(ToLower(std::wstring(title)));
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
	while (!lyric_mutex.try_lock()) { Sleep(10); }

	const wchar_t* filename = (const wchar_t*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_PLAYING_FILENAME);
	wchar_t artist[FILE_INFO_BUFFER_SIZE]{ 0 };
	wchar_t title [FILE_INFO_BUFFER_SIZE]{ 0 };

	extendedFileInfoStructW fi{ filename, L"ARTIST", artist, FILE_INFO_BUFFER_SIZE };
	SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&fi, IPC_GET_EXTENDED_FILE_INFOW_HOOKABLE);
	fi.metadata = L"TITLE"; fi.ret = title;
	SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&fi, IPC_GET_EXTENDED_FILE_INFOW_HOOKABLE);

	if (activeSong != title)
	{
		try
		{
			std::wstring wTitle = ToLower(std::wstring(title));

			if (!handler.HasSong(wTitle))
			{
				std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
				handler.GetLyrics(
					conv.to_bytes(artist),
					conv.to_bytes(std::wstring(title)),
					MajestLyrics::TryDecode);
			}

			activeSong           = wTitle;
			activeSongLyrics     = handler.GetSong(wTitle);
			iCurrentLineScrolled = 0;

			SetDlgItemText(childWnd, IDC_LYRIC_STRING,
				activeSongLyrics.empty() ? L"Lyrics not found." : activeSongLyrics.c_str());
		}
		catch (std::exception& e)
		{
			MessageBoxA(hwnd, e.what(), "Majest Lyrics - Error", MB_OK | MB_ICONERROR);
		}
	}

	lyric_mutex.unlock();
	--activeThreads;
}

void ReadSettingsFile(HWND hwnd)
{
	GetFullPathNameA(SETTINGS_FILE_PATH, MAX_PATH, fullFilename, NULL);
	std::ifstream inStream{ fullFilename };

	if (!inStream.is_open())
	{
		std::string folderPath{ fullFilename };
		CreateDirectoryA(folderPath.substr(0, folderPath.find_last_of('\\')).c_str(), NULL);
		std::ofstream outStream{ fullFilename };
		if (outStream.is_open())
			outStream << "enable=1\nthreading=1\nscrolling=0";
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
			if      (token[0] == "enable")    isEnabled          = atoi(token[1].c_str()) != 0;
			else if (token[0] == "threading") isThreadingEnabled = atoi(token[1].c_str()) != 0;
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
		outStream << "enable=" << isEnabled
		          << "\nthreading=" << isThreadingEnabled
		          << "\nscrolling=" << isScrollingEnabled;
	else
		MessageBoxA(hwnd, "Failed to save settings.", "Majest Lyrics", MB_OK | MB_ICONERROR);
}
