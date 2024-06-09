#ifndef D3DENUM_H
#define D3DENUM_H

enum VertexProcessingType
{
    SOFTWARE_VP,
    MIXED_VP,
    HARDWARE_VP,
    PURE_HARDWARE_VP
};

struct D3DAdapterInfo
{
    int AdapterOrdinal;
    D3DADAPTER_IDENTIFIER9 AdapterIdentifier;
    CArrayList* pDisplayModeList; // D3DDISPLAYMODE 목록
    CArrayList* pDeviceInfoList; // D3DDeviceInfo 포인터 목록
    ~D3DAdapterInfo( void );
};

struct D3DDeviceInfo
{
    int AdapterOrdinal;
    D3DDEVTYPE DevType;
    D3DCAPS9 Caps;
    CArrayList* pDeviceComboList; // D3DDeviceCombo 포인터 목록
    ~D3DDeviceInfo( void );
};

struct D3DDSMSConflict
{
    D3DFORMAT DSFormat;
    D3DMULTISAMPLE_TYPE MSType;
};

struct D3DDeviceCombo
{
    int AdapterOrdinal;
    D3DDEVTYPE DevType;
    D3DFORMAT AdapterFormat;
    D3DFORMAT BackBufferFormat;
    bool IsWindowed;
    CArrayList* pDepthStencilFormatList; // D3DFORMAT 목록
    CArrayList* pMultiSampleTypeList;    // D3DMULTISAMPLE_TYPE 목록
    CArrayList* pMultiSampleQualityList; // DWORD 목록(각 다중 샘플 유형의 품질 수준 수)
    CArrayList* pDSMSConflictList; // D3DDSMS충돌 목록
    CArrayList* pVertexProcessingTypeList; // VertexProcessingType 목록
    CArrayList* pPresentIntervalList;   // D3DPRESENT_INTERVAL 목록

    ~D3DDeviceCombo( void );
};


typedef bool(* CONFIRMDEVICECALLBACK)( D3DCAPS9* pCaps, 
    VertexProcessingType vertexProcessingType, D3DFORMAT backBufferFormat );

class CD3DEnumeration
{
private:
    IDirect3D9* m_pD3D;

private:
    HRESULT EnumerateDevices( D3DAdapterInfo* pAdapterInfo, CArrayList* pAdapterFormatList );
    HRESULT EnumerateDeviceCombos( D3DDeviceInfo* pDeviceInfo, CArrayList* pAdapterFormatList );
    void BuildDepthStencilFormatList( D3DDeviceCombo* pDeviceCombo );
    void BuildMultiSampleTypeList( D3DDeviceCombo* pDeviceCombo );
    void BuildDSMSConflictList( D3DDeviceCombo* pDeviceCombo );
    void BuildVertexProcessingTypeList( D3DDeviceInfo* pDeviceInfo, D3DDeviceCombo* pDeviceCombo );
    void BuildPresentIntervalList( D3DDeviceInfo* pDeviceInfo, D3DDeviceCombo* pDeviceCombo );

public:
    CArrayList* m_pAdapterInfoList;
    // 다음 변수를 사용하여 모드, 형식, 
    // 등이 열거됩니다.  전화하기 전에 원하는 값으로 설정하십시오.
    // Enumerate().
    CONFIRMDEVICECALLBACK ConfirmDeviceCallback;
    UINT AppMinFullscreenWidth;
    UINT AppMinFullscreenHeight;
    UINT AppMinColorChannelBits; // 어댑터 형식의 채널당 최소 색상 비트
    UINT AppMinAlphaChannelBits; // 백 버퍼 형식의 픽셀당 최소 알파 비트
    UINT AppMinDepthBits;
    UINT AppMinStencilBits;
    bool AppUsesDepthBuffer;
    bool AppUsesMixedVP; // 앱이 혼합 vp 모드를 활용할 수 있는지 여부
    bool AppRequiresWindowed;
    bool AppRequiresFullscreen;
    CArrayList* m_pAllowedAdapterFormatList;   // D3DFORMAT 목록

    CD3DEnumeration();
    ~CD3DEnumeration();
    void SetD3D(IDirect3D9* pD3D) { m_pD3D = pD3D; }
    HRESULT Enumerate();
};
#endif