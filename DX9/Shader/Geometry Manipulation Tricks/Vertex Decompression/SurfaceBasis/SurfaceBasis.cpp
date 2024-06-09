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
#include "../Framework/DXUtil.h"
#include "../Framework/D3DEnumeration.h"
#include "../Framework/D3DSettings.h"
#include "../Framework/D3DApp.h"
#include "../Framework/D3DFont.h"
#include "../Framework/D3DFile.h"
#include "../Framework/D3DUtil.h"
#include "../Framework/resource.h"
#include "SurfaceBasis.h"
#include <stack>

CMyD3DApplication* g_pApp  = NULL;
HINSTANCE          g_hInst = NULL;

// 이로 인해 무게 중심 좌표가 양자화되고 현재 구멍이 발생합니다.
const bool bFloatNPatch = true;

enum BarycentricPrecision
{
	BYTE_BARYCENTRIC,
	WORD_BARYCENTRIC,
	FLOAT_BARYCENTRIC,
};
const enum BarycentricPrecision gBarycentricPrecision = FLOAT_BARYCENTRIC;

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    CMyD3DApplication d3dApp;

    g_pApp  = &d3dApp;
    g_hInst = hInst;

    InitCommonControls();
    if( FAILED( d3dApp.Create( hInst ) ) )
        return 0;

    return d3dApp.Run();
}

CMyD3DApplication::CMyD3DApplication()
{
    m_dwCreationWidth           = 500;
    m_dwCreationHeight          = 375;
    m_strWindowTitle            = TEXT( "SurfaceBasis" );
    m_d3dEnumeration.AppUsesDepthBuffer   = TRUE;
	m_bStartFullscreen			= false;
	m_bShowCursorWhenFullscreen	= false;

    m_pFont                     = new CD3DFont( _T("Arial"), 12, D3DFONT_BOLD );
    m_bLoadingApp               = TRUE;
    m_pD3DXMesh                 = NULL;
	m_pD3DXPatchMesh			= NULL;
	m_pD3DXPatchMeshDest		= NULL;
    m_pDI                       = NULL;
    m_pKeyboard                 = NULL;

	m_pBaryDecl					= NULL;
	m_pNPatchDecl				= NULL;
	m_pLinearDecl				= NULL;

	m_pVB						= NULL; 
	
	m_baryFX					= NULL;
	m_npatchFX					= NULL;
	
	m_posVertexData				= NULL;
	m_normVertexData			= NULL;
	m_indexData					= NULL;

	m_numVertices				= 0;
	m_numIndices				= 0;

	m_baryPos					= NULL;
	m_npatchPos					= NULL;
	m_npatchNorm				= NULL;

	m_linearPos					= NULL;
	m_linearNorm				= NULL;

    ZeroMemory( &m_UserInput, sizeof(m_UserInput) );
    m_fWorldRotX                = 0.0f;
    m_fWorldRotY                = 0.0f;

	m_displayMethod =			DM_BARYCENTRIC;

    // 레지스트리에서 설정 읽기
    ReadSettings();
}

CMyD3DApplication::~CMyD3DApplication()
{
}

//FinalCleanup()과 쌍을 이룹니다.
// 창이 생성되었고 IDirect3D9 인터페이스가 생성되었습니다.
// 생성되었지만 아직 장치가 생성되지 않았습니다.  여기에서 할 수 있습니다
// 애플리케이션 관련 초기화 및 정리를 수행합니다.
// 장치에 의존하지 않습니다.
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
	// TODO: 일회성 초기화 수행
	// 앱 로딩이 완료될 때까지 로딩 상태 메시지 그리기
    SendMessage( m_hWnd, WM_PAINT, 0, 0 );
	// DirectInput 초기화
    InitInput( m_hWnd );

    m_bLoadingApp = FALSE;
    return S_OK;
}

//레지스트리에서 앱 설정 읽기
VOID CMyD3DApplication::ReadSettings()
{
    HKEY hkey;
    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, DXAPP_KEY,  0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
    {
		// TODO: 필요에 따라 변경

		// 저장된 창 너비/높이를 읽습니다.  이것은 단지 예일 뿐이며,
		// DXUtil_Read*() 함수를 사용하는 방법입니다.
        DXUtil_ReadIntRegKey( hkey, TEXT("Width"), &m_dwCreationWidth, m_dwCreationWidth );
        DXUtil_ReadIntRegKey( hkey, TEXT("Height"), &m_dwCreationHeight, m_dwCreationHeight );

        RegCloseKey( hkey );
    }
}

//레지스트리에 앱 설정 쓰기
VOID CMyD3DApplication::WriteSettings()
{
    HKEY hkey;

    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, DXAPP_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
    {
		// TODO: 필요에 따라 변경
		
		// 창 너비/높이를 씁니다.  이것은 단지 예일 뿐이며, DXUtil_Write*() 함수를 사용하는 방법입니다.
        DXUtil_WriteIntRegKey( hkey, TEXT("Width"), m_rcWindowClient.right );
        DXUtil_WriteIntRegKey( hkey, TEXT("Height"), m_rcWindowClient.bottom );

        RegCloseKey( hkey );
    }
}

//DirectInput 객체 초기화
HRESULT CMyD3DApplication::InitInput( HWND hWnd )
{
    HRESULT hr;

	// IDirectInput8* 생성
    if( FAILED( hr = DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&m_pDI, NULL ) ) )
        return DXTRACE_ERR( "DirectInput8Create", hr );
    
	// 키보드용 IDirectInputDevice8*을 생성합니다.
    if( FAILED( hr = m_pDI->CreateDevice( GUID_SysKeyboard, &m_pKeyboard, NULL ) ) )
        return DXTRACE_ERR( "CreateDevice", hr );
    
	// 키보드 데이터 형식을 설정합니다.
    if( FAILED( hr = m_pKeyboard->SetDataFormat( &c_dfDIKeyboard ) ) )
        return DXTRACE_ERR( "SetDataFormat", hr );
    
	// 키보드에서 협동 수준을 설정합니다.
    if( FAILED( hr = m_pKeyboard->SetCooperativeLevel( hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND |  DISCL_NOWINKEY ) ) )
        return DXTRACE_ERR( "SetCooperativeLevel", hr );

	// 키보드 획득
    m_pKeyboard->Acquire();

    return S_OK;
}

// 기기 초기화 중에 호출되는 이 코드는 디스플레이 기기를 확인합니다. 최소한의 기능 세트에 대해
HRESULT CMyD3DApplication::ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior, D3DFORMAT Format )
{
    UNREFERENCED_PARAMETER( Format );
    UNREFERENCED_PARAMETER( dwBehavior );
    UNREFERENCED_PARAMETER( pCaps );
    
    BOOL bCapsAcceptable;

	// TODO: 이러한 디스플레이 캡이 허용되는지 확인하기 위해 검사를 수행합니다.
    bCapsAcceptable = TRUE;

    if( bCapsAcceptable )         
        return S_OK;
    else
        return E_FAIL;
}

