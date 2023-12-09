#include "stdafx.h" 
#include "WinAPI.h" //다이렉트 x 파이프라인
//#include "DirectX12EnginePipline.h"

_Use_decl_annotations_ //error c28213 해결 사용 이유: 정적분석 도구 에서 주석을 가져오도록 사용
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WinAPI projectK;
    //DirectX12EnginePipline Engine(1600, 900, L"DirectX12 Mini Engine");
    if (!projectK.Init(/*&Engine,*/ hInstance, nCmdShow))  //초기화 실패하면 
        return 0;   // 0으로 리턴
    return projectK.Run(/*&Engine*/); //성공시 run 코드 실행
}
