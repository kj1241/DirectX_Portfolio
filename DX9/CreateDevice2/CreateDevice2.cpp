
//-----------------------------------------------------------------------------
#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )




//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
LPDIRECT3D9             g_pD3D = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // Our rendering device
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; // Buffer to hold vertices

// A structure for our custom vertex type
struct CUSTOMVERTEX
{
	FLOAT x, y, z;      // The untransformed, 3D position for the vertex
	DWORD color;        // The vertex color
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)




//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT InitD3D(HWND hWnd)
{
	// Create the D3D object.
	if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		return E_FAIL;

	// Set up the structure used to create the D3DDevice
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	// Create the D3DDevice
	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}

	// Turn off culling, so we see the front and back of the triangle
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// Turn off D3D lighting, since we are providing our own vertex colors
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitGeometry()
// Desc: Creates the scene geometry
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
	// Initialize three vertices for rendering a triangle
	CUSTOMVERTEX g_Vertices[] =
	{
		{ 0.0f,   0.0f, 0.0f, 0x000000ff, },
		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 0.0f,   -10.0f, 0.0f, 0x000000ff, },

		{ 0.0f,   0.0f, 0.0f, 0x000000ff, },
		{ 0.0f,   -10.0f, 0.0f, 0x000000ff, },
		{ 5.0f,  -5.0f, 0.0f, 0x0000ffff, },

		////////////////////////////////////////////////

		{ 0.0f,   0.0f, 0.0f, 0x000000ff, },
		{ -55.0f,   20.0f, 0.0f, 0xffffff00, },
		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },

		////////////////////////////////////////////////


		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -55.0f,   20.0f, 0.0f, 0xffffff00, },
		{ -54.0f,   15.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -54.0f,   15.0f, 0.0f, 0xffffff00, },
		{ -52.0f,   10.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -52.0f,   10.0f, 0.0f, 0xffffff00, },
		{ -50.0f,   5.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -50.0f,   5.0f, 0.0f, 0xffffff00, },
		{ -51.0f,   0.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -51.0f,   0.0f, 0.0f, 0xffffff00, },
		{ -52.0f,   -5.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -52.0f,   -5.0f, 0.0f, 0xffffff00, },
		{ -54.0f,   -10.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -54.0f,   -10.0f, 0.0f, 0xffffff00, },
		{ -55.0f,   -15.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -55.0f,   -15.0f, 0.0f, 0xffffff00, },
		{ -55.0f,   -20.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -55.0f,   -20.0f, 0.0f, 0xffffff00, },
		{ -54.0f,   -25.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -54.0f,   -25.0f, 0.0f, 0xffffff00, },
		{ -52.0f,   -30.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -52.0f,   -30.0f, 0.0f, 0xffffff00, },
		{ -50.0f,   -35.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -50.0f,   -35.0f, 0.0f, 0xffffff00, },
		{ -47.0f,   -40.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -47.0f,   -40.0f, 0.0f, 0xffffff00, },
		{ -45.0f,   -42.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -45.0f,   -42.0f, 0.0f, 0xffffff00, },
		{ -42.0f,   -45.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -42.0f,   -45.0f, 0.0f, 0xffffff00, },
		{ -40.0f,   -47.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -40.0f,   -47.0f, 0.0f, 0xffffff00, },
		{ -35.0f,   -50.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -35.0f,   -50.0f, 0.0f, 0xffffff00, },
		{ -30.0f,   -47.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -30.0f,   -47.0f, 0.0f, 0xffffff00, },
		{ -27.0f,   -45.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -27.0f,   -45.0f, 0.0f, 0xffffff00, },
		{ -25.0f,   -42.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -25.0f,   -42.0f, 0.0f, 0xffffff00, },
		{ -22.0f,   -40.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -22.0f,   -40.0f, 0.0f, 0xffffff00, },
		{ -20.0f,   -37.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -20.0f,   -37.0f, 0.0f, 0xffffff00, },
		{ -19.0f,   -35.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -19.0f,   -35.0f, 0.0f, 0xffffff00, },
		{ -15.0f,   -30.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -15.0f,   -30.0f, 0.0f, 0xffffff00, },
		{ -10.0f,   -20.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -10.0f,   -20.0f, 0.0f, 0xffffff00, },
		{ -8.0f,   -15.0f, 0.0f, 0xffffff00, },

		{ -5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ -8.0f,   -15.0f, 0.0f, 0xffffff00, },
		{ -7.0f,   -10.0f, 0.0f, 0xffffff00, },

		////////////////////////////////////////////////

		{ 0.0f,   0.0f, 0.0f, 0x000000ff, },
		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 55.0f,   20.0f, 0.0f, 0xffffff00, },

		////////////////////////////////////////////////

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 7.0f,   -10.0f, 0.0f, 0xffffff00, },
		{ 8.0f,   -15.0f, 0.0f, 0xffffff00, },


		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 8.0f,   -15.0f, 0.0f, 0xffffff00, },
		{ 10.0f,   -20.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 10.0f,   -20.0f, 0.0f, 0xffffff00, },
		{ 12.0f,   -25.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 12.0f,   -25.0f, 0.0f, 0xffffff00, },
		{ 15.0f,   -30.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 15.0f,   -30.0f, 0.0f, 0xffffff00, },
		{ 19.0f,   -35.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 19.0f,   -35.0f, 0.0f, 0xffffff00, },
		{ 20.0f,   -38.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 20.0f,   -38.0f, 0.0f, 0xffffff00, },
		{ 22.0f,   -40.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 22.0f,   -40.0f, 0.0f, 0xffffff00, },
		{ 25.0f,   -42.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 25.0f,   -42.0f, 0.0f, 0xffffff00, },
		{ 30.0f,   -47.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 30.0f,   -47.0f, 0.0f, 0xffffff00, },
		{ 35.0f,   -50.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 35.0f,   -50.0f, 0.0f, 0xffffff00, },
		{ 40.0f,   -47.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 40.0f,   -47.0f, 0.0f, 0xffffff00, },
		{ 42.0f,   -45.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 42.0f,   -45.0f, 0.0f, 0xffffff00, },
		{ 45.0f,   -42.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 45.0f,   -42.0f, 0.0f, 0xffffff00, },
		{ 47.0f,   -40.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 47.0f,   -40.0f, 0.0f, 0xffffff00, },
		{ 50.0f,   -35.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 50.0f,   -35.0f, 0.0f, 0xffffff00, },
		{ 52.0f,   -30.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 52.0f,   -30.0f, 0.0f, 0xffffff00, },
		{ 54.0f,   -25.0f, 0.0f, 0xffffff00, },

		{ 5.0f,   -5.0f, 0.0f, 0x0000ffff, },
		{ 54.0f,   -25.0f, 0.0f, 0xffffff00, },
		{ 55.0f,   -20.0f, 0.0f, 0xffffff00, },

		///////////////////////////////////////////////

		{ 0.0f,   5.0f, 0.0f, 0x000000ff, },
		{ 5.0f,   15.0f, 0.0f, 0x0000ffff, },
		{ 4.0f,   20.0f, 0.0f, 0xffffff00, },

		{ 0.0f,   10.0f, 0.0f, 0x000000ff, },
		{ 4.0f,   20.0f, 0.0f, 0xffffff00, },
		{ 0.0f,   25.0f, 0.0f, 0x000000ff, },

		{ 0.0f,   5.0f, 0.0f, 0x000000ff, },
		{ 0.0f,   25.0f, 0.0f, 0x000000ff, },
		{ -4.0f,  20.0f, 0.0f, 0xffffff00, },

		{ 0.0f,   10.0f, 0.0f, 0x000000ff, },
		{ -4.0f,  20.0f, 0.0f, 0xffffff00, },
		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },

		////////////////////////////////////////////////

		{ 0.0f,   10.0f, 0.0f, 0x000000ff, },
		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -55.0f,  25.0f, 0.0f, 0xffffff00, },

		///////////////////////////////////////////////

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -10.0f,  20.0f, 0.0f, 0xffffff00, },
		{ -15.0f,  25.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -15.0f,  25.0f, 0.0f, 0xffffff00, },
		{ -20.0f,  29.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -20.0f,  29.0f, 0.0f, 0xffffff00, },
		{ -21.0f,  30.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -21.0f,  30.0f, 0.0f, 0xffffff00, },
		{ -25.0f,  34.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -25.0f,  34.0f, 0.0f, 0xffffff00, },
		{ -27.0f,  35.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -27.0f,  35.0f, 0.0f, 0xffffff00, },
		{ -30.0f,  37.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -30.0f,  37.0f, 0.0f, 0xffffff00, },
		{ -35.0f,  40.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -35.0f,  40.0f, 0.0f, 0xffffff00, },
		{ -40.0f,  37.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -40.0f,  37.0f, 0.0f, 0xffffff00, },
		{ -34.0f,  35.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -34.0f,  35.0f, 0.0f, 0xffffff00, },
		{ -45.0f,  34.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -45.0f,  34.0f, 0.0f, 0xffffff00, },
		{ -50.0f,  30.0f, 0.0f, 0xffffff00, },

		{ -5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ -50.0f,  30.0f, 0.0f, 0xffffff00, },
		{ -55.0f,  25.0f, 0.0f, 0xffffff00, },

		////////////////////////////////////////////////////

		{ 0.0f,  5.0f, 0.0f, 0x000000ff, },
		{ 55.0f,  -15.0f, 0.0f, 0xffffff00, },
		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },

		///////////////////////////////////////////////////

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 55.0f,  -15.0f, 0.0f, 0xffffff00, },
		{ 53.0f,  -10.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 53.0f,  -10.0f, 0.0f, 0xffffff00, },
		{ 52.0f,  -5.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 52.0f,  -5.0f, 0.0f, 0xffffff00, },
		{ 51.0f,  0.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 51.0f,  0.0f, 0.0f, 0xffffff00, },
		{ 50.0f,  5.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 50.0f,  5.0f, 0.0f, 0xffffff00, },
		{ 52.0f,  10.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 52.0f,  10.0f, 0.0f, 0xffffff00, },
		{ 54.0f,  15.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 54.0f,  15.0f, 0.0f, 0xffffff00, },
		{ 55.0f,  20.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 55.0f,  20.0f, 0.0f, 0xffffff00, },
		{ 55.0f,  25.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 55.0f,  25.0f, 0.0f, 0xffffff00, },
		{ 50.0f,  30.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 50.0f,  30.0f, 0.0f, 0xffffff00, },
		{ 45.0f,  34.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 45.0f,  34.0f, 0.0f, 0xffffff00, },
		{ 44.0f,  35.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 44.0f,  35.0f, 0.0f, 0xffffff00, },
		{ 40.0f,  37.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 40.0f,  37.0f, 0.0f, 0xffffff00, },
		{ 35.0f,  40.0f, 0.0f, 0xffffff00, },


		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 35.0f,  40.0f, 0.0f, 0xffffff00, },
		{ 30.0f,  37.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 30.0f,  37.0f, 0.0f, 0xffffff00, },
		{ 27.0f,  35.0f, 0.0f, 0xffffff00, },


		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 27.0f,  35.0f, 0.0f, 0xffffff00, },
		{ 25.0f,  34.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 25.0f,  34.0f, 0.0f, 0xffffff00, },
		{ 21.0f,  30.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 21.0f,  30.0f, 0.0f, 0xffffff00, },
		{ 20.0f,  29.0f, 0.0f, 0xffffff00, },

		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 20.0f,  29.0f, 0.0f, 0xffffff00, },
		{ 15.0f,  25.0f, 0.0f, 0xffffff00, },


		{ 5.0f,  15.0f, 0.0f, 0x0000ffff, },
		{ 15.0f,  25.0f, 0.0f, 0xffffff00, },
		{ 10.0f,  20.0f, 0.0f, 0xffffff00, },

	};

	// Create the vertex buffer.
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(1500 * sizeof(CUSTOMVERTEX),
		0, D3DFVF_CUSTOMVERTEX,
		D3DPOOL_DEFAULT, &g_pVB, NULL)))
	{
		return E_FAIL;
	}

	// Fill the vertex buffer.
	VOID* pVertices;
	if (FAILED(g_pVB->Lock(0, sizeof(g_Vertices), (void**)&pVertices, 0)))
		return E_FAIL;
	memcpy(pVertices, g_Vertices, sizeof(g_Vertices));
	g_pVB->Unlock();

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
VOID Cleanup()
{
	if (g_pVB != NULL)
		g_pVB->Release();

	if (g_pd3dDevice != NULL)
		g_pd3dDevice->Release();

	if (g_pD3D != NULL)
		g_pD3D->Release();
}



