#ifndef AFX_QUANTISATION_H__C429C6DE_1BFE_4EAD_8122_272A65AAA8C3__INCLUDED_
#define AFX_QUANTISATION_H__C429C6DE_1BFE_4EAD_8122_272A65AAA8C3__INCLUDED_
#include "CompressedMesh.h"

#define DXAPP_KEY        TEXT("Software\\DirectX9\\SurfaceBasis")

// 현재 입력 상태를 저장하는 구조체
struct UserInput
{
	BYTE diks[256];   // DirectInput 키보드 상태 버퍼 

	BOOL bRotateUp;
	BOOL bRotateDown;
	BOOL bRotateLeft;
	BOOL bRotateRight;
	BOOL bChange;

	BOOL b1, b2, b3, b4, b5, b6, b7, b8, b9;

	BOOL bf5, bf6, bf7, bf8, bf9;
};

//애플리케이션 클래스입니다. 기본 클래스인 CD3DApplication은 
//모든 Direct3D 샘플에서 필요한 일반 기능을 제공합니다. 
//CMyD3DApplication은 이 샘플 프로그램에 특정한 기능을 추가합니다.
class CMyD3DApplication : public CD3DApplication
{
	enum COMPRESS_STATE
	{
		DISPLAY_QUANT_NORMAL = 0,
		DISPLAY_SO8BIT_POS,
		DISPLAY_CT8BIT_POS,
		DISPLAY_SO16BIT_POS,
		DISPLAY_CT16BIT_POS,
		DISPLAY_SCT16BIT_POS,
		DISPLAY_CT10BIT_POS,
		DISPLAY_CT101012BIT_POS,
	};

	enum MESH_NAME
	{
		DISPLAY_TEAPOT = 0,
		DISPLAY_CUBE,
		DISPLAY_TEAPOT_ROW,
		DISPLAY_CUBE_ROW,

		MAX_MESHES
	};

    BOOL                    m_bLoadingApp;          // 앱을 로드하는 중인지 여부
    CD3DFont* m_pFont;                // 텍스트를 그리는 데 사용되는 폰트
    ID3DXMesh* m_pD3DXMesh[MAX_MESHES]; // Teapot을 저장하는 D3DX 메시
    CompressedMesh* m_pQNMesh[MAX_MESHES];    // 양자화된 정규화된 압축 메시
    CompressedMesh* m_pSO8Mesh[MAX_MESHES];    // 스케일 및 오프셋 8비트 압축 메시
    CompressedMesh* m_pCT8Mesh[MAX_MESHES];    // 압축 변환 8비트 압축 메시
    CompressedMesh* m_pSO16Mesh[MAX_MESHES];    // 스케일 및 오프셋 8비트 압축 메시
    CompressedMesh* m_pCT16Mesh[MAX_MESHES];    // 압축 변환 16비트 압축 메시
    CompressedMesh* m_pSCT16Mesh[MAX_MESHES];    // 슬라이딩 압축 변환 16비트 압축 메시
    CompressedMesh* m_pCT10Mesh[MAX_MESHES];    // 압축 변환 UDEC3N 비트 압축 메시
    CompressedMesh* m_pCT101012Mesh[MAX_MESHES];    // 압축 변환 10,10,12 비트 압축 메시

    IDirect3DVertexShader9* m_dwControlShader[MAX_MESHES];    // D3DX 메시 쉐이더
    IDirect3DVertexShader9* m_dwQNShader[MAX_MESHES];    // 양자화된 정규화된 압축 메시 쉐이더
    IDirect3DVertexShader9* m_dwSO8Shader[MAX_MESHES];    // 스케일 및 오프셋 8비트 압축 메시 쉐이더
    IDirect3DVertexShader9* m_dwCT8Shader[MAX_MESHES];    // 압축 변환 8비트 압축 메시 쉐이더
    IDirect3DVertexShader9* m_dwSO16Shader[MAX_MESHES];    // 스케일 및 오프셋 8비트 압축 메시 쉐이더
    IDirect3DVertexShader9* m_dwCT16Shader[MAX_MESHES];    // 압축 변환 16비트 압축 메시 쉐이더
    IDirect3DVertexShader9* m_dwSCT16Shader[MAX_MESHES]; // 슬라이딩 압축 변환 16비트 압축 메시 쉐이더
    IDirect3DVertexShader9* m_dwCT10Shader[MAX_MESHES]; //압축 변환 DEC3N 압축 메시 쉐이더  
    IDirect3DVertexShader9* m_dwCT101012Shader[MAX_MESHES]; //압축 변환 10,10,12 비트 압축 메시 쉐이더

    bool                    m_original;             // 어떤 메시가 가시화되어 있는지?
    D3DXMATRIX              m_matView;              // 뷰 행렬
    D3DXMATRIX              m_matProj;              // 투영 행렬
    D3DXMATRIX              m_matWorld;             // 월드 행렬

    LPDIRECTINPUT8          m_pDI;                  // DirectInput 객체
    LPDIRECTINPUTDEVICE8    m_pKeyboard;            // DirectInput 키보드 장치
    UserInput               m_UserInput;            // 사용자 입력을 저장하는 구조체 

    FLOAT                   m_fWorldRotX;           // 월드 회전 상태 X축
    FLOAT                   m_fWorldRotY;           // 월드 회전 상태 Y축
    CompressedMesh*         m_pCompressMesh;        // 현재 가시 압축 메시 (통계용)
    COMPRESS_STATE          m_compressState;        // 현재 가시화된 압축
	MESH_NAME				m_meshState;			// 현재 보이는 메쉬

protected:
    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT Render();
    HRESULT FrameMove();
    HRESULT FinalCleanup();
    HRESULT ConfirmDevice( D3DCAPS9*, DWORD, D3DFORMAT );

    HRESULT RenderText();

    HRESULT InitInput( HWND hWnd );
    void    UpdateInput( UserInput* pUserInput );
    void    CleanupDirectInput();

    VOID    ReadSettings();
    VOID    WriteSettings();

	HRESULT GenerateTeapotRow( ID3DXMesh** mesh );
	HRESULT GenerateCubeRow( ID3DXMesh** mesh );
	HRESULT CreateVertexShaders( unsigned int i);

public:
    LRESULT MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
    CMyD3DApplication();
};
#endif // !defined(AFX_QUANTISATION_H__C429C6DE_1BFE_4EAD_8122_272A65AAA8C3__INCLUDED_)