// DeleteDeviceObjects()와 페어링됨
// 장치가 생성되었습니다.  손실되지 않는 리소스 여기에서 Reset()을 생성할 수 있습니다. 
// -- D3DPOOL_MANAGED의 리소스, D3DPOOL_SCRATCH 또는 D3DPOOL_SYSTEMMEM.  
// 다음을 통해 생성된 이미지 표면 CreateImageSurface는 손실되지 않으며 여기에서 생성할 수 있습니다.  
// 버텍스 셰이더와 픽셀 셰이더는 여기에서 생성될 수도 있습니다.
// Reset()에서 손실
HRESULT CMyD3DApplication::InitDeviceObjects()
{
	// TODO: 장치 객체 생성

    HRESULT hr;

	// 글꼴 초기화
    m_pFont->InitDeviceObjects( m_pd3dDevice );

	// D3DX를 사용하여 큐브 메시 생성
    if( FAILED( hr = D3DXCreateBox( m_pd3dDevice,  1, 1, 1, &m_pD3DXMesh, NULL ) ) )
        return DXTRACE_ERR( "D3DXCreateTeapot", hr );

	D3DXWELDEPSILONS Epsilons;
	memset( &Epsilons, 0, sizeof(D3DXWELDEPSILONS) );
	Epsilons.Normal = 100000;
	D3DXWeldVertices( m_pD3DXMesh,  0, &Epsilons, 0, 0, 0, 0 );

	// 평균 법선을 제공합니다(따라서 n-패치가 더 흥미롭습니다).
	D3DXComputeNormals(m_pD3DXMesh, 0 );

	// 제어 n-패치에 대한 패치 메시를 생성합니다.
    if( FAILED( hr = D3DXCreateNPatchMesh( m_pD3DXMesh, &m_pD3DXPatchMesh ) ) )
        return DXTRACE_ERR( "D3DXCreateNPatchMesh", hr );

	// 테셀레이션 속도 향상
	if( FAILED( hr = m_pD3DXPatchMesh->Optimize( 0 ) ) )
        return DXTRACE_ERR( "m_pD3DXPatchMesh->Optimize", hr );

	if( FAILED( hr = GenerateD3DNPatchMesh( 1 ) ) )
        return DXTRACE_ERR( "GenerateD3DNPatchMesh", hr );

	GetXMeshData( m_pD3DXMesh );

	// 기본 무게 중심 좌표
	if( FAILED( hr = D3DXCreateEffectFromFile( m_pd3dDevice, "control.fx", 0, 0, 0, 0, &m_controlFX, 0 ) ) )
		return DXTRACE_ERR( "D3DXCreateEffectFromFile", hr );

	// 기본 무게 중심 좌표
	if( FAILED( hr = D3DXCreateEffectFromFile( m_pd3dDevice, "bary.fx", 0, 0, 0, 0, &m_baryFX, 0 ) ) )
		return DXTRACE_ERR( "D3DXCreateEffectFromFile", hr );

	// n패치
	if( FAILED( hr = D3DXCreateEffectFromFile( m_pd3dDevice, "npatch.fx", 0, 0, 0, 0, &m_npatchFX, 0 ) ) )
		return DXTRACE_ERR( "D3DXCreateEffectFromFile", hr );

	// n패치
	if( FAILED( hr = D3DXCreateEffectFromFile( m_pd3dDevice, "linear.fx", 0, 0, 0, 0, &m_linearFX, 0 ) ) )
		return DXTRACE_ERR( "D3DXCreateEffectFromFile", hr );

	// 무게 중심 좌표 방법이 기본값입니다.
	if( FAILED( hr = GenerateBaryConstants() ) )
		return DXTRACE_ERR( "GenerateBaryConstants", hr );
	
    return S_OK;
}

HRESULT CMyD3DApplication::GetXMeshData( LPD3DXMESH in )
{
	HRESULT hRes;

	// 입력 정점 데이터를 얻습니다.
	float* inStream = 0;
	hRes = in->LockVertexBuffer(D3DLOCK_READONLY, (void**)&inStream);
	if( FAILED(hRes) ) return hRes;

	m_numVertices = in->GetNumVertices();

	// 정점을 저장하기 위해 일부 메모리를 할당합니다.
	m_posVertexData = new float[ m_numVertices * 3 ];
	m_normVertexData = new float[ m_numVertices * 3 ];

	// 정점 데이터를 복사합니다.
	for(unsigned int i=0;i < m_numVertices;i++)
	{
		m_posVertexData[(i*3)+0] = inStream[0];
		m_posVertexData[(i*3)+1] = inStream[1];
		m_posVertexData[(i*3)+2] = inStream[2];
		m_normVertexData[(i*3)+0] = inStream[3];
		m_normVertexData[(i*3)+1] = inStream[4];
		m_normVertexData[(i*3)+2] = inStream[5];

		// 다음 정점
		inStream += 6;
	}
	
	hRes = in->UnlockVertexBuffer();
	if( FAILED(hRes) ) return hRes;

	// 이제 인덱스를 복사합니다.
	m_numIndices = in->GetNumFaces() * 3;

	// 16비트 인덱스만
	const unsigned int ibsize = m_numIndices * sizeof(WORD);

	m_indexData = new WORD[ m_numIndices ];

	WORD* inIBStream = 0;
	hRes = in->LockIndexBuffer( D3DLOCK_READONLY, (void**)&inIBStream);
	if( FAILED(hRes) ) return hRes;

	memcpy(m_indexData, inIBStream, ibsize);

	// 이제 두 인덱스 버퍼를 모두 잠금 해제합니다.
	hRes = in->UnlockIndexBuffer();
	if( FAILED(hRes) ) return hRes;

	return S_OK;
}

