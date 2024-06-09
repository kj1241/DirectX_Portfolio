#define STRICT
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <basetsd.h>
#include <math.h>
#include <stdio.h>
#include <d3dx9.h>
#include <dxerr.h>
#include <tchar.h>
#include <dinput.h>
#include <limits.h>

#include "../Framework/DXUtil.h"
#include "../Framework/D3DEnumeration.h"
#include "../Framework/D3DSettings.h"
#include "../Framework/D3DApp.h"
#include "../Framework/D3DFont.h"
#include "../Framework/D3DFile.h"
#include "../Framework/D3DUtil.h"

#include "../Framework/resource.h"
#include "Compression.h"

#define FORCE_SOFTWARE_VERTEX_PROCESSING 0
#define FORCE_SOFTWARE_PIXEL_PROCESSING 0

CMyD3DApplication* g_pApp  = NULL;
HINSTANCE          g_hInst = NULL;

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    CMyD3DApplication d3dApp;

    g_pApp  = &d3dApp;
    g_hInst = hInst;

    if( FAILED( d3dApp.Create( hInst ) ) )
        return 0;

    return d3dApp.Run();
}

CMyD3DApplication::CMyD3DApplication()
{
    m_dwCreationWidth           = 640;
    m_dwCreationHeight          = 480;
    m_strWindowTitle            = TEXT( "Quantisation" );
    m_d3dEnumeration.AppUsesDepthBuffer   = TRUE;
	m_bStartFullscreen			= false;
	m_bShowCursorWhenFullscreen	= false;

    m_pFont                     = new CD3DFont( _T("Arial"), 10, D3DFONT_BOLD );
    m_bLoadingApp               = TRUE;
    m_pDI                       = NULL;
    m_pKeyboard                 = NULL;

	for(unsigned int i=0; i < MAX_MESHES;i++)
	{
		m_pD3DXMesh[i]              = NULL;
		m_pQNMesh[i]				= NULL;
		m_pSO8Mesh[i]				= NULL;
		m_pCT8Mesh[i]				= NULL;
		m_pSO16Mesh[i]				= NULL;
		m_pCT16Mesh[i]				= NULL;
		m_pSCT16Mesh[i]				= NULL;
		m_pCT10Mesh[i]				= NULL;
		m_pCT101012Mesh[i]				= NULL;

		m_dwControlShader[i] = NULL;
		m_dwQNShader[i] = NULL;
		m_dwSO8Shader[i] = NULL;
		m_dwCT8Shader[i] = NULL;
		m_dwSO16Shader[i] = NULL;
		m_dwCT16Shader[i] = NULL;
		m_dwSCT16Shader[i] = NULL;
		m_dwCT10Shader[i] = NULL;
		m_dwCT101012Shader[i] = NULL;

	}
	m_meshState					= DISPLAY_TEAPOT;

	m_compressState = DISPLAY_QUANT_NORMAL;

    ZeroMemory( &m_UserInput, sizeof(m_UserInput) );
    m_fWorldRotX                = 0.0f;
    m_fWorldRotY                = 0.0f;

    ReadSettings();
}

HRESULT CMyD3DApplication::OneTimeSceneInit()
{
	// 한 번만 초기화 작업을 수행합니다.

	// 앱이 로딩을 완료할 때까지 로딩 상태 메시지를 표시합니다.
	SendMessage(m_hWnd, WM_PAINT, 0, 0);

	// DirectInput을 초기화합니다.
	InitInput(m_hWnd);

	m_bLoadingApp = FALSE;

	return S_OK;
}

//레지스트리에서 앱 설정을 읽어옵니다.

VOID CMyD3DApplication::ReadSettings()
{
	HKEY hkey;
	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, DXAPP_KEY,
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL))
	{
		// 필요에 따라 변경합니다.

		// 저장된 창 너비/높이를 읽어옵니다. 이는 DXUtil_Read*() 함수를 사용하는 예제입니다.
		DXUtil_ReadIntRegKey(hkey, TEXT("Width"), &m_dwCreationWidth, m_dwCreationWidth);
		DXUtil_ReadIntRegKey(hkey, TEXT("Height"), &m_dwCreationHeight, m_dwCreationHeight);

		RegCloseKey(hkey);
	}
}

//앱 설정을 레지스트리에 작성합니다.
VOID CMyD3DApplication::WriteSettings()
{
	HKEY hkey;
	DWORD dwType = REG_DWORD;
	DWORD dwLength = sizeof(DWORD);

	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, DXAPP_KEY,
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL))
	{
		// 필요에 따라 변경합니다.

		// 창 너비/높이를 작성합니다. 
		//이는 DXUtil_Write*() 함수를 사용하는 예제입니다.
		DXUtil_WriteIntRegKey(hkey, TEXT("Width"), m_rcWindowClient.right);
		DXUtil_WriteIntRegKey(hkey, TEXT("Height"), m_rcWindowClient.bottom);

		RegCloseKey(hkey);
	}
}

//DirectInput 객체를 초기화합니다.
HRESULT CMyD3DApplication::InitInput(HWND hWnd)
{
	HRESULT hr;

	// IDirectInput8*를 생성합니다.
	if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
		IID_IDirectInput8, (VOID**)&m_pDI, NULL)))
		return DXTRACE_ERR("DirectInput8Create", hr);

	// 키보드용 IDirectInputDevice8*를 생성합니다.
	if (FAILED(hr = m_pDI->CreateDevice(GUID_SysKeyboard, &m_pKeyboard, NULL)))
		return DXTRACE_ERR("CreateDevice", hr);

	// 키보드 데이터 형식을 설정합니다.
	if (FAILED(hr = m_pKeyboard->SetDataFormat(&c_dfDIKeyboard)))
		return DXTRACE_ERR("SetDataFormat", hr);

	// 키보드의 협력 수준을 설정합니다.
	if (FAILED(hr = m_pKeyboard->SetCooperativeLevel(hWnd,	DISCL_NONEXCLUSIVE |DISCL_FOREGROUND |DISCL_NOWINKEY)))
		return DXTRACE_ERR("SetCooperativeLevel", hr);

	// 키보드를 획득합니다.
	m_pKeyboard->Acquire();

	return S_OK;
}