//-----------------------------------------------------------------------------
// Name: SetupMatrices()
// Desc: Sets up the world, view, and projection transform Matrices.
//-----------------------------------------------------------------------------
VOID SetupMatrices()
{
	// For our world matrix, we will just rotate the object about the y-axis.
	D3DXMATRIXA16 matWorld;

	// Set up the rotation matrix to generate 1 full rotation (2*PI radians) 
	// every 1000 ms. To avoid the loss of precision inherent in very high 
	// floating point numbers, the system time is modulated by the rotation 
	// period before conversion to a radian angle.
	//UINT iTime = timeGetTime() % 1000;
	//FLOAT fAngle = iTime * ( 2.0f * D3DX_PI ) / 1000.0f;

	FLOAT a = 0;
	D3DXMatrixRotationY(&matWorld, a);
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

	// Set up our view matrix. A view matrix can be defined given an eye point,
	// a point to lookat, and a direction for which way is up. Here, we set the
	// eye five units back along the z-axis and up three units, look at the
	// origin, and define "up" to be in the y-direction.
	D3DXVECTOR3 vEyePt(0.0f, 0.0f, -100.0f);
	D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
	D3DXMATRIXA16 matView;
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
	g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

	// For the projection matrix, we set up a perspective transform (which
	// transforms geometry from 3D view space to 2D viewport space, with
	// a perspective divide making objects smaller in the distance). To build
	// a perpsective transform, we need the field of view (1/4 pi is common),
	// the aspect ratio, and the near and far clipping planes (which define at
	// what distances geometry should be no longer be rendered).
	D3DXMATRIXA16 matProj;
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
	g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}