HRESULT CMyD3DApplication::GenerateBaryConstants()
{
	HRESULT hRes;
	// 먼저 정점 데이터의 색인을 해제합니다.
	m_baryPos = new float[ m_numIndices * 4];
	for(unsigned int i=0; i < m_numIndices;i++)
	{
		m_baryPos[ (i*4) + 0 ] = m_posVertexData[ (m_indexData[i]*3) + 0 ];
		m_baryPos[ (i*4) + 1 ] = m_posVertexData[ (m_indexData[i]*3) + 1 ];
		m_baryPos[ (i*4) + 2 ] = m_posVertexData[ (m_indexData[i]*3) + 2 ];
		m_baryPos[ (i*4) + 3 ] = 0;
	}

	const unsigned int vbSize = sizeof(unsigned int) * m_numIndices;

	hRes = m_pd3dDevice->CreateVertexBuffer( vbSize, 0, 0,D3DPOOL_MANAGED, &m_pVB, NULL );
	if( FAILED(hRes) ) return hRes;

	unsigned int* outStream = 0;
	hRes = m_pVB->Lock(0,0, (void**)&outStream, 0 );
	if( FAILED(hRes) ) return hRes;

	// 면적(정규화된 무게 중심) 좌표를 계산합니다.
	for( unsigned int index=0; index < m_numIndices/3;index++)
	{
		// 첫 번째 정점 : i=1, j=k=0
		// 두 번째 정점 : j=1, i=k=0
		// 세 번째 정점 : k=1, i=j=0 (단, k는 저장되지 않음)

		unsigned int packVert;
		unsigned int i,j,k;

		i = 255; j = k = 0; // 첫 번째 정점
		packVert = (i << 16) + (j << 8) + (index << 0); // D3DCOLOR 압축 풀기에 적합
		outStream[ (index*3) + 0 ] = packVert;
		j = 255; i = k = 0; // 두 번째 정점
		packVert = (i << 16) + (j << 8) + (index << 0); // D3DCOLOR 압축 풀기에 적합
		outStream[ (index*3) + 1 ] = packVert;
		k = 255; i = j = 0; // 세 번째 정점
		packVert = (i << 16) + (j << 8) + (index << 0); // D3DCOLOR 압축 풀기에 적합
		outStream[ (index*3) + 2 ] = packVert;
	}

	// 이제 정점 버퍼의 잠금을 해제합니다.
	hRes = m_pVB->Unlock();
	if( FAILED(hRes) ) return hRes;

	D3DVERTEXELEMENT9	Decl[MAX_FVF_DECL_SIZE];
	// 선언 D3D 생성(고정 기능 레지스터)
	Decl[0].Stream		= 0;
	Decl[0].Offset		= 0;
	Decl[0].Type		= D3DDECLTYPE_D3DCOLOR;
	Decl[0].Method		= D3DDECLMETHOD_DEFAULT;
	Decl[0].Usage		= D3DDECLUSAGE_POSITION;
	Decl[0].UsageIndex	= 0;
	
	// D3DDECL_END()
	Decl[1].Stream		= 0xFF;
	Decl[1].Offset		= 0;
	Decl[1].Type		= D3DDECLTYPE_UNUSED;
	Decl[1].Method		= 0;
	Decl[1].Usage		= 0;
	Decl[1].UsageIndex	= 0;

	SAFE_RELEASE( m_pBaryDecl );
	hRes = m_pd3dDevice->CreateVertexDeclaration( Decl, &m_pBaryDecl );
	if( FAILED(hRes) ) return hRes;

	// 상수 정점을 설정합니다.
	m_baryFX->SetVectorArray( "VertexPos", (D3DXVECTOR4*)m_baryPos, m_numIndices );

	return S_OK;
}

HRESULT CMyD3DApplication::GenerateNPatchConstants()
{
	HRESULT hRes;
	// 먼저 정점 데이터의 색인을 해제합니다.
	m_npatchPos = new float[ m_numIndices * 4];
	m_npatchNorm = new float[ m_numIndices * 4];

	for(unsigned int i=0; i < m_numIndices;i++)
	{
		m_npatchPos[ (i*4) + 0 ] = m_posVertexData[ (m_indexData[i]*3) + 0 ];
		m_npatchPos[ (i*4) + 1 ] = m_posVertexData[ (m_indexData[i]*3) + 1 ];
		m_npatchPos[ (i*4) + 2 ] = m_posVertexData[ (m_indexData[i]*3) + 2 ];
		m_npatchPos[ (i*4) + 3 ] = 0;

		m_npatchNorm[ (i*4) + 0 ] = m_normVertexData[ (m_indexData[i]*3) + 0 ];
		m_npatchNorm[ (i*4) + 1 ] = m_normVertexData[ (m_indexData[i]*3) + 1 ];
		m_npatchNorm[ (i*4) + 2 ] = m_normVertexData[ (m_indexData[i]*3) + 2 ];
		m_npatchNorm[ (i*4) + 3 ] = 0;
	}

	hRes = GenerateNPatchBuffers( m_lod );
	if( FAILED(hRes) ) return hRes;

	D3DVERTEXELEMENT9	Decl[MAX_FVF_DECL_SIZE];
	// 선언 D3D 생성
	Decl[0].Stream		= 0;
	Decl[0].Offset		= 0;
	if( bFloatNPatch == false )
		Decl[0].Type	= D3DDECLTYPE_D3DCOLOR;
	else
		Decl[0].Type	= D3DDECLTYPE_FLOAT3;
	Decl[0].Method		= D3DDECLMETHOD_DEFAULT;
	Decl[0].Usage		= D3DDECLUSAGE_POSITION;
	Decl[0].UsageIndex	= 0;
	
	// D3DDECL_END()
	Decl[1].Stream		= 0xFF;
	Decl[1].Offset		= 0;
	Decl[1].Type		= D3DDECLTYPE_UNUSED;
	Decl[1].Method		= 0;
	Decl[1].Usage		= 0;
	Decl[1].UsageIndex	= 0;

	SAFE_RELEASE( m_pNPatchDecl );
	hRes = m_pd3dDevice->CreateVertexDeclaration( Decl, &m_pNPatchDecl );
	if( FAILED(hRes) ) return hRes;

	// 상수 정점을 설정합니다.
	m_npatchFX->SetVectorArray( "VertexPos", (D3DXVECTOR4*)m_npatchPos, m_numIndices );
	m_npatchFX->SetVectorArray( "VertexNorm", (D3DXVECTOR4*)m_npatchNorm, m_numIndices );

	return S_OK;
}