//디스플레이 장치의 일부 최소한의 기능을 확인합니다.

HRESULT CMyD3DApplication::ConfirmDevice(D3DCAPS9* pCaps, DWORD dwBehavior,	D3DFORMAT Format)
{
	BOOL bCapsAcceptable;

	bCapsAcceptable = TRUE;

	if ((dwBehavior & D3DCREATE_HARDWARE_VERTEXPROCESSING) ||
		(dwBehavior & D3DCREATE_MIXED_VERTEXPROCESSING))
	{
#if FORCE_SOFTWARE_VERTEX_PROCESSING == 0
		if (pCaps->VertexShaderVersion < D3DVS_VERSION(1, 0))
#endif
			bCapsAcceptable = FALSE;
	}

#if FORCE_SOFTWARE_PIXEL_PROCESSING == 1
	if (pCaps->DevCaps & D3DDEVCAPS_HWRASTERIZATION)
		bCapsAcceptable = FALSE;
#endif

	if (bCapsAcceptable)
		return S_OK;
	else
		return E_FAIL;
}


//티팟 행을 초기화합니다.
HRESULT CMyD3DApplication::GenerateTeapotRow( ID3DXMesh** mesh )
{
	HRESULT hRes;
	ID3DXMesh* teaMesh;
	const int numPots = 10;

	// Create a teapot mesh using D3DX
	hRes = D3DXCreateTeapot( m_pd3dDevice, &teaMesh, NULL );
	if( FAILED(hRes) )return hRes;

	hRes = D3DXCreateMeshFVF(  teaMesh->GetNumFaces() * numPots,teaMesh->GetNumVertices() * numPots,teaMesh->GetOptions(),	teaMesh->GetFVF(),m_pd3dDevice,mesh );
	if( FAILED(hRes) )return hRes;
	
	// 	x + z 축을 따라 이동할 때마다 주전자 numPots 번 복사
	float* inStream = 0;
	hRes = teaMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;
	float* outStream = 0;
	hRes = (*mesh)->LockVertexBuffer(0, (void**)&outStream);
	if( FAILED(hRes) ) return hRes;

	WORD* inIBStream = 0;
	hRes = teaMesh->LockIndexBuffer(D3DLOCK_READONLY, (void**)&inIBStream);
	if( FAILED(hRes) ) return hRes;
	WORD* outIBStream = 0;
	hRes = (*mesh)->LockIndexBuffer(0, (void**)&outIBStream);
	if( FAILED(hRes) ) return hRes;

	unsigned int vn = teaMesh->GetNumVertices();
	unsigned int in = teaMesh->GetNumFaces();
	for(unsigned int i=0;i < numPots;++i)
	{
		float randfloat = ((float)rand()) / float(RAND_MAX);
		for(unsigned int j=0;j < vn;++j)
		{
			outStream[(i * vn * 6)+(j*6)+0] = inStream[(j*6)+0] + (8 * i);
			outStream[(i * vn * 6)+(j*6)+1] = inStream[(j*6)+1] + (3 * randfloat);
			outStream[(i * vn * 6)+(j*6)+2] = inStream[(j*6)+2] + (20 * i);
			outStream[(i * vn * 6)+(j*6)+3] = inStream[(j*6)+3];
			outStream[(i * vn * 6)+(j*6)+4] = inStream[(j*6)+4];
			outStream[(i * vn * 6)+(j*6)+5] = inStream[(j*6)+5];
		}
		for(unsigned int j=0;j < in;++j)
		{
			outIBStream[(i * in * 3)+(j*3)+0] = inIBStream[(j*3)+0] + (i * vn);
			outIBStream[(i * in * 3)+(j*3)+1] = inIBStream[(j*3)+1] + (i * vn);
			outIBStream[(i * in * 3)+(j*3)+2] = inIBStream[(j*3)+2] + (i * vn);
		}
	}

	hRes = teaMesh->UnlockVertexBuffer();
	if( FAILED(hRes) ) 
		return hRes;
	hRes = teaMesh->UnlockIndexBuffer();
	if( FAILED(hRes) ) 
		return hRes;
	hRes = (*mesh)->UnlockVertexBuffer();
	if( FAILED(hRes) ) 
		return hRes;
	hRes = (*mesh)->UnlockIndexBuffer();
	if( FAILED(hRes) ) 
		return hRes;

	SAFE_RELEASE( teaMesh );

	return S_OK;
}

// 큐브 행을 초기화합니다.
HRESULT CMyD3DApplication::GenerateCubeRow( ID3DXMesh** mesh )
{
	HRESULT hRes;
	ID3DXMesh* teaMesh;
	const int numPots = 1500;

	// Create a teapot mesh using D3DX
	hRes = D3DXCreateBox( m_pd3dDevice, 1,1,1, &teaMesh, NULL );
	if( FAILED(hRes) )
		return hRes;

	hRes = D3DXCreateMeshFVF(  teaMesh->GetNumFaces() * numPots,teaMesh->GetNumVertices() * numPots,teaMesh->GetOptions(),teaMesh->GetFVF(),m_pd3dDevice,mesh );
	if( FAILED(hRes) 
		)return hRes;
	
	// x + z 축을 따라 이동할 때마다 큐브 numPots번을 복사합니다.
	float* inStream = 0;
	hRes = teaMesh->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;
	float* outStream = 0;
	hRes = (*mesh)->LockVertexBuffer(0, (void**)&outStream);
	if( FAILED(hRes) ) return hRes;

	WORD* inIBStream = 0;
	hRes = teaMesh->LockIndexBuffer(D3DLOCK_READONLY, (void**)&inIBStream);
	if( FAILED(hRes) ) return hRes;
	WORD* outIBStream = 0;
	hRes = (*mesh)->LockIndexBuffer(0, (void**)&outIBStream);
	if( FAILED(hRes) ) return hRes;

	unsigned int vn = teaMesh->GetNumVertices();
	unsigned int in = teaMesh->GetNumFaces();
	for(unsigned int i=0;i < numPots;i++)
	{
	
		float randfloat = ((float)rand()) / float(RAND_MAX);
		for(unsigned int j=0;j < vn;++j)
		{
			outStream[(i * vn * 6)+(j*6)+0] = inStream[(j*6)+0] + (float(i) * 160.f * randfloat);
			outStream[(i * vn * 6)+(j*6)+1] = inStream[(j*6)+1] + (1 * randfloat);
			outStream[(i * vn * 6)+(j*6)+2] = inStream[(j*6)+2] + (float(i) * 160.f * randfloat);
			outStream[(i * vn * 6)+(j*6)+3] = inStream[(j*6)+3];
			outStream[(i * vn * 6)+(j*6)+4] = inStream[(j*6)+4];
			outStream[(i * vn * 6)+(j*6)+5] = inStream[(j*6)+5];
		}
		for(unsigned int j=0;j < in;++j)
		{
			outIBStream[(i * in * 3)+(j*3)+0] = inIBStream[(j*3)+0] + (i * vn);
			outIBStream[(i * in * 3)+(j*3)+1] = inIBStream[(j*3)+1] + (i * vn);
			outIBStream[(i * in * 3)+(j*3)+2] = inIBStream[(j*3)+2] + (i * vn);
		}
	}

	hRes = teaMesh->UnlockVertexBuffer();
	if( FAILED(hRes) ) 
		return hRes;
	hRes = teaMesh->UnlockIndexBuffer();
	if( FAILED(hRes) ) 
		return hRes;
	hRes = (*mesh)->UnlockVertexBuffer();
	if( FAILED(hRes) ) 
		return hRes;
	hRes = (*mesh)->UnlockIndexBuffer();
	if( FAILED(hRes) ) 
		return hRes;

	SAFE_RELEASE( teaMesh );

	return S_OK;
}

