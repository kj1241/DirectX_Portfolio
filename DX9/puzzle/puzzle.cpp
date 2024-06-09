#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <iostream>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

LPDIRECT3D9 d3d; 
LPDIRECT3DDEVICE9 d3ddev;
LPD3DXSPRITE d3dspt;

LPDIRECT3DTEXTURE9 sprite;   
LPDIRECT3DTEXTURE9 sprite_hero;   
LPDIRECT3DTEXTURE9 sprite_enemy;  
LPDIRECT3DTEXTURE9 sprite_bullet;  

void initD3D(HWND hWnd);    // sets up and initializes Direct3D
void render_frame(void);    // renders a single frame
void cleanD3D(void);		// closes Direct3D and releases memory
void init_game(void);		//퍼즐 게임 초기화 
void do_game_logic(void);   //퍼즐 게임 로직 처리 

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
using namespace std;
enum { BLACK, RED, HEART };

#define X_NUM 5 
#define Y_NUM 5 

//기본 클래스 
class entity {

public:
	float x_pos;
	float y_pos;
	int status;
	bool bShow;

	void init(float x, float y);
	void hide();
};

void entity::hide()
{
	bShow = false;
}

void entity::init(float x, float y)
{

	x_pos = x;
	y_pos = y;
	status = rand() % 3;
	bShow = true;

}

//객체 생성 
entity block[25];

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"WindowClass";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(NULL, L"WindowClass", L"Our Direct3D Program",
		WS_EX_TOPMOST | WS_POPUP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);
	initD3D(hWnd);
	init_game();

	MSG msg;

	while (TRUE)
	{
		DWORD starting_point = GetTickCount();

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		do_game_logic();
		render_frame();

		// check the 'escape' key
		if (KEY_DOWN(VK_ESCAPE))
			PostMessage(hWnd, WM_DESTROY, 0, 0);

		while ((GetTickCount() - starting_point) < 25);
	}
	cleanD3D();

	return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	} break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void initD3D(HWND hWnd)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = FALSE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = SCREEN_WIDTH;
	d3dpp.BackBufferHeight = SCREEN_HEIGHT;

	d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddev);

	D3DXCreateSprite(d3ddev, &d3dspt);  

	D3DXCreateTextureFromFileEx(d3ddev,   
		L"Panel3.png",    // the file name
		D3DX_DEFAULT,    // default width
		D3DX_DEFAULT,    // default height
		D3DX_DEFAULT,    // no mip mapping
		NULL,    // regular usage
		D3DFMT_A8R8G8B8,    // 32-bit pixels with alpha
		D3DPOOL_MANAGED,    // typical memory handling
		D3DX_DEFAULT,    // no filtering
		D3DX_DEFAULT,    // no mip filtering
		D3DCOLOR_XRGB(255, 0, 255),    // the hot-pink color key
		NULL,    // no image info struct
		NULL,    // not using 256 colors
		&sprite);    // load to sprite

	D3DXCreateTextureFromFileEx(d3ddev,    // the device pointer
		L"hero.png",    // the file name
		D3DX_DEFAULT,    // default width
		D3DX_DEFAULT,    // default height
		D3DX_DEFAULT,    // no mip mapping
		NULL,    // regular usage
		D3DFMT_A8R8G8B8,    // 32-bit pixels with alpha
		D3DPOOL_MANAGED,    // typical memory handling
		D3DX_DEFAULT,    // no filtering
		D3DX_DEFAULT,    // no mip filtering
		D3DCOLOR_XRGB(255, 0, 255),    // the hot-pink color key
		NULL,    // no image info struct
		NULL,    // not using 256 colors
		&sprite_hero);    // load to sprite

	D3DXCreateTextureFromFileEx(d3ddev,    // the device pointer
		L"enemy.png",    // the file name
		D3DX_DEFAULT,    // default width
		D3DX_DEFAULT,    // default height
		D3DX_DEFAULT,    // no mip mapping
		NULL,    // regular usage
		D3DFMT_A8R8G8B8,    // 32-bit pixels with alpha
		D3DPOOL_MANAGED,    // typical memory handling
		D3DX_DEFAULT,    // no filtering
		D3DX_DEFAULT,    // no mip filtering
		D3DCOLOR_XRGB(255, 0, 255),    // the hot-pink color key
		NULL,    // no image info struct
		NULL,    // not using 256 colors
		&sprite_enemy);    // load to sprite


	D3DXCreateTextureFromFileEx(d3ddev,    // the device pointer
		L"bullet.png",    // the file name
		D3DX_DEFAULT,    // default width
		D3DX_DEFAULT,    // default height
		D3DX_DEFAULT,    // no mip mapping
		NULL,    // regular usage
		D3DFMT_A8R8G8B8,    // 32-bit pixels with alpha
		D3DPOOL_MANAGED,    // typical memory handling
		D3DX_DEFAULT,    // no filtering
		D3DX_DEFAULT,    // no mip filtering
		D3DCOLOR_XRGB(255, 0, 255),    // the hot-pink color key
		NULL,    // no image info struct
		NULL,    // not using 256 colors
		&sprite_bullet);    // load to sprite


	return;
}

