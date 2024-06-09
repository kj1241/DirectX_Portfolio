#pragma once

// TODO: "DirectX AppWizard Apps"를 당신의 이름이나 회사 이름으로 변경하세요
#define DXAPP_KEY        TEXT("Software\\DirectX9\\SurfaceBasis")

// 현재 입력 상태를 저장하는 구조체
struct UserInput
{
    BYTE diks[256];   // DirectInput 키보드 상태 버퍼 
    BOOL bRotateUp;
    BOOL bRotateDown;
    BOOL bRotateLeft;
    BOOL bRotateRight;

    BOOL b1, b2, b3, b4, b5, b6, b7, b8, b9;
    BOOL bf5, bf6, bf7, bf8, bf9;
};

enum DISPLAY_METHOD
{
    DM_LINEAR,
};

class CMyD3DApplication : public CD3DApplication
{
    BOOL m_bLoadingApp;          // 애플리케이션이 로딩 중인지 여부
    CD3DFont* m_pFont;           // 텍스트를 그리기 위한 폰트
    ID3DXMesh* m_pD3DXMesh;      // 주전자(Teapot)를 저장하기 위한 D3DX 메쉬
    ID3DXPatchMesh* m_pD3DXPatchMesh; // 주전자(Teapot)를 저장하기 위한 D3DX 패치 메쉬
    ID3DXMesh* m_pD3DXPatchMeshDest;  // 주전자(Teapot)를 저장하기 위한 D3DX 패치 메쉬
    LPDIRECTINPUT8 m_pDI;        // DirectInput 객체
    LPDIRECTINPUTDEVICE8 m_pKeyboard; // DirectInput 키보드 장치
    UserInput m_UserInput;       // 사용자 입력을 저장하는 구조체 
    FLOAT m_fWorldRotX;          // 월드 회전 상태 X축
    FLOAT m_fWorldRotY;          // 월드 회전 상태 Y축
    LPD3DXEFFECT m_linearFX;     // npatch 효과

    unsigned int			m_numVertices;
    unsigned int			m_numIndices;
    float* m_posVertexData;
    float* m_normVertexData;
    float* m_uvVertexData;
    WORD* m_indexData;

    LPDIRECT3DVERTEXBUFFER9 m_pVB; // 실제로 렌더링할 VB
    LPDIRECT3DINDEXBUFFER9 m_pIB;  // 실제로 렌더링할 IB (사용할 경우)

    D3DXMATRIX				m_matProj;
    D3DXMATRIX				m_matView;
    D3DXMATRIX				m_matWorld;
    DISPLAY_METHOD			m_displayMethod;

    HRESULT SetupDisplayMethod();
    void CleanDisplayMethod();
    HRESULT GetXMeshData(LPD3DXMESH in);


    // 테셀레이터 관련 변수
    unsigned int			m_lod;
    float LookupDisplacementValue(WORD i0, WORD i1, WORD i2, float i, float j);
    void CreateMandelbrot(unsigned int dwSize);
    float* m_DisplacementTexture;

    // 선형 표면 기초
    LPDIRECT3DVERTEXDECLARATION9	m_pLinearDecl;
    float* m_linearPos;
    float* m_linearNorm;
    HRESULT							GenerateLinearConstants();
    HRESULT							GenerateLinearBuffers(unsigned int LOD);

protected:
    virtual HRESULT OneTimeSceneInit();
    virtual HRESULT InitDeviceObjects();
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
    virtual HRESULT Render();
    virtual HRESULT FrameMove();
    virtual HRESULT FinalCleanup();
    virtual HRESULT ConfirmDevice(D3DCAPS9*, DWORD, D3DFORMAT);

    HRESULT RenderText();

    HRESULT InitInput(HWND hWnd);
    void    UpdateInput(UserInput* pUserInput);
    void    CleanupDirectInput();
    VOID    ReadSettings();
    VOID    WriteSettings();

public:
    LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    CMyD3DApplication();
    virtual ~CMyD3DApplication();
};