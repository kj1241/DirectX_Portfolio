#pragma once

//c++11 이후에 비트 리터럴 있는거 알지만 안쓸꺼임
enum PlatformLog {
	CONSOLE = 0x01, //0001
	TEXTFILE = 0x02, //0010
	WINDOW = 0x04   // 0100
};

class DebugLog
{
public:

	DebugLog(UINT32 nTarget, LPCWSTR szFilename = nullptr);
	~DebugLog();
	void CreateDebugLogWindow();
	bool Log(LPCWSTR fmt, ...);


private:
	static LRESULT CALLBACK WndProcDebugLog(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	unsigned int nTarget;
	wchar_t sizeFilename[MAX_PATH];
	HWND hwnd;
	HWND hwndList;

	int width = 300;
	int height = 600;
};