//장치 객체 초기화
HRESULT CMyD3DApplication::InitDeviceObjects()
{
	// TODO: 장치 객체 생성
    HRESULT hr;

	// 폰트 초기화
    m_pFont->InitDeviceObjects( m_pd3dDevice );

	for(unsigned int i=0; i < MAX_MESHES; ++i)
	{
		switch(i)
		{
		case DISPLAY_TEAPOT:
			// D3DX를 사용하여 티팟 메쉬 생성
			if( FAILED( hr = D3DXCreateTeapot( m_pd3dDevice, &m_pD3DXMesh[i], NULL ) ) )
				return DXTRACE_ERR( "D3DXCreateTeapot", hr );
			break;
		case DISPLAY_CUBE:
			// D3DX를 사용하여 박스 메쉬 생성
		    if( FAILED( hr = D3DXCreateBox( m_pd3dDevice, 1.0f, 1.0f, 1.0f, &m_pD3DXMesh[i], NULL ) ) )
				return DXTRACE_ERR( "D3DXCreateBox", hr );
			break;
		case DISPLAY_TEAPOT_ROW:
			// 티팟 여러 개를 한 줄로 생성
			if( FAILED( hr = GenerateTeapotRow( &m_pD3DXMesh[i] ) ) )
				return DXTRACE_ERR( "GenerateTeapotRow", hr );
			break;
		case DISPLAY_CUBE_ROW:
			// 박스 여러 개를 한 줄로 생성
			if( FAILED( hr = GenerateCubeRow( &m_pD3DXMesh[i] ) ) )
				return DXTRACE_ERR( "GenerateCubeRow", hr );
			break;

		}
		// 메쉬의 정규를 압축된 버전 생성
		m_pQNMesh[i] = new CompressedMesh( m_pd3dDevice );
		hr = m_pQNMesh[i]->QuantiseNormals( m_pD3DXMesh[i] );
		if( FAILED( hr ) ) 
			return DXTRACE_ERR( "CompressedMesh::QuantiseNormals", hr );

		// 메쉬의 스케일과 오프셋 위치를 8비트로 압축된 버전 생성
		m_pSO8Mesh[i] = new CompressedMesh( m_pd3dDevice );
		hr = m_pSO8Mesh[i]->ScaleAndOffsetPosition8bit( m_pD3DXMesh[i] );
		if( FAILED( hr ) ) 
			return DXTRACE_ERR( "CompressedMesh::ScaleAndOffsetPosition8bit", hr );

		// 메쉬의 변환 위치를 8비트로 압축된 버전 생성
		m_pCT8Mesh[i] = new CompressedMesh( m_pd3dDevice );
		hr = m_pCT8Mesh[i]->CompressionTransformPosition8bit( m_pD3DXMesh[i] );
		if( FAILED( hr ) ) 
			return DXTRACE_ERR( "CompressedMesh::CompressionTransformPosition8bit", hr );

		// 메쉬의 스케일과 오프셋 위치를 16비트로 압축된 버전 생성
		m_pSO16Mesh[i] = new CompressedMesh( m_pd3dDevice );
		hr = m_pSO16Mesh[i]->ScaleAndOffsetPosition16bit( m_pD3DXMesh[i] );
		if( FAILED( hr ) ) 
			return DXTRACE_ERR( "CompressedMesh::ScaleAndOffsetPosition16bit", hr );

		// 메시의 압축된 변환 위치(16비트) 압축 버전을 생성
		m_pCT16Mesh[i] = new CompressedMesh( m_pd3dDevice );
		hr = m_pCT16Mesh[i]->CompressionTransformPosition16bit( m_pD3DXMesh[i] );
		if( FAILED( hr ) ) 
			return DXTRACE_ERR( "CompressedMesh::CompressionTransformPosition16bit", hr );

		// 메시의 슬라이딩 압축 변환 위치(16비트) 압축 버전을 생성
		m_pSCT16Mesh[i] = new CompressedMesh( m_pd3dDevice );
		hr = m_pSCT16Mesh[i]->SlidingCompressionTransformPosition16bit( m_pD3DXMesh[i] );
		if( FAILED( hr ) ) 
			return DXTRACE_ERR( "CompressedMesh::SlidingCompressionTransformPosition16bit", hr );

		// 메시의 압축된 변환 위치(UDEC3N) 압축 버전을 생성
		m_pCT10Mesh[i] = new CompressedMesh( m_pd3dDevice );
		hr = m_pCT10Mesh[i]->CompressionTransformPositionDEC3N( m_pD3DXMesh[i] );
		if( FAILED( hr ) ) 
			return DXTRACE_ERR( "CompressedMesh::CompressionTransformPositionDEC3N", hr );

		// 메시의 압축된 변환 위치(UDEC3N) 압축 버전을 생성
		m_pCT101012Mesh[i] = new CompressedMesh( m_pd3dDevice );
		hr = m_pCT101012Mesh[i]->CompressionTransformPosition101012( m_pD3DXMesh[i] );
		if( FAILED( hr ) ) 
			return DXTRACE_ERR( "CompressedMesh::CompressionTransformPosition101012", hr );

		hr = CreateVertexShaders( i );
		if( FAILED( hr ) ) 
			return DXTRACE_ERR( "CreateVertexShaders", hr );
	}
	return S_OK;
}

