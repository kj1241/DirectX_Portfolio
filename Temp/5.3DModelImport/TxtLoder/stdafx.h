#ifndef STDAFX_H // STDAFX_H가 정의 안되어 있다면
#define STDAFX_H // STDAFX_H를 정의


#include <windows.h> //winAPI
#include <string> // string변수
#include <fstream>
#include <iostream>
#include <d3d12.h> //directx12 
#include <dxgi1_6.h> //dxgi
#include <D3Dcompiler.h> //d3d 컴파일러
#include <DirectXMath.h> //DirectX Math 함수를 사용하기위해서
#include "d3dx12.h"
#include <wrl.h> //Microsoft 사용을 위해서
#include <fstream>
#include <wincodec.h>
#include "DirectXMath.h"
#include <unordered_map>

#include <DirectXColors.h> // 다이렉트 x 색
#include "DXHelper.h"


using namespace DirectX;
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#endif //가정 끝(pragma once)와 비슷한 효과  헤더파일이 중복되어 읽어도 괜찮도록 만들어줌