HRESULT CMyD3DApplication::GenerateLinearConstants()
{
	HRESULT hRes;

	// 먼저 정점 데이터의 색인을 해제합니다.
	m_linearPos = new float[ m_numVertices * 4];
	m_linearNorm = new float[ m_numVertices * 4];

	for(unsigned int i=0; i < m_numVertices;i++)
	{
		m_linearPos[ (i*4) + 0 ] = m_posVertexData[ (i*3) + 0 ];
		m_linearPos[ (i*4) + 1 ] = m_posVertexData[ (i*3) + 1 ];
		m_linearPos[ (i*4) + 2 ] = m_posVertexData[ (i*3) + 2 ];
		m_linearPos[ (i*4) + 3 ] = 0;

		m_linearNorm[ (i*4) + 0 ] = m_normVertexData[ (i*3) + 0 ];
		m_linearNorm[ (i*4) + 1 ] = m_normVertexData[ (i*3) + 1 ];
		m_linearNorm[ (i*4) + 2 ] = m_normVertexData[ (i*3) + 2 ];
		m_linearNorm[ (i*4) + 3 ] = 0;
	}

	// 정점 데이터를 직접 사용합니다.
	hRes = GenerateLinearBuffers( m_lod );
	if( FAILED(hRes) ) return hRes;


	D3DVERTEXELEMENT9	Decl[MAX_FVF_DECL_SIZE];


	// 선언 D3D 생성 
	// 무게 중심 좌표
	Decl[0].Stream		= 0;
	Decl[0].Offset		= 0;
	if( gBarycentricPrecision == BYTE_BARYCENTRIC )
		Decl[0].Type		= D3DDECLTYPE_D3DCOLOR;
	else if(gBarycentricPrecision == WORD_BARYCENTRIC)
		Decl[0].Type		= D3DDECLTYPE_USHORT4N;
	else if(gBarycentricPrecision == FLOAT_BARYCENTRIC)
		Decl[0].Type		= D3DDECLTYPE_FLOAT2;

	Decl[0].Method		= D3DDECLMETHOD_DEFAULT;
	Decl[0].Usage		= D3DDECLUSAGE_POSITION;
	Decl[0].UsageIndex	= 0;

	// 인덱스
	Decl[1].Stream		= 0;
	if(gBarycentricPrecision == FLOAT_BARYCENTRIC)
		Decl[1].Offset		= sizeof( float ) * 2;
	else
		Decl[1].Offset		= sizeof( unsigned int );
	Decl[1].Type		= D3DDECLTYPE_D3DCOLOR;
	Decl[1].Method		= D3DDECLMETHOD_DEFAULT;
	Decl[1].Usage		= D3DDECLUSAGE_POSITION;
	Decl[1].UsageIndex	= 1;

	// D3DDECL_END()
	Decl[2].Stream		= 0xFF;
	Decl[2].Offset		= 0;
	Decl[2].Type		= D3DDECLTYPE_UNUSED;
	Decl[2].Method		= 0;
	Decl[2].Usage		= 0;
	Decl[2].UsageIndex	= 0;

	SAFE_RELEASE( m_pLinearDecl );
	hRes = m_pd3dDevice->CreateVertexDeclaration( Decl, &m_pLinearDecl );
	if( FAILED(hRes) ) 
		return hRes;

	// 상수 정점을 설정합니다.
	m_linearFX->SetVectorArray( "VertexPos", (D3DXVECTOR4*)m_linearPos, m_numVertices );
	m_linearFX->SetVectorArray( "VertexNorm", (D3DXVECTOR4*)m_linearNorm, m_numVertices );

	return S_OK;
}

static unsigned int calcPatchVertsPerOrigTri( unsigned int n )
{
	return unsigned int(pow(4, n)) * 3;
}

//무게중심좌표
struct BARYCENTRIC_COORDS
{
	float i, j; // k는 쉽게 계산할 수 있습니다.

	BARYCENTRIC_COORDS() : i(0), j(0) {};
	BARYCENTRIC_COORDS( float x, float y ) : i(x), j(y) {};
	BARYCENTRIC_COORDS( const BARYCENTRIC_COORDS& right) : i(right.i), j(right.j) {};
	float getI() { return i; };
	float getJ() { return j; };
	float getK() { return 1.f - i - j; };

	BYTE getByteQuantisedI()
	{ 
		// 이 코드는 극단적인 정밀도 부족으로 인한 구멍을 줄이는 데 도움이 됩니다.
		unsigned int i = getI() * 256.f;
		if( i >= 256 )
			i = 255;
		return BYTE( i ); 
	};
	BYTE getByteQuantisedJ()
	{ 
		// 이 코드는 극단적인 정밀도 부족으로 인한 구멍을 줄이는 데 도움이 됩니다.
		unsigned int j = getJ() * 256.f;
		if( j >= 256 )
			j = 255;
		return BYTE( j ); 
	};
	BYTE getByteQuantisedK()
	{
		// 이 코드는 극단적인 정밀도 부족으로 인한 구멍을 줄이는 데 도움이 됩니다.
		unsigned int k = getK() * 256.f;
		if( k >= 256 )
			k = 255;
		return BYTE( k ); 
	};

	WORD getWordQuantisedI()
	{ 
		unsigned int i = getI() * 65535.f;
		return WORD( i ); 
	};
	WORD getWordQuantisedJ()
	{ 
		unsigned int j = getJ() * 65535.f;
		return WORD( j ); 
	};
	WORD getWordQuantisedK()
	{ 
		unsigned int k = getK() * 65535.f;
		return WORD( k ); 
	};

	BARYCENTRIC_COORDS& operator+=( const BARYCENTRIC_COORDS& right )
	{
		i = i + right.i;
		j = j + right.j;
		return *this;
	}
	BARYCENTRIC_COORDS operator+( const BARYCENTRIC_COORDS& right ) const
	{
		BARYCENTRIC_COORDS ret(*this);
		ret += right;
		return ret;
	}

	BARYCENTRIC_COORDS& operator/( float right )
	{
		i = i / right;
		j = j / right;
		return *this;
	}

};

struct BARYCENTRIC_TRIANGLE
{
	BARYCENTRIC_COORDS a, b, c;
	unsigned int lod;

	// 기본 표준 삼각형
	BARYCENTRIC_TRIANGLE() : a(1,0), b(0,1), c(0,0), lod(0) {};

	BARYCENTRIC_TRIANGLE( const BARYCENTRIC_TRIANGLE& right )
	{
		a = right.a;
		b = right.b;
		c = right.c;
		lod = right.lod;
	}
};

static void PatchTesselate( BARYCENTRIC_TRIANGLE tri,BARYCENTRIC_TRIANGLE& a,BARYCENTRIC_TRIANGLE& b,BARYCENTRIC_TRIANGLE& c,BARYCENTRIC_TRIANGLE& d )
{
	BARYCENTRIC_COORDS ab = (tri.a + tri.b) / 2;
	BARYCENTRIC_COORDS bc = (tri.b + tri.c) / 2;
	BARYCENTRIC_COORDS ac = (tri.a + tri.c) / 2;

	a.a = tri.a;
	a.b = ab;
	a.c = ac;
	a.lod = tri.lod+1;

	b.a = bc;
	b.b = tri.c;
	b.c = ac;
	b.lod = tri.lod+1;

	c.a = bc;
	c.b = ab;
	c.c = tri.b;
	c.lod = tri.lod+1;

	d.a = bc;
	d.b = ac;
	d.c = ab;
	d.lod = tri.lod+1;
}

HRESULT CMyD3DApplication::GenerateD3DNPatchMesh( unsigned int LOD )
{
	HRESULT hr;
	LOD = LOD + 1; // D3D와 내 계산의 차이

	if( LOD == m_lod && m_pD3DXPatchMeshDest != 0 )
		return S_OK;

	m_lod = LOD;
	SAFE_RELEASE( m_pD3DXPatchMeshDest );

	// 테셀레이트된 메쉬를 유지하는 데 필요한 메쉬의 크기를 결정합니다.
	DWORD NumTris, NumVerts;
	if( FAILED( hr = m_pD3DXPatchMesh->GetTessSize( (float)LOD, FALSE, &NumTris, &NumVerts ) ) )
        return DXTRACE_ERR( "m_pD3DXPatchMesh->Optimize", hr );

	if( FAILED( hr = D3DXCreateMeshFVF( NumTris, NumVerts, 0, m_pD3DXMesh->GetFVF(), m_pd3dDevice, &m_pD3DXPatchMeshDest ) ) )
        return DXTRACE_ERR( "m_pD3DXPatchMesh->Optimize", hr );

	if( FAILED( hr = m_pD3DXPatchMesh->Tessellate( (float)LOD, m_pD3DXPatchMeshDest ) ) )
        return DXTRACE_ERR( "m_pD3DXPatchMesh->Tessellate", hr );

	return S_OK;
}