//이 데모에서 사용되는 버텍스 셰이더를 생성합니다.
HRESULT CMyD3DApplication::CreateVertexShaders( unsigned int i)
{
	HRESULT		 hr;
    TCHAR        strVertexShaderPath[512];
    LPD3DXBUFFER pCode;
	//--------------------------------------------------------------------------
	// 버텍스 셰이더 파일 찾기
    DXUtil_FindMediaFileCch( strVertexShaderPath, 512, _T("control.vsh") );

	// 파일에서 버텍스 셰이더 어셈블
    if( FAILED( hr = D3DXAssembleShaderFromFile( strVertexShaderPath, 0, 0, NULL, &pCode, NULL ) ) )
        return hr;
	D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];

	hr = m_pD3DXMesh[i]->GetDeclaration(decl);
	if( FAILED( hr ) ) 
		return hr;

	// 고정 기능 버텍스 프로그램 생성
	hr = m_pd3dDevice->CreateVertexShader(  (DWORD*)pCode->GetBufferPointer(),&m_dwControlShader[i] );
	if( FAILED( hr ) ) 
		return hr;
	pCode->Release();

	//--------------------------------------------------------------------------
	// 버텍스 셰이더 파일 찾기
    DXUtil_FindMediaFileCch( strVertexShaderPath, 512, _T("quantNormal.vsh") );

	// 파일에서 버텍스 셰이더 어셈블
    if( FAILED( hr = D3DXAssembleShaderFromFile( strVertexShaderPath,0, 0, NULL, &pCode, NULL ) ) )
        return hr;

	// 고정 기능 버텍스 프로그램 생성
	hr = m_pd3dDevice->CreateVertexShader(  (DWORD*)pCode->GetBufferPointer(),	&m_dwQNShader[i] );
	if( FAILED( hr ) )
		return hr;
	pCode->Release();

	//--------------------------------------------------------------------------
   // 버텍스 셰이더 파일 찾기
    DXUtil_FindMediaFileCch( strVertexShaderPath, 512, _T("so8pos.vsh") );

	// 파일에서 버텍스 셰이더 어셈블
    if( FAILED( hr = D3DXAssembleShaderFromFile( strVertexShaderPath, 
                                                 0, 0, NULL, &pCode, NULL ) ) )
        return hr;

	// 고정 기능 버텍스 프로그램 생성
	hr = m_pd3dDevice->CreateVertexShader(  (DWORD*)pCode->GetBufferPointer(),
											&m_dwSO8Shader[i] );
	if( FAILED( hr ) )
		return hr;
	pCode->Release();

	//--------------------------------------------------------------------------
	// 버텍스 셰이더 파일 찾기
    DXUtil_FindMediaFileCch( strVertexShaderPath, 512, _T("ct8pos.vsh") );

	// 파일에서 버텍스 셰이더 어셈블
    if( FAILED( hr = D3DXAssembleShaderFromFile( strVertexShaderPath, 0, 0, NULL, &pCode, NULL ) ) )
        return hr;

	// 고정 기능 버텍스 프로그램 생성
	hr = m_pd3dDevice->CreateVertexShader(  (DWORD*)pCode->GetBufferPointer(),&m_dwCT8Shader[i] );
	if( FAILED( hr ) ) 
		return hr;
	pCode->Release();

	//--------------------------------------------------------------------------
	// 버텍스 셰이더 파일 찾기
    DXUtil_FindMediaFileCch( strVertexShaderPath, 512, _T("so16pos.vsh") );

	// 파일에서 버텍스 셰이더 어셈블
    if( FAILED( hr = D3DXAssembleShaderFromFile( strVertexShaderPath, 0, 0, NULL, &pCode, NULL ) ) )
        return hr;

	// 고정 기능 버텍스 프로그램 생성
	hr = m_pd3dDevice->CreateVertexShader(  (DWORD*)pCode->GetBufferPointer(),&m_dwSO16Shader[i] );
	if( FAILED( hr ) ) 
		return hr;
	pCode->Release();

	//--------------------------------------------------------------------------
	// 버텍스 셰이더 파일 찾기
    DXUtil_FindMediaFileCch( strVertexShaderPath, 512, _T("ct16pos.vsh") );

	// 파일에서 버텍스 셰이더 어셈블
    if( FAILED( hr = D3DXAssembleShaderFromFile( strVertexShaderPath,  0, 0, NULL, &pCode, NULL ) ) )
        return hr;

	// 고정 기능 버텍스 프로그램 생성
	hr = m_pd3dDevice->CreateVertexShader(  (DWORD*)pCode->GetBufferPointer(),&m_dwCT16Shader[i] );
	if( FAILED( hr ) ) 
		return hr;
	pCode->Release();

	//--------------------------------------------------------------------------
   // 버텍스 셰이더 파일 찾기
    DXUtil_FindMediaFileCch( strVertexShaderPath, 512, _T("sct16pos.vsh") );

	// 파일에서 버텍스 셰이더 어셈블
    if( FAILED( hr = D3DXAssembleShaderFromFile( strVertexShaderPath, 0, 0, NULL, &pCode, NULL ) ) )
        return hr;

	// 고정 기능 버텍스 프로그램 생성
	hr = m_pd3dDevice->CreateVertexShader(  (DWORD*)pCode->GetBufferPointer(),&m_dwSCT16Shader[i] );
	if( FAILED( hr ) )
		return hr;
	pCode->Release();

	//--------------------------------------------------------------------------
	// 버텍스 셰이더 파일 찾기
    DXUtil_FindMediaFileCch( strVertexShaderPath, 512, _T("ct10pos.vsh") );

	// 파일에서 버텍스 셰이더 어셈블
    if( FAILED( hr = D3DXAssembleShaderFromFile( strVertexShaderPath, 0, 0, NULL, &pCode, NULL ) ) )
        return hr;

	// 고정 기능 버텍스 프로그램 생성
	hr = m_pd3dDevice->CreateVertexShader(  (DWORD*)pCode->GetBufferPointer(),&m_dwCT10Shader[i] );
	if( FAILED( hr ) ) 
		return hr;
	pCode->Release();

	//--------------------------------------------------------------------------
	// 버텍스 셰이더 파일 찾기
    DXUtil_FindMediaFileCch( strVertexShaderPath, 512, _T("ct101012pos.vsh") );

	// 파일에서 버텍스 셰이더 어셈블
    if( FAILED( hr = D3DXAssembleShaderFromFile( strVertexShaderPath,  0, 0, NULL, &pCode, NULL ) ) )
        return hr;

	// 고정 기능 버텍스 프로그램 생성
	hr = m_pd3dDevice->CreateVertexShader(  (DWORD*)pCode->GetBufferPointer(),&m_dwCT101012Shader[i] );
	if( FAILED( hr ) ) 
		return hr;
	pCode->Release();

	return S_OK;
}

