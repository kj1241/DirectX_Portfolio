#include "stdafx.h"
#include "DebugLog.h"


LRESULT CALLBACK DebugLog::WndProcDebugLog(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


DebugLog::DebugLog(UINT32 ntarget, LPCWSTR sizefilename)
{
	this->nTarget = ntarget;
	//
	if (nTarget & PlatformLog::TEXTFILE)
		lstrcpy(this->sizeFilename, sizefilename);
	else
		sizeFilename[0] = NULL;
	//
	if (nTarget & PlatformLog::WINDOW)
		CreateDebugLogWindow();
	else
		hwnd = nullptr;

	Log(L"Debug Log Window ");
}

DebugLog::~DebugLog()
{
	DestroyWindow(hwnd);
}

void DebugLog::CreateDebugLogWindow()
{
	int x, y;
	WNDCLASS wc;
	RECT rc;

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)DebugLog::WndProcDebugLog;
	wc.cbClsExtra = 0;									// No Extra Window Data
	wc.cbWndExtra = 0;									// No Extra Window Data
	wc.hInstance = GetModuleHandle(NULL);
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"DebugLogWindow";

	RegisterClass(&wc);


	x = GetSystemMetrics(SM_CXSCREEN) - width;
	y = 0;

	hwnd = CreateWindow(L"DebugLogWindow", L"DebugLog", WS_POPUP | WS_CAPTION, x, y, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);
	GetClientRect(hwnd, &rc);
	hwndList = CreateWindowW(L"LISTBOX", L"", WS_CHILD | WS_VSCROLL, 0, 0, rc.right, rc.bottom, hwnd, NULL, GetModuleHandle(NULL), NULL);
	ShowWindow(hwnd, SW_SHOW);
	ShowWindow(hwndList, SW_SHOW);
}

bool DebugLog::Log(LPCWSTR fmt, ...)
{
	wchar_t buff[1024];
	wchar_t	date[128];
	wchar_t	time[128];

	_wstrdate_s(date);
	_wstrtime_s(time);
	vswprintf_s(buff, fmt, (char*)(&fmt + 1));

	// Console에 출력할 경우 
	if (nTarget & PlatformLog::CONSOLE)
	{
		std::cout << "(날짜[" << date << "] 시간[" << time << "]) : " << buff << "\n";
	}

	// Log File에 출력할 경우
	if (nTarget & PlatformLog::TEXTFILE)
	{
		std::ofstream file(sizeFilename, std::ios::app);
		if (file.is_open()) {
			file << "(날짜[" << date << "] 시간[" << time << "]) : " << buff << "\n";
			file.close();
		}
		else {
			std::cerr << "파일을 열 수 없습니다." << std::endl;
		}


	}

	// Log Window에 출력할 경우 
	if (nTarget & PlatformLog::WINDOW)
	{
		SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)buff);
		UINT32 n = SendMessage(hwndList, LB_GETCOUNT, 0, 0L) - 1;
		SendMessage(hwndList, LB_SETCURSEL, (WPARAM)n, 0L);
	}
	return true;
}