//이 코드는 Direct3D를 사용하여 레벨 오브 디테일(LOD)에 따라 패치를 생성하고 이를 정점 버퍼(Vertex Buffer)에 저장하는 두 가지 함수입니다.
//하나는 일반적인 정점 버퍼를 생성하고, 다른 하나는 선형 버퍼를 생성합니다. 
//두 함수 모두 특정 레벨의 디테일(LOD)에 따라 삼각형 패치를 분할(tessellate)하고 그 결과를 정점 버퍼에 저장합니다.
//바리센트릭 좌표를 사용하여 각 정점을 표현합니다.
//NPatchBuffer 생성 
//정점 버퍼를 생성하고, 이를 지정된 LOD에 따라 tessellation된 패치 데이터로 채웁니다.
HRESULT CMyD3DApplication::GenerateNPatchBuffers( unsigned int LOD )
{
	HRESULT hRes;

	m_lod = LOD;

	SAFE_RELEASE( m_pVB );

	const unsigned int numTri = m_numIndices / 3;
	const unsigned int numVerts = calcPatchVertsPerOrigTri( LOD ) * numTri;
	unsigned int vbSize;
	
	if( bFloatNPatch == false )
	{
		vbSize = sizeof(unsigned int) * numVerts;
	} else
	{
		vbSize = sizeof(float) * 3 * numVerts;
	}

	hRes = m_pd3dDevice->CreateVertexBuffer( vbSize, 0, 0,D3DPOOL_MANAGED, &m_pVB, NULL );
	if( FAILED(hRes) ) return hRes;

	unsigned int *outStream;

	hRes = m_pVB->Lock(0, vbSize, (void**)&outStream, 0 );
	if( FAILED(hRes) ) return hRes;

	for(unsigned int index=0;index < m_numIndices/3;index++)
	{
		std::stack<BARYCENTRIC_TRIANGLE> lodStack;
		lodStack.push( BARYCENTRIC_TRIANGLE() );

		while( !lodStack.empty() )
		{
			BARYCENTRIC_TRIANGLE tri = lodStack.top();
			lodStack.pop();

			if( tri.lod < LOD)
			{
				BARYCENTRIC_TRIANGLE tri_a, tri_b, tri_c, tri_d;

				PatchTesselate( tri, tri_a, tri_b, tri_c, tri_d );
				lodStack.push( tri_a );
				lodStack.push( tri_b );
				lodStack.push( tri_c );
				lodStack.push( tri_d );
			} else
			{
				if( tri.lod == LOD )
				{
					if( bFloatNPatch == false )
					{
						//출력 좌표
						*(outStream+0) = ((unsigned int)(tri.a.i*255)<<16) | ((unsigned int)(tri.a.j*255)<<8) | (index<<0); // correct for D3DCOLOR unpacking
						*(outStream+1) = ((unsigned int)(tri.b.i*255)<<16) | ((unsigned int)(tri.b.j*255)<<8) | (index<<0); // correct for D3DCOLOR unpacking
						*(outStream+2) = ((unsigned int)(tri.c.i*255)<<16) | ((unsigned int)(tri.c.j*255)<<8) | (index<<0); // correct for D3DCOLOR unpacking
						outStream += 3;
					} else
					{
						*((float*)outStream+0) = tri.a.i;
						*((float*)outStream+1) = tri.a.j;
						*((float*)outStream+2) = index / 255.f;
						*((float*)outStream+3) = tri.b.i;
						*((float*)outStream+4) = tri.b.j;
						*((float*)outStream+5) = index / 255.f;
						*((float*)outStream+6) = tri.c.i;
						*((float*)outStream+7) = tri.c.j;
						*((float*)outStream+8) = index / 255.f;
						outStream += 9;
					}
				}
			}
		}
	}

	// 이제 정점 버퍼를 잠금 해제하세요.
	hRes = m_pVB->Unlock();
	if( FAILED(hRes) ) return hRes;

	return S_OK;
}