//장면 객체를 복원합니다.
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
	// TODO: 렌더 상태 설정

	// 텍스처 설정
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

	m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

	// 기타 렌더 상태 설정
    m_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE,   FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,        TRUE );

	// 월드 행렬 설정
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity( &matIdentity );
    m_pd3dDevice->SetTransform( D3DTS_WORLD,  &matIdentity );

	// 뷰 행렬 설정
    D3DXVECTOR3 vFromPt   = D3DXVECTOR3( 0.0f, 0.0f, -5.0f );
    D3DXVECTOR3 vLookatPt = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec    = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &m_matView, &vFromPt, &vLookatPt, &vUpVec );
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &m_matView );

	// 투영 행렬 설정
    FLOAT fAspect = ((FLOAT)m_d3dsdBackBuffer.Width) / m_d3dsdBackBuffer.Height;
    D3DXMatrixPerspectiveFovLH( &m_matProj, D3DX_PI/4, fAspect, 1.0f, 100.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &m_matProj );

	// 폰트 복원
    m_pFont->RestoreDeviceObjects();

    return S_OK;
}

//한 프레임마다 호출되며, 이 호출은 장면을 애니메이션화하는 진입점입니다.
HRESULT CMyD3DApplication::FrameMove()
{
	// TODO: 월드 업데이트

   // 사용자 입력 상태 업데이트
    UpdateInput( &m_UserInput );

	// 사용자 입력에 따라 월드 상태 업데이트
    D3DXMATRIX matRotY;
    D3DXMATRIX matRotX;

    if( m_UserInput.bRotateLeft && !m_UserInput.bRotateRight )
        m_fWorldRotY += m_fElapsedTime;
    else if( m_UserInput.bRotateRight && !m_UserInput.bRotateLeft )
        m_fWorldRotY -= m_fElapsedTime;

    if( m_UserInput.bRotateUp && !m_UserInput.bRotateDown )
        m_fWorldRotX += m_fElapsedTime;
    else if( m_UserInput.bRotateDown && !m_UserInput.bRotateUp )
        m_fWorldRotX -= m_fElapsedTime;

    D3DXMatrixRotationX( &matRotX, m_fWorldRotX );
    D3DXMatrixRotationY( &matRotY, m_fWorldRotY );

    D3DXMatrixMultiply( &m_matWorld, &matRotX, &matRotY );
    m_pd3dDevice->SetTransform( D3DTS_WORLD, &m_matWorld );

	m_original = (m_UserInput.bChange > 0) ? true : false;

	// 압축 방법 선택
	if( m_UserInput.b1 )
		m_compressState = DISPLAY_QUANT_NORMAL;
	else if( m_UserInput.b2 )
		m_compressState = DISPLAY_SO8BIT_POS;
	else if( m_UserInput.b3 )
		m_compressState = DISPLAY_CT8BIT_POS;
	else if( m_UserInput.b4 )
		m_compressState = DISPLAY_SO16BIT_POS;
	else if( m_UserInput.b5 )
		m_compressState = DISPLAY_CT16BIT_POS;
	else if( m_UserInput.b6 )
		m_compressState = DISPLAY_SCT16BIT_POS;
	else if( m_UserInput.b7 )
		m_compressState = DISPLAY_CT10BIT_POS;
	else if( m_UserInput.b8 )
		m_compressState = DISPLAY_CT101012BIT_POS;

	// 객체 선택
	if( m_UserInput.bf5 )
		m_meshState = DISPLAY_TEAPOT;
	else if( m_UserInput.bf6 )
		m_meshState = DISPLAY_CUBE;
	else if( m_UserInput.bf7 )
		m_meshState = DISPLAY_TEAPOT_ROW;
	else if( m_UserInput.bf8 )
		m_meshState = DISPLAY_CUBE_ROW;
    return S_OK;
}

