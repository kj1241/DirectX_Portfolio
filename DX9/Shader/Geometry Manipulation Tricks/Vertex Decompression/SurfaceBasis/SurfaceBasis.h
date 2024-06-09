#pragma once

#define DXAPP_KEY        TEXT("Software\\DirectX9\\SurfaceBasis")

// 현재 입력 상태를 저장하는 구조체
struct UserInput
{
    BYTE diks[256];   // DirectInput 키보드 상태 버퍼

    BOOL bRotateUp;
    BOOL bRotateDown;
    BOOL bRotateLeft;
    BOOL bRotateRight;

	BOOL b1,b2,b3,b4,b5,b6,b7,b8,b9;
	BOOL bf5,bf6,bf7,bf8,bf9;
};

enum DISPLAY_METHOD
{
	DM_CONTROL,			// 구식 방식으로 메시 만들기
	DM_CONTROL_NPATCH,	// D3DX를 사용하여 CPU에서 N-패치를 수행합니다.
	DM_BARYCENTRIC, 	// '일반' 메시를 렌더링하기 위해 무게 중심 좌표를 사용했습니다.
	DM_NPATCH,			// 정점 셰이더 N-패치
	DM_LINEAR,
};

class CMyD3DApplication : public CD3DApplication
{
    BOOL                    m_bLoadingApp;			// TRUE, 앱이 로드 중인 경우
    CD3DFont*               m_pFont;				// 텍스트를 그리는 글꼴
    ID3DXMesh*              m_pD3DXMesh;			// 찻주전자를 저장하는 D3DX 메시
    ID3DXPatchMesh*         m_pD3DXPatchMesh;		// 찻주전자를 저장하기 위한 D3DX 패치 메시
    ID3DXMesh*				m_pD3DXPatchMeshDest;	// 찻주전자를 저장하기 위한 D3DX 패치 메시
    LPDIRECTINPUT8          m_pDI;					// DirectInput 객체
	LPDIRECTINPUTDEVICE8    m_pKeyboard;            // DirectInput 키보드 장치
    UserInput               m_UserInput;			// 사용자 입력을 저장하는 구조체

    FLOAT                   m_fWorldRotX;			// 월드 회전 상태 X축
    FLOAT                   m_fWorldRotY;           // 월드 회전 상태 Y축

	LPD3DXEFFECT			m_controlFX;			// 일반 변환 효과
	LPD3DXEFFECT			m_baryFX;				// 첫 번째 무게 중심 효과
	LPD3DXEFFECT			m_npatchFX;				// n패치 효과
	LPD3DXEFFECT			m_linearFX;				// n패치 효과

	unsigned int			m_numVertices;
	unsigned int			m_numIndices;
	float*					m_posVertexData;
	float*					m_normVertexData;
	WORD*					m_indexData;

	LPDIRECT3DVERTEXBUFFER9	m_pVB;					// 렌더링할 실제 VB
	LPDIRECT3DVERTEXBUFFER9	m_pIB;					// 렌더링할 실제 VB(사용된 경우)

	D3DXMATRIX				m_matProj;
	D3DXMATRIX				m_matView;
	D3DXMATRIX				m_matWorld;
	DISPLAY_METHOD			m_displayMethod;

	HRESULT SetupDisplayMethod();
	void CleanDisplayMethod();
	HRESULT GetXMeshData( LPD3DXMESH in );

	// 테셀레이터 관련 내용
	unsigned int			m_lod;

	// 첫 번째 Bary 표면 기본 변수
	LPDIRECT3DVERTEXDECLARATION9	m_pBaryDecl;
	float*							m_baryPos;
	HRESULT							GenerateBaryConstants();

	// npatch 표면 기반
	LPDIRECT3DVERTEXDECLARATION9	m_pNPatchDecl;
	float*							m_npatchPos;
	float*							m_npatchNorm;
	HRESULT							GenerateNPatchConstants();
	HRESULT							GenerateNPatchBuffers( unsigned int LOD );
	HRESULT							GenerateD3DNPatchMesh( unsigned int LOD );

	// 선형 표면 기반
	LPDIRECT3DVERTEXDECLARATION9	m_pLinearDecl;
	float*							m_linearPos;
	float*							m_linearNorm;
	HRESULT							GenerateLinearConstants();
	HRESULT							GenerateLinearBuffers( unsigned int LOD );

protected:
    virtual HRESULT OneTimeSceneInit();
    virtual HRESULT InitDeviceObjects();
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
    virtual HRESULT Render();
    virtual HRESULT FrameMove();
    virtual HRESULT FinalCleanup();
    virtual HRESULT ConfirmDevice( D3DCAPS9*, DWORD, D3DFORMAT );

    HRESULT RenderText();

    HRESULT InitInput( HWND hWnd );
    void    UpdateInput( UserInput* pUserInput );
    void    CleanupDirectInput();
    VOID    ReadSettings();
    VOID    WriteSettings();

public:
    LRESULT MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
    CMyD3DApplication();
    virtual ~CMyD3DApplication();
};

