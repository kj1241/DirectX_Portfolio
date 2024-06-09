#ifndef D3DFONT_H
#define D3DFONT_H
#include <tchar.h>
#include <D3D9.h>

// 글꼴 생성 플래그
#define D3DFONT_BOLD        0x0001
#define D3DFONT_ITALIC      0x0002
#define D3DFONT_ZENABLE     0x0004

// 글꼴 렌더링 플래그
#define D3DFONT_CENTERED_X  0x0001
#define D3DFONT_CENTERED_Y  0x0002
#define D3DFONT_TWOSIDED    0x0004
#define D3DFONT_FILTERED    0x0008

class CD3DFont
{
    TCHAR   m_strFontName[80];    // 글꼴 속성
    DWORD   m_dwFontHeight;
    DWORD   m_dwFontFlags;

    LPDIRECT3DDEVICE9       m_pd3dDevice; // 렌더링에 사용되는 D3DDevice
    LPDIRECT3DTEXTURE9      m_pTexture;   // 이 글꼴의 d3d 텍스처
    LPDIRECT3DVERTEXBUFFER9 m_pVB;        // 텍스트 렌더링을 위한 VertexBuffer
    DWORD   m_dwTexWidth;                 // 텍스처 크기
    DWORD   m_dwTexHeight;
    FLOAT   m_fTextScale;
    FLOAT   m_fTexCoords[128-32][4];
    DWORD   m_dwSpacing;                  // 측면당 문자 픽셀 간격

    // 렌더 상태 설정 및 복원을 위한 상태 블록
    LPDIRECT3DSTATEBLOCK9 m_pStateBlockSaved;
    LPDIRECT3DSTATEBLOCK9 m_pStateBlockDrawText;

public:
    // 2D 및 3D 텍스트 그리기 기능
    HRESULT DrawText( FLOAT x, FLOAT y, DWORD dwColor, 
                      const TCHAR* strText, DWORD dwFlags=0L );
    HRESULT DrawTextScaled( FLOAT x, FLOAT y, FLOAT z, 
                            FLOAT fXScale, FLOAT fYScale, DWORD dwColor, 
                            const TCHAR* strText, DWORD dwFlags=0L );
    HRESULT Render3DText( const TCHAR* strText, DWORD dwFlags=0L );
    
    // 텍스트의 범위를 가져오는 함수
    HRESULT GetTextExtent( const TCHAR* strText, SIZE* pSize );

    // 장치 종속 개체 초기화 및 삭제
    HRESULT InitDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice );
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();

    // 생성자 / 소멸자
    CD3DFont( const TCHAR* strFontName, DWORD dwHeight, DWORD dwFlags=0L );
    ~CD3DFont();
};
#endif