//사용자 입력을 업데이트합니다. 한 프레임마다 호출됩니다.
void CMyD3DApplication::UpdateInput( UserInput* pUserInput )
{
    HRESULT hr;

	// 입력 장치 상태 가져오기
    ZeroMemory( &pUserInput->diks, sizeof(pUserInput->diks) );
    hr = m_pKeyboard->GetDeviceState( sizeof(pUserInput->diks), &pUserInput->diks );
    if( FAILED(hr) ) 
    {
        m_pKeyboard->Acquire();
        return; 
    }

    pUserInput->bRotateLeft  = ( (pUserInput->diks[DIK_LEFT] & 0x80)  == 0x80 );
    pUserInput->bRotateRight = ( (pUserInput->diks[DIK_RIGHT] & 0x80) == 0x80 );
    pUserInput->bRotateUp    = ( (pUserInput->diks[DIK_UP] & 0x80)    == 0x80 );
    pUserInput->bRotateDown  = ( (pUserInput->diks[DIK_DOWN] & 0x80)  == 0x80 );
    pUserInput->bChange		 = ( (pUserInput->diks[DIK_TAB] & 0x80)  == 0x80 );

	pUserInput->b1 = ( (pUserInput->diks[DIK_1] & 0x80)  == 0x80 );
	pUserInput->b2 = ( (pUserInput->diks[DIK_2] & 0x80)  == 0x80 );
	pUserInput->b3 = ( (pUserInput->diks[DIK_3] & 0x80)  == 0x80 );
	pUserInput->b4 = ( (pUserInput->diks[DIK_4] & 0x80)  == 0x80 );
	pUserInput->b5 = ( (pUserInput->diks[DIK_5] & 0x80)  == 0x80 );
	pUserInput->b6 = ( (pUserInput->diks[DIK_6] & 0x80)  == 0x80 );
	pUserInput->b7 = ( (pUserInput->diks[DIK_7] & 0x80)  == 0x80 );
	pUserInput->b8 = ( (pUserInput->diks[DIK_8] & 0x80)  == 0x80 );
	pUserInput->b9 = ( (pUserInput->diks[DIK_9] & 0x80)  == 0x80 );

	pUserInput->bf5 = ( (pUserInput->diks[DIK_F5] & 0x80)  == 0x80 );
	pUserInput->bf6 = ( (pUserInput->diks[DIK_F6] & 0x80)  == 0x80 );
	pUserInput->bf7 = ( (pUserInput->diks[DIK_F7] & 0x80)  == 0x80 );
	pUserInput->bf8 = ( (pUserInput->diks[DIK_F8] & 0x80)  == 0x80 );
	pUserInput->bf9 = ( (pUserInput->diks[DIK_F9] & 0x80)  == 0x80 );
}

