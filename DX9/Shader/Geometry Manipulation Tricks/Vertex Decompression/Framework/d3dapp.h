#ifndef D3DAPP_H
#define D3DAPP_H

//에러코드
enum APPMSGTYPE { MSG_NONE, MSGERR_APPMUSTEXIT, MSGWARN_SWITCHEDTOREF };

#define D3DAPPERR_NODIRECT3D          0x82000001
#define D3DAPPERR_NOWINDOW            0x82000002
#define D3DAPPERR_NOCOMPATIBLEDEVICES 0x82000003
#define D3DAPPERR_NOWINDOWABLEDEVICES 0x82000004
#define D3DAPPERR_NOHARDWAREDEVICE    0x82000005
#define D3DAPPERR_HALNOTCOMPATIBLE    0x82000006
#define D3DAPPERR_NOWINDOWEDHAL       0x82000007
#define D3DAPPERR_NODESKTOPHAL        0x82000008
#define D3DAPPERR_NOHALTHISMODE       0x82000009
#define D3DAPPERR_NONZEROREFCOUNT     0x8200000a
#define D3DAPPERR_MEDIANOTFOUND       0x8200000b
#define D3DAPPERR_RESETFAILED         0x8200000c
#define D3DAPPERR_NULLREFDEVICE       0x8200000d

class CD3DApplication
{
protected:
    CD3DEnumeration   m_d3dEnumeration;
    CD3DSettings      m_d3dSettings;

    // Internal variables for the state of the app
    bool              m_bWindowed;
    bool              m_bActive;
    bool              m_bDeviceLost;
    bool              m_bMinimized;
    bool              m_bMaximized;
    bool              m_bIgnoreSizeChange;
    bool              m_bDeviceObjectsInited;
    bool              m_bDeviceObjectsRestored;

    // 타이밍에 사용되는 내부 변수
    bool              m_bFrameMoving;
    bool              m_bSingleStep;

    // 내부 오류 처리 함수
    HRESULT DisplayErrorMsg( HRESULT hr, DWORD dwType );

    // 3D 장면을 관리하고 렌더링하는 내부 함수
    static bool ConfirmDeviceHelper( D3DCAPS9* pCaps, 
        VertexProcessingType vertexProcessingType, D3DFORMAT backBufferFormat );
    void    BuildPresentParamsFromSettings();
    bool    FindBestWindowedMode( bool bRequireHAL, bool bRequireREF );
    bool    FindBestFullscreenMode( bool bRequireHAL, bool bRequireREF );
    HRESULT ChooseInitialD3DSettings();
    HRESULT Initialize3DEnvironment();
    HRESULT HandlePossibleSizeChange();
    HRESULT Reset3DEnvironment();
    HRESULT ToggleFullscreen();
    HRESULT ForceWindowed();
    HRESULT UserSelectNewDevice();
    void    Cleanup3DEnvironment();
    HRESULT Render3DEnvironment();
    virtual HRESULT AdjustWindowForChange();
    virtual void UpdateStats();

protected:
    // 3D 장면 생성 및 렌더링에 사용되는 주요 개체
    D3DPRESENT_PARAMETERS m_d3dpp;         // CreateDevice/Reset에 대한 매개변수
    HWND              m_hWnd;              // 메인 앱 창
    HWND              m_hWndFocus;         // D3D 포커스 창(보통 m_hWnd와 동일)
    HMENU             m_hMenu;             // 앱 메뉴 표시줄(전체 화면일 때 여기에 저장됨)
    LPDIRECT3D9       m_pD3D;              // 메인 D3D 객체
    LPDIRECT3DDEVICE9 m_pd3dDevice;        // D3D 렌더링 장치
    D3DCAPS9          m_d3dCaps;           // 장치의 캡
    D3DSURFACE_DESC   m_d3dsdBackBuffer;   // 백버퍼의 표면 설명
    DWORD             m_dwCreateFlags;     // sw 또는 hw 정점 처리를 나타냅니다.
    DWORD             m_dwWindowStyle;     // 모드 스위치에 대해 저장된 창 스타일
    RECT              m_rcWindowBounds;    // 모드 스위치에 대해 저장된 창 경계
    RECT              m_rcWindowClient;    // 모드 전환을 위해 저장된 클라이언트 영역 크기

    // 타이밍 변수
    FLOAT             m_fTime;             // 현재 시간(초)
    FLOAT             m_fElapsedTime;      // 마지막 프레임 이후 경과된 시간
    FLOAT             m_fFPS;              // 순간 프레임 속도
    TCHAR             m_strDeviceStats[90];// D3D 장치 통계를 담는 문자열
    TCHAR             m_strFrameStats[90]; // 프레임 통계를 담는 문자열

    // 앱의 재정의 가능한 변수
    TCHAR*            m_strWindowTitle;    // 앱 창 제목
    DWORD             m_dwCreationWidth;   // 창을 만드는 데 사용되는 너비
    DWORD             m_dwCreationHeight;  // 윈도우 생성에 사용된 높이
    bool              m_bShowCursorWhenFullscreen; // 전체 화면일 때 커서를 표시할지 여부
    bool              m_bClipCursorWhenFullscreen; // 전체 화면일 때 커서 위치를 제한할지 여부
    bool              m_bStartFullscreen;    // 전체 화면 모드에서 앱을 시작할지 여부

    // 앱에서 생성된 3D 장면에 대한 재정의 가능한 함수
    virtual HRESULT ConfirmDevice(D3DCAPS9*,DWORD,D3DFORMAT)   { return S_OK; }
    virtual HRESULT OneTimeSceneInit()                         { return S_OK; }
    virtual HRESULT InitDeviceObjects()                        { return S_OK; }
    virtual HRESULT RestoreDeviceObjects()                     { return S_OK; }
    virtual HRESULT FrameMove()                                { return S_OK; }
    virtual HRESULT Render()                                   { return S_OK; }
    virtual HRESULT InvalidateDeviceObjects()                  { return S_OK; }
    virtual HRESULT DeleteDeviceObjects()                      { return S_OK; }
    virtual HRESULT FinalCleanup()                             { return S_OK; }

public:
    // 애플리케이션을 생성, 실행, 일시정지, 정리하는 함수
    virtual HRESULT Create( HINSTANCE hInstance );
    virtual INT     Run();
    virtual LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    virtual void    Pause( bool bPause );
    virtual ~CD3DApplication(){ }

    // 내부 생성자
    CD3DApplication();
};
#endif