//선형 버퍼 생성
//선형 버퍼를 생성하고, 이를 지정된 LOD에 따라 tessellation된 패치 데이터로 채웁니다.
HRESULT CMyD3DApplication::GenerateLinearBuffers( unsigned int LOD )
{
	HRESULT hRes;

	m_lod = LOD;

	SAFE_RELEASE( m_pVB );

	const unsigned int numTri = m_numIndices / 3;
	const unsigned int numVerts = calcPatchVertsPerOrigTri( LOD ) * numTri;
	unsigned int vbSize;
	
	// 무게 중심은 모든 표면에서 재사용 가능
	// 분할 금액이 비슷하므로 2개의 스트림으로 분할하면 많은 비용을 절약할 수 있습니다.
	// 램
	if( gBarycentricPrecision == FLOAT_BARYCENTRIC )
		vbSize = (sizeof(float)*2 + sizeof(unsigned int)) * numVerts;
	else
		vbSize = (sizeof(unsigned int) * 2) * numVerts;
	
	hRes = m_pd3dDevice->CreateVertexBuffer( vbSize, 0, 0,D3DPOOL_MANAGED, &m_pVB, NULL );
	if( FAILED(hRes) ) 
		return hRes;

	unsigned int *outStream;

	hRes = m_pVB->Lock(0, vbSize, (void**)&outStream, 0 );
	if( FAILED(hRes) ) 
		return hRes;

	for(unsigned int index=0;index < m_numIndices/3;index++)
	{
		std::stack<BARYCENTRIC_TRIANGLE> lodStack;
		lodStack.push( BARYCENTRIC_TRIANGLE() );

		while( !lodStack.empty() )
		{
			BARYCENTRIC_TRIANGLE tri = lodStack.top();
			lodStack.pop();

			if( tri.lod < LOD)
			{
				BARYCENTRIC_TRIANGLE tri_a, tri_b, tri_c, tri_d;

				PatchTesselate( tri, tri_a, tri_b, tri_c, tri_d );
				lodStack.push( tri_a );
				lodStack.push( tri_b );
				lodStack.push( tri_c );
				lodStack.push( tri_d );
			} 
			else
			{
				if( tri.lod == LOD )
				{
					unsigned int uii, uij;
					// 바이트 인덱스가 문제가 되지 않도록 큰 메시를 분할해야 한다는 점을 기억하세요(현재)
					BYTE i0 = (BYTE) m_indexData[ (index*3) + 0 ];
					BYTE i1 = (BYTE) m_indexData[ (index*3) + 1 ];
					BYTE i2 = (BYTE) m_indexData[ (index*3) + 2 ];

					if( gBarycentricPrecision == FLOAT_BARYCENTRIC )
					{
						*((float*)outStream+0) = tri.a.getI();
						*((float*)outStream+1) = tri.a.getJ();
						*(outStream+2) = (i0<<16) | (i1 << 8) | (i2 << 0);
						*((float*)outStream+3) = tri.b.getI();
						*((float*)outStream+4) = tri.b.getJ();
						*(outStream+5) = (i0<<16) | (i1 << 8) | (i2 << 0);
						*((float*)outStream+6) = tri.c.getI();
						*((float*)outStream+7) = tri.c.getJ();
						*(outStream+8) = (i0<<16) | (i1 << 8) | (i2 << 0);

						outStream += 9;
					} else
					{
						// 출력 좌표
						if( gBarycentricPrecision == BYTE_BARYCENTRIC )
						{
							uii = tri.a.getByteQuantisedI();
							uij = tri.a.getByteQuantisedJ();
							*(outStream+0) = (uii<<16) | (uij<<8); // D3DCOLOR 압축 풀기에 적합
							uii = tri.b.getByteQuantisedI();
							uij = tri.b.getByteQuantisedJ();
							*(outStream+2) = (uii<<16) | (uij<<8); // D3DCOLOR 압축 풀기에 적합
							uii = tri.c.getByteQuantisedI();
							uij = tri.c.getByteQuantisedJ();
							*(outStream+4) = (uii<<16) | (uij<<8); /// D3DCOLOR 압축 풀기에 적합
						} else if( gBarycentricPrecision == WORD_BARYCENTRIC )
						{
							uii = tri.a.getWordQuantisedI();
							uij = tri.a.getWordQuantisedJ();
							*(outStream+0) = (uii<<0) | (uij<<16); 
							uii = tri.b.getWordQuantisedI();
							uij = tri.b.getWordQuantisedJ();
							*(outStream+2) = (uii<<0) | (uij<<16); 
							uii = tri.c.getWordQuantisedI();
							uij = tri.c.getWordQuantisedJ();
							*(outStream+4) = (uii<<0) | (uij<<16); 
						}

						*(outStream+1) = (i0<<16) | (i1 << 8) | (i2 << 0);
						*(outStream+3) = (i0<<16) | (i1 << 8) | (i2 << 0);
						*(outStream+5) = (i0<<16) | (i1 << 8) | (i2 << 0);
						outStream += 6;
					}
				}
			}
		}
	}

	// 이제 정점 버퍼의 잠금을 해제합니다.
	hRes = m_pVB->Unlock();
	if( FAILED(hRes) )
		return hRes;

	return S_OK;
}


// InvalidateDeviceObjects()와 페어링됨 장치가 존재하지만 방금 Reset()되었을 수 있습니다.  
// 리소스 D3DPOOL_DEFAULT 및 실행 중에 지속되는 기타 장치 상태 렌더링은 여기에서 설정되어야 합니다.  
// 렌더링 상태, 행렬, 텍스처, 등 렌더링 중에 변경되지 않는 사항은 여기에서 한 번만 설정하면 됩니다.
// Render() 또는 FrameMove() 중 중복된 상태 설정을 방지합니다.
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
	// TODO: 렌더 상태 설정
	
	// 머티리얼 설정
    D3DMATERIAL9 mtrl;
    D3DUtil_InitMaterial( mtrl, 1.0f, 0.0f, 0.0f );
    m_pd3dDevice->SetMaterial( &mtrl );

	// 텍스처 설정
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

	// 기타 렌더링 상태 설정
    m_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE,   FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,        TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_AMBIENT,        0x000F0F0F );

	// 월드 매트릭스 설정
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity( &matIdentity );
    m_pd3dDevice->SetTransform( D3DTS_WORLD,  &matIdentity );
	m_matWorld = matIdentity;


	// 뷰 매트릭스를 설정합니다. 뷰 매트릭스는 시점을 기준으로 정의할 수 있습니다.
	// 볼 지점과 위쪽 방향입니다. 
	// 여기서는 z축을 따라 뒤로 5단위, 위쪽으로 3단위를 살펴봅니다.
	// 원점을 지정하고 "위쪽"이 y 방향이 되도록 정의합니다.
    D3DXMATRIX matView;
    D3DXVECTOR3 vFromPt   = D3DXVECTOR3( 0.0f, 0.0f, -3.0f );
    D3DXVECTOR3 vLookatPt = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec    = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &matView, &vFromPt, &vLookatPt, &vUpVec );
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );
	m_matView = matView;

	// 투영 행렬 설정
    D3DXMATRIX matProj;
    FLOAT fAspect = ((FLOAT)m_d3dsdBackBuffer.Width) / m_d3dsdBackBuffer.Height;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, fAspect, 1.0f, 100.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
	m_matProj = matProj;

	// 조명 상태 설정
    D3DLIGHT9 light;
    D3DUtil_InitLight( light, D3DLIGHT_DIRECTIONAL, -1.0f, -1.0f, 2.0f );
    m_pd3dDevice->SetLight( 0, &light );
    m_pd3dDevice->LightEnable( 0, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );

	// 글꼴 복원
    m_pFont->RestoreDeviceObjects();

    return S_OK;
}