HRESULT CMyD3DApplication::Render()
{
	// 뷰포트를 지웁니다.
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x000000ff, 1.0f, 0L );

	// 씬을 시작합니다.
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        D3DXMATRIX mat;
        D3DXMatrixMultiply( &mat, &m_matView, &m_matProj );
        D3DXMatrixMultiply( &mat, &m_matWorld, &mat );
		D3DXMatrixTranspose( &mat, &mat );

        m_pd3dDevice->SetVertexShaderConstantF( 0, (float*) &mat, 4 );

		if( m_original )
		{
			D3DXVECTOR3 const1 = D3DXVECTOR4( 0.5f, 0.f, 0.f, 0.f);
			m_pd3dDevice->SetVertexShaderConstantF( 4, (float*) &const1, 1 );

	        m_pd3dDevice->SetVertexShader( m_dwControlShader[m_meshState] );

			LPDIRECT3DINDEXBUFFER9	pIB;
			LPDIRECT3DVERTEXBUFFER9 pVB;
			m_pD3DXMesh[m_meshState]->GetVertexBuffer( &pVB );
			m_pD3DXMesh[m_meshState]->GetIndexBuffer( &pIB );

			HRESULT hRes;
			hRes = m_pd3dDevice->SetFVF( m_pD3DXMesh[m_meshState]->GetFVF() );
			if( FAILED(hRes) ) 
				return hRes;
			hRes = m_pd3dDevice->SetStreamSource( 0, pVB, 0, D3DXGetFVFVertexSize(m_pD3DXMesh[m_meshState]->GetFVF()) );
			if( FAILED(hRes) ) 
				return hRes;
			hRes = m_pd3dDevice->SetIndices( pIB );
			if( FAILED(hRes) ) 
				return hRes;
			hRes = m_pd3dDevice->DrawIndexedPrimitive(  D3DPT_TRIANGLELIST, 0, 0, m_pD3DXMesh[m_meshState]->GetNumVertices(), 0, m_pD3DXMesh[m_meshState]->GetNumFaces() );
			if( FAILED(hRes) ) 
				return hRes;

			pIB->Release();
			pVB->Release();
      
		} 
		else
		{
			switch( m_compressState )
			{
				case DISPLAY_QUANT_NORMAL:
				{
					m_pd3dDevice->SetVertexShader( m_dwQNShader[m_meshState] );
					m_pQNMesh[m_meshState]->Draw();
					m_pCompressMesh = m_pQNMesh[m_meshState];
					break;
				}
				case DISPLAY_SO8BIT_POS:
				{
					D3DXVECTOR3 sos = m_pSO8Mesh[m_meshState]->GetScaleOffsetScale();
					D3DXVECTOR3 soo = m_pSO8Mesh[m_meshState]->GetScaleOffsetOffset();

					D3DXVECTOR4 sos4( sos[0], sos[1], sos[2], 1.f);
					D3DXVECTOR4 soo4( soo[0], soo[1], soo[2], 1.f);
					m_pd3dDevice->SetVertexShaderConstantF( 4,(float*) &sos4, 1 );
					m_pd3dDevice->SetVertexShaderConstantF( 5,(float*) &soo4, 1 );

					D3DXVECTOR4 const1 = D3DXVECTOR4( 0.5f, 0.f, 0.f, 0.f);
					m_pd3dDevice->SetVertexShaderConstantF( 6,(float*) &const1, 1 );

					m_pd3dDevice->SetVertexShader( m_dwSO8Shader[m_meshState] );
					m_pSO8Mesh[m_meshState]->Draw();
					m_pCompressMesh = m_pSO8Mesh[m_meshState];
					break;
				}
				case DISPLAY_CT8BIT_POS:
				{
					D3DXMATRIX ct = m_pCT8Mesh[m_meshState]->GetCompressionTransfromMatrix();
					D3DXMatrixInverse( &ct, 0, &ct);
					D3DXMatrixTranspose( &ct, &ct );
					m_pd3dDevice->SetVertexShaderConstantF( 4,(float*) &ct, 4 );

					D3DXVECTOR4 const1 = D3DXVECTOR4( 0.5f, 1.f, 0.f, 0.f);
					m_pd3dDevice->SetVertexShaderConstantF( 8,(float*) &const1, 1 );

					m_pd3dDevice->SetVertexShader( m_dwCT8Shader[m_meshState] );
					m_pCT8Mesh[m_meshState]->Draw();
					m_pCompressMesh = m_pCT8Mesh[m_meshState];
					break;
				}
				case DISPLAY_SO16BIT_POS:
				{
					D3DXVECTOR3 sos = m_pSO16Mesh[m_meshState]->GetScaleOffsetScale();
					D3DXVECTOR3 soo = m_pSO16Mesh[m_meshState]->GetScaleOffsetOffset();

					D3DXVECTOR4 sos4( sos[0], sos[1], sos[2], 1.f);
					D3DXVECTOR4 soo4( soo[0], soo[1], soo[2], 1.f);
					m_pd3dDevice->SetVertexShaderConstantF( 4,(float*) &sos4, 1 );
					m_pd3dDevice->SetVertexShaderConstantF( 5,(float*) &soo4, 1 );

					D3DXVECTOR4 const1 = D3DXVECTOR4( 0.5f, 1.f, 1.f / 65535.f, 0.f);
					m_pd3dDevice->SetVertexShaderConstantF( 6,(float*) &const1, 1 );

					m_pd3dDevice->SetVertexShader( m_dwSO16Shader[m_meshState] );
					m_pSO16Mesh[m_meshState]->Draw();
					m_pCompressMesh = m_pSO16Mesh[m_meshState];
					break;
				}
				case DISPLAY_CT16BIT_POS:
				{
					D3DXMATRIX ct = m_pCT16Mesh[m_meshState]->GetCompressionTransfromMatrix();
					D3DXMatrixInverse( &ct, 0, &ct);
					D3DXMatrixTranspose( &ct, &ct );
					m_pd3dDevice->SetVertexShaderConstantF( 4,(float*) &ct, 4 );

					D3DXVECTOR4 const1 = D3DXVECTOR4( 0.5f, 1.f, 1.f / 65535.f, 0.f);
					m_pd3dDevice->SetVertexShaderConstantF( 8,(float*) &const1, 1 );

					m_pd3dDevice->SetVertexShader( m_dwCT16Shader[m_meshState] );
					m_pCT16Mesh[m_meshState]->Draw();
					m_pCompressMesh = m_pCT16Mesh[m_meshState];
					break;
				}
				case DISPLAY_SCT16BIT_POS:
				{
					D3DXMATRIX ct = m_pSCT16Mesh[m_meshState]->GetCompressionTransfromMatrix();
					D3DXMatrixInverse( &ct, 0, &ct);
					D3DXMatrixTranspose( &ct, &ct );
					m_pd3dDevice->SetVertexShaderConstantF( 4,(float*) &ct, 4 );

					// D3DXVECTOR3 a = D3DXVECTOR4(a,b,c,d)와 같은 문제를 해결하기 위해
					// 어글리한 버그를 겪었습니다! (컴파일 오류가 없습니다.)
					D3DXVECTOR4 const1 = D3DXVECTOR4( 0.5f, 256.f, 65536.f, 16777216.f);
					m_pd3dDevice->SetVertexShaderConstantF( 8,(float*) &const1, 1 );

					D3DXVECTOR4 const2 = D3DXVECTOR4( 1.f/32767.f, 1.f, 0.f, 0.f);
					// 1/32767은 실제로 23비트를 24비트로 저장하기 때문에 1/32767이다
					// 이 문제를 해결하기 위해 압축 코드를 분석하여 추가 비트를 얻어내었습니다.
					// 실제로는 23비트가 정점 쉐이더(float에 23비트 가수가 있음)로 들어갑니다.
					// 이 문제를 피하기 위해 문제를 회피했습니다.
					m_pd3dDevice->SetVertexShaderConstantF( 9,(float*) &const2, 1 );

					m_pd3dDevice->SetVertexShader( m_dwSCT16Shader[m_meshState] );
					m_pSCT16Mesh[m_meshState]->Draw();
					m_pCompressMesh = m_pSCT16Mesh[m_meshState];
					break;
				}
				case DISPLAY_CT10BIT_POS:
				{
					D3DXMATRIX ct = m_pCT10Mesh[m_meshState]->GetCompressionTransfromMatrix();
					D3DXMatrixInverse( &ct, 0, &ct);
					D3DXMatrixTranspose( &ct, &ct );
					m_pd3dDevice->SetVertexShaderConstantF( 4,(float*) &ct, 4 );

					D3DXVECTOR4 const1 = D3DXVECTOR4( 0.5f, 1.f, 0.f, 0.f);
					m_pd3dDevice->SetVertexShaderConstantF( 8,(float*) &const1, 1 );

					m_pd3dDevice->SetVertexShader( m_dwCT10Shader[m_meshState] );
					m_pCT10Mesh[m_meshState]->Draw();
					m_pCompressMesh = m_pCT10Mesh[m_meshState];
					break;
				}
				case DISPLAY_CT101012BIT_POS:
				{
					D3DXMATRIX ct = m_pCT101012Mesh[m_meshState]->GetCompressionTransfromMatrix();
					D3DXMatrixInverse( &ct, 0, &ct);
					D3DXMatrixTranspose( &ct, &ct );
					m_pd3dDevice->SetVertexShaderConstantF( 4,(float*) &ct, 4 );

					D3DXVECTOR4 const1 = D3DXVECTOR4( 1/1024.f, 1.f, 1.f/64.f, 0.25f);
					m_pd3dDevice->SetVertexShaderConstantF( 8,(float*) &const1, 1 );

					m_pd3dDevice->SetVertexShader( m_dwCT101012Shader[m_meshState] );
					m_pCT101012Mesh[m_meshState]->Draw();
					m_pCompressMesh = m_pCT101012Mesh[m_meshState];
					break;
				}

			}
		}
		// 렌더링 통계 및 도움말 텍스트를 렌더링합니다.
        RenderText();
		// 씬을 끝냅니다.
        m_pd3dDevice->EndScene();
    }

    return S_OK;
}

