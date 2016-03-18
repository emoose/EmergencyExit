#include "stdafx.hpp"
#include "resource.h"
#include <string>

#define WM_USER_TRAYICON ( WM_USER + 1 )

#define ID_TRAY_APP_ICON 5000
#define ID_TRAY_NAME 5001
#define ID_TRAY_STARTUP 5002
#define ID_TRAY_SEP 5003
#define ID_TRAY_EXIT 5004

NOTIFYICONDATA notifyIconData;
HINSTANCE instance;
HWND hwnd;
TCHAR title[256];

#pragma region "Startup Helpers"
long RegistryGetString(HKEY hKey, const std::wstring& valueName, std::wstring& value, const std::wstring& defaultValue)
{
	value = defaultValue;
	WCHAR buffer[512];
	DWORD bufferSize = sizeof(buffer);
	long res = RegQueryValueEx(hKey, valueName.c_str(), 0, NULL, (LPBYTE)buffer, &bufferSize);
	if (res == ERROR_SUCCESS)
		value = buffer;

	return res;
}

long RegistrySetString(HKEY hKey, const std::wstring& valueName, const std::wstring& value)
{
	return RegSetValueEx(hKey, valueName.c_str(), 0, REG_SZ, (LPBYTE)value.c_str(), value.length() * sizeof(WCHAR));
}

// returns true if startup entry is set to this exe
bool StartupIsSet()
{
	HKEY runKey = NULL;
	std::wstring runKeyVal;
	if (RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &runKey) != ERROR_SUCCESS)
	{
		OutputDebugString(L"StartupIsSet: Failed to open Run key!\n");
		runKey = NULL;
		return false;
	}
	else
	{
		if (RegistryGetString(runKey, title, runKeyVal, L"") != ERROR_SUCCESS)
		{
			OutputDebugString(L"StartupIsSet: Failed to query startup entry!\n");
			return false;
		}
	}

	WCHAR buffer[512];
	DWORD bufferSize = sizeof(buffer);
	GetModuleFileName(GetModuleHandle(NULL), buffer, bufferSize);

	return runKeyVal == std::wstring(buffer);
}

bool StartupCreateEntry()
{
	HKEY runKey = NULL;
	std::wstring runKeyVal;
	if (RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &runKey) != ERROR_SUCCESS)
	{
		OutputDebugString(L"StartupCreateEntry: Failed to open Run key!\n");
		runKey = NULL;
		return false;
	}

	WCHAR buffer[512];
	DWORD bufferSize = sizeof(buffer);
	GetModuleFileName(GetModuleHandle(NULL), buffer, bufferSize);

	OutputDebugString(L"StartupCreateEntry: Setting startup entry to current exe...\n");
	OutputDebugString(buffer);

	if (RegistrySetString(runKey, title, buffer) != ERROR_SUCCESS)
	{
		OutputDebugString(L"StartupCreateEntry: Failed to create startup entry!\n");
		return false;
	}

	return true;
}

bool StartupDeleteEntry()
{
	HKEY runKey = NULL;
	std::wstring runKeyVal;
	if (RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &runKey) != ERROR_SUCCESS)
	{
		OutputDebugString(L"StartupDeleteEntry: Failed to open Run key!\n");
		runKey = NULL;
		return false;
	}

	return RegDeleteValue(runKey, title) == ERROR_SUCCESS;
}
#pragma endregion

#pragma region "SysTray / WndProc stuff"
void SysTrayInit()
{
	memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));

	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = hwnd;
	notifyIconData.uID = ID_TRAY_APP_ICON;

	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

	notifyIconData.uCallbackMessage = WM_USER_TRAYICON;
	notifyIconData.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_EMERGENCYEXIT));

	wcscpy_s(notifyIconData.szTip, L"EmergencyExit (CTRL+PauseBreak to kill active program)");
	Shell_NotifyIcon(NIM_ADD, &notifyIconData);
}

void SysTrayShowContextMenu()
{
	POINT lpClickPoint;
	GetCursorPos(&lpClickPoint);

	HMENU hPopMenu = CreatePopupMenu();
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING | MF_GRAYED, ID_TRAY_NAME, title);
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING | (StartupIsSet() ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_STARTUP, L"Run on startup");
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_SEPARATOR, ID_TRAY_SEP, L"SEP");
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"Exit");

	SetForegroundWindow(hwnd);
	TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
		lpClickPoint.x, lpClickPoint.y, 0, hwnd, NULL);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, lmId;

	switch (message)
	{
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		switch (wmId)
		{
		case ID_TRAY_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_TRAY_STARTUP:
			if (StartupIsSet())
				StartupDeleteEntry();
			else
				StartupCreateEntry();
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_USER_TRAYICON:
		lmId = LOWORD(lParam);
		switch (lmId)
		{
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN:
			SysTrayShowContextMenu();
			break;
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
#pragma endregion

#pragma region "Process killing / keyboard hook"
void KillForegroundProcess()
{
	OutputDebugString(L"Killing foreground process...\n");
	HWND activeHwnd = GetForegroundWindow();
	if (!activeHwnd)
		return;

	std::wstring outputText;

#ifdef _DEBUG
	// only show this in debug builds, because I think this might require the offending program to still be able to respond to window messages?
	// not sure, in any case 99% of people probably won't see this debug output anyway
	TCHAR title[256];
	GetWindowText(activeHwnd, title, 256);
	outputText = L"Window title: ";
	outputText = outputText + std::wstring(title) + std::wstring(L"\n");
#endif

	DWORD procId;
	GetWindowThreadProcessId(activeHwnd, &procId);

	outputText = outputText + std::wstring(L"Process ID: ") + std::to_wstring(procId) + std::wstring(L"\n");
	OutputDebugString(outputText.c_str());

	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, false, procId);
	TerminateProcess(process, 1);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	bool eatKeystroke = false;
	static bool isHeld = false;
	PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;

	if (nCode == HC_ACTION)
	{
		switch (wParam)
		{
		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (eatKeystroke = (p->vkCode == VK_PAUSE || p->vkCode == VK_CANCEL))
				isHeld = false;

			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (isHeld)
				break; // if it's already been held wait till they release before doing it again

			auto controlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;

			if (eatKeystroke = ((p->vkCode == VK_PAUSE || p->vkCode == VK_CANCEL) && controlPressed))
			{
				KillForegroundProcess();
				isHeld = true;
			}

			break;
		}
	}

	return eatKeystroke ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam);
}
#pragma endregion

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	instance = hInstance;
	wcscpy_s(title, L"EmergencyExit");

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = instance;
	wcex.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_EMERGENCYEXIT));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = title;
	wcex.lpszClassName = title;
	wcex.hIconSm = LoadIcon(instance, MAKEINTRESOURCE(IDI_EMERGENCYEXIT));

	RegisterClassEx(&wcex);

	hwnd = CreateWindow(title, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, instance, NULL);

	if (!hwnd)
		return FALSE;

	if (!StartupIsSet()) // startup entry isn't set to this exe
		if (MessageBox(hwnd, L"Do you want to run EmergencyExit on startup?", title, MB_YESNO) == IDYES)
			StartupCreateEntry();

	SysTrayInit();
	HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &notifyIconData); // kill systray icon
	UnhookWindowsHookEx(keyboardHook);

	return (int)msg.wParam;
}