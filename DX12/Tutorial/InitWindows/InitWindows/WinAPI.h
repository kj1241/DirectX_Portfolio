#pragma once
#include "DebugLog.h"


class WinAPI
{
public:
    WinAPI(); //생성자
    ~WinAPI(); //소멸자
    bool Init(/*DirectX12Base* pDirectX,*/ HINSTANCE hInstance, int nCmdShow); //초기화
    int Run(/*DirectX12Base* pDirectX*/); //실행
    HWND GetHwnd(); //window 핸들 얻기
    unsigned int GetWidth();
    unsigned int GetHeigth();


    static WinAPI* pWinAPI;
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool Log(LPCWSTR fmt, ...);

protected:


private:
    LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); //윈도우 프로시져

    DebugLog* pDebugLog;
    WNDCLASSEX windowClass = { 0 };
    HWND WinAPI_hwnd; //윈도우 핸들

    unsigned int heigth;
    unsigned int width;


};