//설명: 씬에 통계 및 도움말 텍스트를 렌더링합니다.
HRESULT CMyD3DApplication::RenderText()
{
    D3DCOLOR fontColor        = D3DCOLOR_ARGB(255,255,255,0);
    D3DCOLOR fontWarningColor = D3DCOLOR_ARGB(255,0,255,255);
    TCHAR szMsg[MAX_PATH] = TEXT("");

	// 디스플레이 통계 출력
    FLOAT fNextLine = 80.0f; 

	if( m_original )
	{
		lstrcpy( szMsg, TEXT("Uncompressed mesh") );
		fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
	} else
	{
		switch( m_compressState )
		{
		case DISPLAY_QUANT_NORMAL:
			lstrcpy( szMsg, TEXT("Quantised Normal mesh") ); //정규화된 양자화된 메쉬
			break;
		case DISPLAY_SO8BIT_POS:
			lstrcpy( szMsg, TEXT("Scale Offset 8 bit Position mesh") ); //8비트 스케일 오프셋 위치 메쉬
			break;
		case DISPLAY_CT8BIT_POS:
			lstrcpy( szMsg, TEXT("Compression Transform 8 bit Position mesh") ); // 8비트 위치 압축 변환 메쉬
			break;
		case DISPLAY_SO16BIT_POS:
			lstrcpy( szMsg, TEXT("Scale Offset 16 bit Position mesh") ); //16비트 스케일 오프셋 위치 메쉬
			break;
		case DISPLAY_CT16BIT_POS:
			lstrcpy( szMsg, TEXT("Compression Transform 16 bit Position mesh") ); //16비트 위치 압축 변환 메쉬
			break;
		case DISPLAY_SCT16BIT_POS:
			lstrcpy( szMsg, TEXT("Sliding Compression Transform 16 bit Position mesh") ); //16비트 슬라이딩 압축 변환 메쉬
			break;
		case DISPLAY_CT10BIT_POS:
			lstrcpy( szMsg, TEXT("Compression Transform DEC3N Position mesh") ); //DEC3N 위치 압축 변환 메쉬
			break;
		case DISPLAY_CT101012BIT_POS:
			lstrcpy( szMsg, TEXT("Compression Transform 10,10,12 bit Position mesh") );//10, 10, 12비트(32bit) 위치 압축 변환 메쉬
			break;
		}
		fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
	}
    wsprintf( szMsg, TEXT("Original Size : %d Bytes, Compressed Size : %d Bytes"), 
               D3DXGetFVFVertexSize(m_pD3DXMesh[m_meshState]->GetFVF()), m_pCompressMesh->GetVertexSize() );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

    lstrcpy( szMsg, m_strDeviceStats );
    fNextLine -= 20.0f;
    m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

    lstrcpy( szMsg, m_strFrameStats );
    fNextLine -= 20.0f;
    m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

	// 통계 및 도움말 출력
    fNextLine = (FLOAT) m_d3dsdBackBuffer.Height; 

    lstrcpy( szMsg, TEXT("Use arrow keys to rotate object, Hold TAB to view uncompressed mesh") );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

    lstrcpy( szMsg, TEXT("Press 1 - 8 to change compression method, F5 - F8 to change test mesh") );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

    return S_OK;
}

//주요 WndProc를 재정의하여 샘플이 마우스, 키보드 또는 메뉴 명령을 처리할 수 있게 합니다.
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_PAINT:
        {
            if( m_bLoadingApp )
            {
				// 앱이 로딩 중임을 알리는 창을 그립니다.
				// 필요에 따라 변경하세요.
                HDC hDC = GetDC( hWnd );
                TCHAR strMsg[MAX_PATH];
                wsprintf( strMsg, TEXT("Loading... Please wait") );
                RECT rct;
                GetClientRect( hWnd, &rct );
                DrawText( hDC, strMsg, -1, &rct, DT_CENTER|DT_VCENTER|DT_SINGLELINE );
                ReleaseDC( hWnd, hDC );
            }
            break;
        }

    }

    return CD3DApplication::MsgProc( hWnd, msg, wParam, lParam );
}

//디바이스 오브젝트를 무효화합니다.  
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
	// TODO: RestoreDeviceObjects()에서 생성된 모든 오브젝트를 정리합니다.
    m_pFont->InvalidateDeviceObjects();

    return S_OK;
}

//앱이 종료되거나 디바이스가 변경되는 경우 호출되며, 디바이스 종속적인 오브젝트를 삭제합니다.  
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
    // TODO: InitDeviceObjects()에서 생성된 모든 오브젝트를 정리합니다.
    m_pFont->DeleteDeviceObjects();

	for(unsigned int i=0; i < MAX_MESHES;i++)
	{
		SAFE_RELEASE( m_dwControlShader[i] );
		SAFE_RELEASE( m_dwQNShader[i] );
		SAFE_RELEASE( m_dwSO8Shader[i] );
		SAFE_RELEASE( m_dwCT8Shader[i] );
		SAFE_RELEASE( m_dwSO16Shader[i] );
		SAFE_RELEASE( m_dwCT16Shader[i] );
		SAFE_RELEASE( m_dwSCT16Shader[i] );
		SAFE_RELEASE( m_dwCT10Shader[i] );
		SAFE_RELEASE( m_dwCT101012Shader[i] );

		SAFE_RELEASE( m_pD3DXMesh[i] );
		delete m_pQNMesh[i];
		delete m_pSO8Mesh[i];
		delete m_pCT8Mesh[i];
		delete m_pSO16Mesh[i];
		delete m_pCT16Mesh[i];
		delete m_pSCT16Mesh[i];
		delete m_pCT10Mesh[i];
		delete m_pCT101012Mesh[i];
	}

    return S_OK;
}

//앱이 종료되기 전에 앱이 정리될 수 있도록 합니다.
HRESULT CMyD3DApplication::FinalCleanup()
{
	// D3D 폰트 정리
    SAFE_DELETE( m_pFont );

	// DirectInput 정리
    CleanupDirectInput();

	// 설정을 레지스트리에 작성합니다.
    WriteSettings();

    return S_OK;
}

//DirectInput을 정리합니다.
VOID CMyD3DApplication::CleanupDirectInput()
{
	// DirectX 입력 오브젝트 정리
    SAFE_RELEASE( m_pKeyboard );
    SAFE_RELEASE( m_pDI );

}