void init_game()
{
	for (int i = 0; i<X_NUM; i++)
	{
		for (int k = 0; k<Y_NUM; k++)
		{
			block[i*X_NUM + k].init(i * 64, k * 64);
		}
	}
}



void do_game_logic()
{
	int counter = 0;
	int check[4] = { 0,0,0,0 };

	POINT p;
	if (KEY_DOWN(VK_LBUTTON))
	{
		MessageBeep(0);
		GetCursorPos(&p); 
		int x_index = p.x / 64;
		int y_index = p.y / 64;

		//=================================================================================
		//지우기 
		//왼쪽 체크 
		if (block[x_index*X_NUM + y_index].status == block[(x_index - 1)*X_NUM + (y_index)].status)
		{
			if ((x_index) >= 1)
			{
				counter += 1;
				check[0] = 1;
			}
		}
		//오른쪽 체크 
		if (block[x_index*X_NUM + y_index].status == block[(x_index + 1)*X_NUM + (y_index)].status)
		{
			if ((x_index) <= 3)
			{
				counter += 1;
				check[1] = 1;
			}
		}

		////위쪽 체크 
		if (block[x_index*X_NUM + y_index].status == block[(x_index)*X_NUM + (y_index - 1)].status)
		{
			if ((y_index) >= 1)
			{
				counter += 1;
				check[2] = 1;
			}
		}

		////아래쪽 체크 
		if (block[x_index*X_NUM + y_index].status == block[(x_index)*X_NUM + (y_index + 1)].status)
		{
			if ((y_index) <= 3)
			{
				counter += 1;
				check[3] = 1;
			}
		}

		//카운터 체크 
		if (counter >= 1)
		{
			block[(x_index)*X_NUM + (y_index)].hide();

			if (check[0] == 1)
				block[(x_index - 1)*X_NUM + (y_index)].hide();
			if (check[1] == 1)
				block[(x_index + 1)*X_NUM + (y_index)].hide();

			if (check[2] == 1)
				block[(x_index)*X_NUM + (y_index - 1)].hide();
			if (check[3] == 1)
				block[(x_index)*X_NUM + (y_index + 1)].hide();
		}
		//===============================================================================================
	}
}

void render_frame(void)
{
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	d3ddev->BeginScene();    
	d3dspt->Begin(D3DXSPRITE_ALPHABLEND);   

	RECT part;
	SetRect(&part, 0, 0, 64, 64);
	D3DXVECTOR3 center(0.0f, 0.0f, 0.0f);    
	D3DXVECTOR3 position;

	for (int i = 0; i<X_NUM; ++i)
	{
		for (int j = 0; j<Y_NUM; ++j)
		{
			if (block[i*X_NUM + j].bShow == true)
			{
				position = D3DXVECTOR3(block[i*X_NUM + j].x_pos, block[i*X_NUM + j].y_pos, 0.0f);   
				switch (block[i*X_NUM + j].status)
				{
				case RED:
					d3dspt->Draw(sprite_hero, &part, &center, &position, D3DCOLOR_ARGB(255, 255, 255, 255));
					break;
				case HEART:
					d3dspt->Draw(sprite_bullet, &part, &center, &position, D3DCOLOR_ARGB(255, 255, 255, 255));
					break;
				case BLACK:
					d3dspt->Draw(sprite_enemy, &part, &center, &position, D3DCOLOR_ARGB(255, 255, 255, 255));
					break;
				}
			}

		}
	}

	d3dspt->End();
	d3ddev->EndScene();  
	d3ddev->Present(NULL, NULL, NULL, NULL);
	return;
}

void cleanD3D(void)
{
	sprite->Release();
	d3ddev->Release();
	d3d->Release();

	//객체 해제 
	sprite_hero->Release();
	sprite_enemy->Release();
	sprite_bullet->Release();

	return;
}