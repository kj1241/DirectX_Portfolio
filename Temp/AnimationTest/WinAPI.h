#pragma once
#include "DirectX9.h"
#include "DebugLog.h"


class DirectXPipline;

class WinAPI
{
public:
    WinAPI(); //생성자
    ~WinAPI(); //소멸자
    bool Init(/*DirectX12Base* pDirectX,*/ HINSTANCE hInstance, int nCmdShow); //초기화
    int Run(/*DirectX12Base* pDirectX*/); //실행
    HWND GetHwnd(); //window 핸들 얻기
 
    static WinAPI* pWinAPI;

    LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); //윈도우 프로시져

    bool Log(LPCWSTR fmt, ...);

protected:


private:
    DebugLog* pDebugLog;
    DirectXPipline* pDirectX;
    WNDCLASSEX windowClass = { 0 };
    HWND WinAPI_hwnd; //윈도우 핸들

};