//프레임당 한 번씩 호출되며 이 호출은 애니메이션의 진입점입니다.
//장면.
HRESULT CMyD3DApplication::FrameMove()
{
	// 사용자 입력 상태 업데이트
    UpdateInput( &m_UserInput );

	// 사용자 입력에 따라 세계 상태를 업데이트합니다.
    D3DXMATRIX matWorld;
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

    D3DXMatrixMultiply( &matWorld, &matRotX, &matRotY );
    m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );
	m_matWorld = matWorld;


	// LOD 선택(사용 가능한 경우)
	if( m_UserInput.b1 )
	{
		if( m_displayMethod == DM_NPATCH )
			GenerateNPatchBuffers( 0 );
		else if( m_displayMethod == DM_CONTROL_NPATCH )
			GenerateD3DNPatchMesh( 1 );
		else if( m_displayMethod == DM_LINEAR )
			GenerateLinearBuffers( 0 );
	}
	else if( m_UserInput.b2 )
	{
		if( m_displayMethod == DM_NPATCH )
			GenerateNPatchBuffers( 1 );
		else if( m_displayMethod == DM_CONTROL_NPATCH )
			GenerateD3DNPatchMesh( 2 );
		else if( m_displayMethod == DM_LINEAR )
			GenerateLinearBuffers( 1 );
	} 
	else if( m_UserInput.b3 )
	{
		if( m_displayMethod == DM_NPATCH )
			GenerateNPatchBuffers( 2 );
		else if( m_displayMethod == DM_CONTROL_NPATCH )
			GenerateD3DNPatchMesh( 3 );
		else if( m_displayMethod == DM_LINEAR )
			GenerateLinearBuffers( 2 );
	} 
	else if( m_UserInput.b4 )
	{
		if( m_displayMethod == DM_NPATCH )
			GenerateNPatchBuffers( 3 );
		else if( m_displayMethod == DM_CONTROL_NPATCH )
			GenerateD3DNPatchMesh( 4 );
		else if( m_displayMethod == DM_LINEAR )
			GenerateLinearBuffers( 3 );
		
	} 
	else if( m_UserInput.b5 )
	{
		if( m_displayMethod == DM_NPATCH )
			GenerateNPatchBuffers( 4 );
		else if( m_displayMethod == DM_CONTROL_NPATCH )
			GenerateD3DNPatchMesh( 5 );
		else if( m_displayMethod == DM_LINEAR )
			GenerateLinearBuffers( 4 );
		
	}
	else if( m_UserInput.b6 )
	{
		if( m_displayMethod == DM_NPATCH )
			GenerateNPatchBuffers( 5 );
		else if( m_displayMethod == DM_CONTROL_NPATCH )
			GenerateD3DNPatchMesh( 6 );
		else if( m_displayMethod == DM_LINEAR )
			GenerateLinearBuffers( 5 );
	}
	
	// 메소드 선택
	if( m_UserInput.bf5 && m_displayMethod != DM_CONTROL)
		m_displayMethod = DM_CONTROL;
	else if( m_UserInput.bf6 && m_displayMethod != DM_CONTROL_NPATCH)
	{
		m_displayMethod = DM_CONTROL_NPATCH;
		GenerateD3DNPatchMesh( m_lod - 1  );
	} 
	else if( m_UserInput.bf7 && m_displayMethod != DM_BARYCENTRIC)
		m_displayMethod = DM_BARYCENTRIC;

	else if( m_UserInput.bf8 && m_displayMethod != DM_NPATCH)
	{
		m_displayMethod = DM_NPATCH;
		GenerateNPatchBuffers( m_lod );
	} 
	else if( m_UserInput.bf9 && m_displayMethod != DM_LINEAR)
	{
		m_displayMethod = DM_LINEAR;
		GenerateLinearBuffers( m_lod );
	}
	return SetupDisplayMethod();
}

//사용자 입력을 업데이트합니다.  프레임당 한 번씩 호출됩니다.
void CMyD3DApplication::UpdateInput( UserInput* pUserInput )
{
    HRESULT hr;

	// 입력의 장치 상태를 가져오고 상태를 흐리게 표시합니다.
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

// 프레임당 한 번 호출되며 이 호출은 3D의 진입점입니다.
// 렌더링. 
// 이 기능은 렌더링 상태를 설정하고 뷰포트를 실행하고 장면을 렌더링합니다.
HRESULT CMyD3DApplication::Render()
{
	// 뷰포트 지우기
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x50505050, 1.0f, 0L );
	//m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
	
    // 장면 시작
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
		// MVP 계산
		D3DXMATRIX matMVP;
        D3DXMatrixMultiply( &matMVP, &m_matView, &m_matProj );
        D3DXMatrixMultiply( &matMVP, &m_matWorld, &matMVP );

		// 공유 효과 풀을 사용해야 하지만 이는 단지 데모일 뿐입니다...
		m_controlFX->SetMatrix( "MVP", &matMVP );
		m_baryFX->SetMatrix( "MVP", &matMVP );
		m_npatchFX->SetMatrix( "MVP", &matMVP );
		m_linearFX->SetMatrix( "MVP", &matMVP );

		unsigned int numPasses;

		switch( m_displayMethod )
		{
		case DM_CONTROL:
			m_controlFX->Begin( &numPasses, 0 );
			for(unsigned int i=0; i < numPasses;i++)
			{
				m_controlFX->BeginPass( i );
				m_pD3DXMesh->DrawSubset( 0 );
			}
			m_controlFX->End();
			break;
		case DM_CONTROL_NPATCH:
			m_controlFX->Begin( &numPasses, 0 );
			for(unsigned int i=0; i < numPasses;i++)
			{
				m_controlFX->BeginPass( i );
				m_pD3DXPatchMeshDest->DrawSubset( 0 );
			}
			m_controlFX->End();
			break;
		case DM_BARYCENTRIC:
			m_baryFX->Begin( &numPasses, 0 );
			for(unsigned int i=0; i < numPasses;i++)
			{
				m_pd3dDevice->SetStreamSource( 0, m_pVB, 0, sizeof(unsigned int) );
				m_pd3dDevice->SetVertexDeclaration( m_pBaryDecl );
				m_baryFX->BeginPass( i );
				m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, m_numIndices /3 );
			}
			m_baryFX->End();
			break;
		case DM_NPATCH:
			m_npatchFX->Begin( &numPasses, 0 );
			for(unsigned int i=0; i < numPasses;i++)
			{
				m_pd3dDevice->SetVertexDeclaration( m_pNPatchDecl );
				m_npatchFX->BeginPass( i );
				const unsigned int numTri = m_numIndices / 3;
				unsigned int numVerts = calcPatchVertsPerOrigTri( m_lod ) * numTri;

				unsigned int vSize;
				if( bFloatNPatch == false )
					vSize = sizeof(unsigned int);
				else
					vSize = sizeof(float) * 3;

				// 일부 카드는 한 번에 많은 정점을 처리하지 못합니다.
				if( numVerts > 65536 )
					numVerts = 65536;

				m_pd3dDevice->SetStreamSource(	0, m_pVB, 0, vSize );
				m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, numVerts /3 );
			}
			m_npatchFX->End();
			break;
		case DM_LINEAR:
			m_linearFX->Begin( &numPasses, 0 );
			for(unsigned int i=0; i < numPasses;i++)
			{
				m_pd3dDevice->SetVertexDeclaration( m_pLinearDecl );
				m_linearFX->BeginPass( i );
				const unsigned int numTri = m_numIndices / 3;
				unsigned int numVerts = calcPatchVertsPerOrigTri( m_lod ) * numTri;

				unsigned int vSize;
				if( gBarycentricPrecision == FLOAT_BARYCENTRIC )
					vSize = sizeof(float)*2 + sizeof(unsigned int);
				else
					vSize = sizeof(unsigned int) * 2;

				// 일부 카드는 한 번에 많은 정점을 처리하지 못합니다.
				if( numVerts > 65536 )
					numVerts = 65536;

				m_pd3dDevice->SetStreamSource(	0, m_pVB, 0, vSize );
				m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, numVerts /3 );
			}
			m_linearFX->End();
			break;
		}

		// 렌더링 통계 및 도움말 텍스트
        RenderText();

		// 장면을 종료합니다.
        m_pd3dDevice->EndScene();
    }
    return S_OK;
}