//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
VOID Render()
{
	// Clear the backbuffer to a black color
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

	// Begin the scene
	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		// Setup the world, view, and projection Matrices
		SetupMatrices();

		// Render the vertex buffer contents
		g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);
		g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 300);

		// End the scene
		g_pd3dDevice->EndScene();
	}

	// Present the backbuffer contents to the display
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, INT)
{
	UNREFERENCED_PARAMETER(hInst);

	// Register the window class
	WNDCLASSEX wc =
	{
		sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		L"D3D Tutorial", NULL
	};
	RegisterClassEx(&wc);

	// Create the application's window
	HWND hWnd = CreateWindow(L"D3D Tutorial", L"D3D Tutorial 03: Matrices",
		WS_OVERLAPPEDWINDOW, 100, 100, 256, 256,
		NULL, NULL, wc.hInstance, NULL);

	// Initialize Direct3D
	if (SUCCEEDED(InitD3D(hWnd)))
	{
		// Create the scene geometry
		if (SUCCEEDED(InitGeometry()))
		{
			// Show the window
			ShowWindow(hWnd, SW_SHOWDEFAULT);
			UpdateWindow(hWnd);

			// Enter the message loop
			MSG msg;
			ZeroMemory(&msg, sizeof(msg));
			while (msg.message != WM_QUIT)
			{
				if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
					Render();
			}
		}
	}

	UnregisterClass(L"D3D Tutorial", wc.hInstance);
	return 0;
}