// 통계와 도움말 텍스트를 장면에 렌더링합니다.
HRESULT CMyD3DApplication::RenderText()
{
    D3DCOLOR fontColor        = D3DCOLOR_ARGB(255,255,255,0);
    TCHAR szMsg[MAX_PATH] = TEXT("");

	// 디스플레이 통계 출력
    FLOAT fNextLine = 40.0f; 

    lstrcpy( szMsg, m_strDeviceStats );
    fNextLine -= 20.0f;
    m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

    lstrcpy( szMsg, m_strFrameStats );
    fNextLine -= 20.0f;
    m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

	fNextLine = 40.0f;
	// 통계 및 도움말 출력
	if( m_displayMethod == DM_BARYCENTRIC )
	{
		m_pFont->DrawText( 2, fNextLine, fontColor, "Vertex shader barycentric vertices" );
		fNextLine += 20.0f;
	} 
	else if( m_displayMethod == DM_NPATCH )
	{
		m_pFont->DrawText( 2, fNextLine, fontColor, "Vertex shader NPatch basis" );
		fNextLine += 20.0f;
	    sprintf( szMsg, "load level %d", m_lod );
		m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
	} 
	else if( m_displayMethod == DM_LINEAR )
	{
		m_pFont->DrawText( 2, fNextLine, fontColor, "Vertex shader linear basis" );
		fNextLine += 20.0f;
	    sprintf( szMsg, "load level %d", m_lod );
		m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
	} 
	else if( m_displayMethod == DM_CONTROL )
	{
		m_pFont->DrawText( 2, fNextLine, fontColor, "Standard D3DX mesh rendering" );
		fNextLine += 20.0f;
	    sprintf( szMsg, "load level %d", m_lod );
		m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
	} 
	else if( m_displayMethod == DM_CONTROL_NPATCH )
	{
		m_pFont->DrawText( 2, fNextLine, fontColor, "D3DX Software NPatch tesselation" );
		fNextLine += 20.0f;
	}

	fNextLine = (FLOAT) m_d3dsdBackBuffer.Height; 
    wsprintf( szMsg, TEXT("Arrow keys: Up=%d Down=%d Left=%d Right=%d"), m_UserInput.bRotateUp, m_UserInput.bRotateDown, m_UserInput.bRotateLeft, m_UserInput.bRotateRight );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    lstrcpy( szMsg, TEXT("Use arrow keys to rotate object") );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    lstrcpy( szMsg, TEXT("Press 'F2' to configure display") );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    return S_OK;
}

// 샘플에서 사용자 지정 메시지를 수행할 수 있도록 기본 WndProc를 재정의합니다.
// 처리(예: 마우스, 키보드 또는 메뉴 명령 처리).
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_PAINT:
        {
            if( m_bLoadingApp )
            {
				// 창에 그림을 그려 사용자에게 앱이 로드 중임을 알립니다.
				// TODO: 필요에 따라 변경
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

//장치 개체를 무효화합니다. 
//RestoreDeviceObjects()와 페어링됨
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
	// TODO: RestoreDeviceObjects()에서 생성된 모든 개체를 정리합니다.
    m_pFont->InvalidateDeviceObjects();
    return S_OK;
}

// InitDeviceObjects()와 페어링됨
// 앱이 종료되거나 기기가 변경될 때 호출됩니다.
// 이 함수는 모든 장치 종속 객체를 삭제합니다.
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
	CleanDisplayMethod();

	delete m_posVertexData; m_posVertexData = 0;
	delete m_normVertexData; m_normVertexData = 0;
	delete m_indexData; m_indexData = 0;
	m_numVertices = 0;
	m_numIndices = 0;

    m_pFont->DeleteDeviceObjects();

	SAFE_RELEASE( m_linearFX ) ;
	SAFE_RELEASE( m_controlFX ) ;
	SAFE_RELEASE( m_npatchFX ) ;
	SAFE_RELEASE( m_baryFX ) ;

	SAFE_RELEASE( m_pD3DXPatchMeshDest ); 
	SAFE_RELEASE( m_pD3DXPatchMesh );
	SAFE_RELEASE( m_pD3DXMesh );
    return S_OK;
}


void CMyD3DApplication::CleanDisplayMethod()
{
	SAFE_RELEASE( m_pVB );

	SAFE_RELEASE( m_pBaryDecl );
	SAFE_RELEASE( m_pNPatchDecl );
	SAFE_RELEASE( m_pLinearDecl );

	if (m_baryPos != nullptr)
	{
		delete m_baryPos; 
		m_baryPos = nullptr;
	}
	if (m_npatchPos != nullptr)
	{
		delete m_npatchPos; 
		m_npatchPos = nullptr;
	}
	if (m_npatchNorm != nullptr)
	{
		delete m_npatchNorm; 
		m_npatchNorm = nullptr;
	}
	if (m_linearPos != nullptr)
	{
		delete m_linearPos; 
		m_linearPos = nullptr;
	}
	if (m_linearNorm != nullptr)
	{
		delete m_linearNorm; 
		m_linearNorm = nullptr;
	}

}

HRESULT CMyD3DApplication::SetupDisplayMethod()
{
    HRESULT hr;

	if( m_displayMethod == DM_BARYCENTRIC )
	{
		if( m_pBaryDecl == 0 )
		{
			CleanDisplayMethod();
			if( FAILED( hr = GenerateBaryConstants() ) )
				return DXTRACE_ERR( "GenerateBaryConstants", hr );
		}
	} 
	else if( m_displayMethod == DM_NPATCH )
	{
		if( m_pNPatchDecl == 0 )
		{
			CleanDisplayMethod();
			if( FAILED( hr = GenerateNPatchConstants() ) )
				return DXTRACE_ERR( "GenerateNPatchConstants", hr );
		}
	} 
	else if( m_displayMethod == DM_LINEAR )
	{
		if( m_pLinearDecl == 0 )
		{
			CleanDisplayMethod();
			if( FAILED( hr = GenerateLinearConstants() ) )
				return DXTRACE_ERR( "GenerateLinearConstants", hr );
		}
	}
	return S_OK;
}

// OneTimeSceneInit()과 페어링됨
// 앱이 종료되기 전에 호출됩니다. 
// 이 함수는 앱에 기회를 제공합니다.
// 그 자체를 정리합니다.
HRESULT CMyD3DApplication::FinalCleanup()
{
	// TODO: 필요한 최종 정리를 수행합니다.
	// D3D 글꼴 정리
    SAFE_DELETE( m_pFont );

	// DirectInput 정리
    CleanupDirectInput();

	// 레지스트리에 설정을 씁니다.
    WriteSettings();

    return S_OK;
}

//DirectInput 정리
VOID CMyD3DApplication::CleanupDirectInput()
{
	// DirectX 입력 객체 정리
    SAFE_RELEASE( m_pKeyboard );
    SAFE_RELEASE( m_pDI );

}