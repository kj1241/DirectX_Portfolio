#define STRICT
#include <stdio.h>
#include <tchar.h>
#include <D3DX9.h>
#include "D3DFont.h"
#include "D3DUtil.h"
#include "DXUtil.h"

#define MAX_NUM_VERTICES 50*6

struct FONT2DVERTEX { 
    D3DXVECTOR4 p;   
    DWORD color;     
    FLOAT tu, tv; 
};
struct FONT3DVERTEX { 
    D3DXVECTOR3 p;   
    D3DXVECTOR3 n;   
    FLOAT tu, tv; 
};

#define D3DFVF_FONT2DVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#define D3DFVF_FONT3DVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

inline FONT2DVERTEX InitFont2DVertex( const D3DXVECTOR4& p, D3DCOLOR color, FLOAT tu, FLOAT tv )
{
    FONT2DVERTEX v;   v.p = p;   v.color = color;   v.tu = tu;   v.tv = tv;
    return v;
}

inline FONT3DVERTEX InitFont3DVertex( const D3DXVECTOR3& p, const D3DXVECTOR3& n, FLOAT tu, FLOAT tv )
{
    FONT3DVERTEX v;   v.p = p;   v.n = n;   v.tu = tu;   v.tv = tv;
    return v;
}

//글꼴 클래스 생성자
CD3DFont::CD3DFont( const TCHAR* strFontName, DWORD dwHeight, DWORD dwFlags )
{
    _tcsncpy( m_strFontName, strFontName, sizeof(m_strFontName) / sizeof(TCHAR) );
    m_strFontName[sizeof(m_strFontName) / sizeof(TCHAR) - 1] = _T('\0');
    m_dwFontHeight         = dwHeight;
    m_dwFontFlags          = dwFlags;
    m_dwSpacing            = 0;

    m_pd3dDevice           = NULL;
    m_pTexture             = NULL;
    m_pVB                  = NULL;

    m_pStateBlockSaved     = NULL;
    m_pStateBlockDrawText  = NULL;
}

//글꼴 클래스 소멸자
CD3DFont::~CD3DFont()
{
    InvalidateDeviceObjects();
    DeleteDeviceObjects();
}

//사용된 정점 버퍼를 포함하여 장치 종속 개체를 초기화합니다. 텍스트와 글꼴 이미지를 저장하는 텍스처 맵을 렌더링합니다.
HRESULT CD3DFont::InitDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice )
{
    HRESULT hr;

    // 장치의 로컬 복사본을 유지합니다.
    m_pd3dDevice = pd3dDevice;

    // 글꼴 및 텍스처 크기 설정
    m_fTextScale  = 1.0f;    // 크기 조정 없이 글꼴을 텍스처에 그립니다.

    // Large fonts need larger textures
    if( m_dwFontHeight > 60 )
        m_dwTexWidth = m_dwTexHeight = 2048;
    else if( m_dwFontHeight > 30 )
        m_dwTexWidth = m_dwTexHeight = 1024;
    else if( m_dwFontHeight > 15 )
        m_dwTexWidth = m_dwTexHeight = 512;
    else
        m_dwTexWidth  = m_dwTexHeight = 256;

    // 요청한 텍스처가 너무 크면 더 작은 텍스처와 더 작은 글꼴을 사용하고,
    // 렌더링할 때 크기를 늘립니다.
    D3DCAPS9 d3dCaps;
    m_pd3dDevice->GetDeviceCaps( &d3dCaps );

    if( m_dwTexWidth > d3dCaps.MaxTextureWidth )
    {
        m_fTextScale = (FLOAT)d3dCaps.MaxTextureWidth / (FLOAT)m_dwTexWidth;
        m_dwTexWidth = m_dwTexHeight = d3dCaps.MaxTextureWidth;
    }

    // 글꼴에 대한 새 텍스처를 만듭니다.
    hr = m_pd3dDevice->CreateTexture( m_dwTexWidth, m_dwTexHeight, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, &m_pTexture, NULL );
    if( FAILED(hr) )
        return hr;

    // 비트맵 생성 준비
    DWORD*      pBitmapBits;
    BITMAPINFO bmi;
    ZeroMemory( &bmi.bmiHeader,  sizeof(BITMAPINFOHEADER) );
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       =  (int)m_dwTexWidth;
    bmi.bmiHeader.biHeight      = -(int)m_dwTexHeight;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biBitCount    = 32;

    // 글꼴에 대한 DC 및 비트맵을 생성합니다.
    HDC     hDC       = CreateCompatibleDC( NULL );
    HBITMAP hbmBitmap = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS,(void**)&pBitmapBits, NULL, 0 );
    SetMapMode( hDC, MM_TEXT );

    // 글꼴을 생성합니다.  ANTIALIASED_QUALITY를 지정하면 다음과 같은 결과를 얻을 수 있습니다.
    // 앤티앨리어싱된 글꼴이지만 보장되지는 않습니다.
    INT nHeight    = -MulDiv( m_dwFontHeight, 
        (INT)(GetDeviceCaps(hDC, LOGPIXELSY) * m_fTextScale), 72 );
    DWORD dwBold   = (m_dwFontFlags&D3DFONT_BOLD)   ? FW_BOLD : FW_NORMAL;
    DWORD dwItalic = (m_dwFontFlags&D3DFONT_ITALIC) ? TRUE    : FALSE;
    HFONT hFont    = CreateFont( nHeight, 0, 0, 0, dwBold, dwItalic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, m_strFontName );
    if( NULL==hFont )
        return E_FAIL;

    HGDIOBJ hbmOld = SelectObject( hDC, hbmBitmap );
    HGDIOBJ hFontOld = SelectObject( hDC, hFont );

    // 텍스트 속성 설정
    SetTextColor( hDC, RGB(255,255,255) );
    SetBkColor(   hDC, 0x00000000 );
    SetTextAlign( hDC, TA_TOP );

    // 인쇄 가능한 모든 문자를 반복하여 비트맵에 출력합니다.
    // 한편, 각 캐릭터에 해당하는 텍스 코드를 추적합니다.
    DWORD x = 0;
    DWORD y = 0;
    TCHAR str[2] = _T("x");
    SIZE size;

    // 줄 높이를 기준으로 문자 사이의 간격을 계산합니다.
    GetTextExtentPoint32( hDC, TEXT(" "), 1, &size );
    x = m_dwSpacing = (DWORD) ceil(size.cy * 0.3f);

    for( TCHAR c=32; c<127; c++ )
    {
        str[0] = c;
        GetTextExtentPoint32( hDC, str, 1, &size );

        if( (DWORD)(x + size.cx + m_dwSpacing) > m_dwTexWidth )
        {
            x  = m_dwSpacing;
            y += size.cy+1;
        }

        ExtTextOut( hDC, x+0, y+0, ETO_OPAQUE, NULL, str, 1, NULL );

        m_fTexCoords[c-32][0] = ((FLOAT)(x + 0       - m_dwSpacing))/m_dwTexWidth;
        m_fTexCoords[c-32][1] = ((FLOAT)(y + 0       + 0          ))/m_dwTexHeight;
        m_fTexCoords[c-32][2] = ((FLOAT)(x + size.cx + m_dwSpacing))/m_dwTexWidth;
        m_fTexCoords[c-32][3] = ((FLOAT)(y + size.cy + 0          ))/m_dwTexHeight;

        x += size.cx + (2 * m_dwSpacing);
    }

    // 표면을 잠그고 설정된 픽셀에 대한 알파 값을 씁니다.
    D3DLOCKED_RECT d3dlr;
    m_pTexture->LockRect( 0, &d3dlr, 0, 0 );
    BYTE* pDstRow = (BYTE*)d3dlr.pBits;
    WORD* pDst16;
    BYTE bAlpha;    // 픽셀 강도의 4비트 측정값

    for( y=0; y < m_dwTexHeight; y++ )
    {
        pDst16 = (WORD*)pDstRow;
        for( x=0; x < m_dwTexWidth; x++ )
        {
            bAlpha = (BYTE)((pBitmapBits[m_dwTexWidth*y + x] & 0xff) >> 4);
            if (bAlpha > 0)
            {
                *pDst16++ = (WORD) ((bAlpha << 12) | 0x0fff);
            }
            else
            {
                *pDst16++ = 0x0000;
            }
        }
        pDstRow += d3dlr.Pitch;
    }

    // 텍스처 업데이트가 완료되었으므로 사용한 개체를 정리합니다.
    m_pTexture->UnlockRect(0);
    SelectObject( hDC, hbmOld );
    SelectObject( hDC, hFontOld );
    DeleteObject( hbmBitmap );
    DeleteObject( hFont );
    DeleteDC( hDC );

    return S_OK;
}

HRESULT CD3DFont::RestoreDeviceObjects()
{
    HRESULT hr;


    // 문자에 대한 정점 버퍼를 생성합니다.
    int vertexSize = max( sizeof(FONT2DVERTEX), sizeof(FONT3DVERTEX ) );
    if( FAILED( hr = m_pd3dDevice->CreateVertexBuffer( MAX_NUM_VERTICES * vertexSize, D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &m_pVB, NULL ) ) )
        return hr;
    
    // 텍스트 렌더링을 위한 상태 블록 생성
    for( UINT which=0; which<2; which++ )
    {
        m_pd3dDevice->BeginStateBlock();
        m_pd3dDevice->SetTexture( 0, m_pTexture );

        if ( D3DFONT_ZENABLE & m_dwFontFlags )
            m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
        else
            m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );

        m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
        m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,   D3DBLEND_SRCALPHA );
        m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND,  D3DBLEND_INVSRCALPHA );
        m_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE,  TRUE );
        m_pd3dDevice->SetRenderState( D3DRS_ALPHAREF,         0x08 );
        m_pd3dDevice->SetRenderState( D3DRS_ALPHAFUNC,  D3DCMP_GREATEREQUAL );
        m_pd3dDevice->SetRenderState( D3DRS_FILLMODE,   D3DFILL_SOLID );
        m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,   D3DCULL_CCW );
        m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
        m_pd3dDevice->SetRenderState( D3DRS_CLIPPING,         TRUE );
        m_pd3dDevice->SetRenderState( D3DRS_CLIPPLANEENABLE,  FALSE );
        m_pd3dDevice->SetRenderState( D3DRS_VERTEXBLEND,      D3DVBF_DISABLE );
        m_pd3dDevice->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE );
        m_pd3dDevice->SetRenderState( D3DRS_FOGENABLE,        FALSE );
        m_pd3dDevice->SetRenderState( D3DRS_COLORWRITEENABLE,
            D3DCOLORWRITEENABLE_RED  | D3DCOLORWRITEENABLE_GREEN |
            D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
        m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
        m_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );

        if( which==0 )
            m_pd3dDevice->EndStateBlock( &m_pStateBlockSaved );
        else
            m_pd3dDevice->EndStateBlock( &m_pStateBlockDrawText );
    }

    return S_OK;
}

//모든 장치 종속 개체를 삭제합니다.
HRESULT CD3DFont::InvalidateDeviceObjects()
{
    SAFE_RELEASE( m_pVB );
    SAFE_RELEASE( m_pStateBlockSaved );
    SAFE_RELEASE( m_pStateBlockDrawText );

    return S_OK;
}

//모든 장치 종속 개체를 삭제합니다.
HRESULT CD3DFont::DeleteDeviceObjects()
{
    SAFE_RELEASE( m_pTexture );
    m_pd3dDevice = NULL;

    return S_OK;
}

//텍스트 문자열의 크기를 가져옵니다.
HRESULT CD3DFont::GetTextExtent( const TCHAR* strText, SIZE* pSize )
{
    if( NULL==strText || NULL==pSize )
        return E_FAIL;

    FLOAT fRowWidth  = 0.0f;
    FLOAT fRowHeight = (m_fTexCoords[0][3]-m_fTexCoords[0][1])*m_dwTexHeight;
    FLOAT fWidth     = 0.0f;
    FLOAT fHeight    = fRowHeight;

    while( *strText )
    {
        TCHAR c = *strText++;

        if( c == _T('\n') )
        {
            fRowWidth = 0.0f;
            fHeight  += fRowHeight;
        }

        if( (c-32) < 0 || (c-32) >= 128-32 )
            continue;

        FLOAT tx1 = m_fTexCoords[c-32][0];
        FLOAT tx2 = m_fTexCoords[c-32][2];

        fRowWidth += (tx2-tx1)*m_dwTexWidth - 2*m_dwSpacing;

        if( fRowWidth > fWidth )
            fWidth = fRowWidth;
    }

    pSize->cx = (int)fWidth;
    pSize->cy = (int)fHeight;

    return S_OK;
}

//크기가 조정된 2D 텍스트를 그립니다.  x와 y는 뷰포트 좌표에 있습니다.
//(범위: -1 ~ +1).  fXScale 및 fYScale은 크기 비율입니다. 
//전체 뷰포트에 상대적입니다.  예를 들어, fXScale 0.25는
//화면 너비의 1/8입니다.  이를 통해 고정된 텍스트를 출력할 수 있습니다.
//화면이나 창 크기가 변경되더라도 뷰포트의 일부입니다.
HRESULT CD3DFont::DrawTextScaled( FLOAT x, FLOAT y, FLOAT z, FLOAT fXScale, FLOAT fYScale, DWORD dwColor, const TCHAR* strText, DWORD dwFlags )
{
    if( m_pd3dDevice == NULL )
        return E_FAIL;

    // 렌더 상태 설정
    m_pStateBlockSaved->Capture();
    m_pStateBlockDrawText->Apply();
    m_pd3dDevice->SetFVF( D3DFVF_FONT2DVERTEX );
    m_pd3dDevice->SetPixelShader( NULL );
    m_pd3dDevice->SetStreamSource( 0, m_pVB, 0, sizeof(FONT2DVERTEX) );

    // 필터 상태 설정
    if( dwFlags & D3DFONT_FILTERED )
    {
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    }

    D3DVIEWPORT9 vp;
    m_pd3dDevice->GetViewport( &vp );
    FLOAT fLineHeight = ( m_fTexCoords[0][3] - m_fTexCoords[0][1] ) * m_dwTexHeight;

    // 뷰포트의 중앙에 텍스트 블록을 놓습니다.
    if( dwFlags & D3DFONT_CENTERED_X )
    {
        const TCHAR* strTextTmp = strText;
        float xFinal = 0.0f;

        while( *strTextTmp )
        {
            TCHAR c = *strTextTmp++;
    
            if( c == _T('\n') )
                break;            // 지원되지 않습니다.
            if( (c-32) < 0 || (c-32) >= 128-32 )
                continue;

            FLOAT tx1 = m_fTexCoords[c-32][0];
            FLOAT tx2 = m_fTexCoords[c-32][2];

            FLOAT w = (tx2-tx1)*m_dwTexWidth;

            w *= (fXScale*vp.Height)/fLineHeight;

            xFinal += w - (2 * m_dwSpacing) * (fXScale*vp.Height)/fLineHeight;
        }

        x = -xFinal/vp.Width;
    }
    if( dwFlags & D3DFONT_CENTERED_Y )
    {
        y = -fLineHeight/vp.Height;
    }

    FLOAT sx  = (x+1.0f)*vp.Width/2;
    FLOAT sy  = (y+1.0f)*vp.Height/2;
    FLOAT sz  = z;
    FLOAT rhw = 1.0f;

    // 문자 간격 조정
    sx -= m_dwSpacing * (fXScale*vp.Height)/fLineHeight;
    FLOAT fStartX = sx;

    // 정점 버퍼 채우기
    FONT2DVERTEX* pVertices;
    DWORD         dwNumTriangles = 0L;
    m_pVB->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD );

    while( *strText )
    {
        TCHAR c = *strText++;

        if( c == _T('\n') )
        {
            sx  = fStartX;
            sy += fYScale*vp.Height;
        }

        if( (c-32) < 0 || (c-32) >= 128-32 )
            continue;

        FLOAT tx1 = m_fTexCoords[c-32][0];
        FLOAT ty1 = m_fTexCoords[c-32][1];
        FLOAT tx2 = m_fTexCoords[c-32][2];
        FLOAT ty2 = m_fTexCoords[c-32][3];

        FLOAT w = (tx2-tx1)*m_dwTexWidth;
        FLOAT h = (ty2-ty1)*m_dwTexHeight;

        w *= (fXScale*vp.Height)/fLineHeight;
        h *= (fYScale*vp.Height)/fLineHeight;

        if( c != _T(' ') )
        {
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+0-0.5f,sy+h-0.5f,sz,rhw), dwColor, tx1, ty2 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+0-0.5f,sy+0-0.5f,sz,rhw), dwColor, tx1, ty1 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+w-0.5f,sy+h-0.5f,sz,rhw), dwColor, tx2, ty2 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+w-0.5f,sy+0-0.5f,sz,rhw), dwColor, tx2, ty1 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+w-0.5f,sy+h-0.5f,sz,rhw), dwColor, tx2, ty2 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+0-0.5f,sy+0-0.5f,sz,rhw), dwColor, tx1, ty1 );
            dwNumTriangles += 2;

            if( dwNumTriangles*3 > (MAX_NUM_VERTICES-6) )
            {
                // 정점 버퍼 잠금 해제, 렌더링 및 다시 잠그기
                m_pVB->Unlock();
                m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, dwNumTriangles );
                m_pVB->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD );
                dwNumTriangles = 0L;
            }
        }

        sx += w - (2 * m_dwSpacing) * (fXScale*vp.Height)/fLineHeight;
    }

    // 정점 버퍼 잠금 해제 및 렌더링
    m_pVB->Unlock();
    if( dwNumTriangles > 0 )
        m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, dwNumTriangles );

    // 수정된 렌더 상태를 복원합니다.
    m_pStateBlockSaved->Apply();

    return S_OK;
}

//2D 텍스트를 그립니다.sx와 sy는 픽셀 단위입니다.
HRESULT CD3DFont::DrawText( FLOAT sx, FLOAT sy, DWORD dwColor,  const TCHAR* strText, DWORD dwFlags )
{
    if( m_pd3dDevice == NULL )
        return E_FAIL;

    // 렌더상태 설정
    m_pStateBlockSaved->Capture();
    m_pStateBlockDrawText->Apply();
    m_pd3dDevice->SetFVF( D3DFVF_FONT2DVERTEX );
    m_pd3dDevice->SetPixelShader( NULL );
    m_pd3dDevice->SetStreamSource( 0, m_pVB, 0, sizeof(FONT2DVERTEX) );

    // 필터 상태 설정
    if( dwFlags & D3DFONT_FILTERED )
    {
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    }

    // 뷰포트의 중앙에 텍스트 블록을 놓습니다.
    if( dwFlags & D3DFONT_CENTERED_X )
    {
        D3DVIEWPORT9 vp;
        m_pd3dDevice->GetViewport( &vp );
        const TCHAR* strTextTmp = strText;
        float xFinal = 0.0f;

        while( *strTextTmp )
        {
            TCHAR c = *strTextTmp++;
    
            if( c == _T('\n') )
                break;            // 지원되지 않습니다.
            if( (c-32) < 0 || (c-32) >= 128-32 )
                continue;

            FLOAT tx1 = m_fTexCoords[c-32][0];
            FLOAT tx2 = m_fTexCoords[c-32][2];
    
            FLOAT w = (tx2-tx1) *  m_dwTexWidth / m_fTextScale;
    
            xFinal += w - (2 * m_dwSpacing);
        }

        sx = (vp.Width-xFinal)/2.0f;
    }
    if( dwFlags & D3DFONT_CENTERED_Y )
    {
        D3DVIEWPORT9 vp;
        m_pd3dDevice->GetViewport( &vp );
        float fLineHeight = ((m_fTexCoords[0][3]-m_fTexCoords[0][1])*m_dwTexHeight);
        sy = (vp.Height-fLineHeight)/2;
    }

    // 문자 간격 조정
    sx -= m_dwSpacing;
    FLOAT fStartX = sx;

    // 정점 버퍼 채우기
    FONT2DVERTEX* pVertices = NULL;
    DWORD         dwNumTriangles = 0;
    m_pVB->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD );

    while( *strText )
    {
        TCHAR c = *strText++;

        if( c == _T('\n') )
        {
            sx = fStartX;
            sy += (m_fTexCoords[0][3]-m_fTexCoords[0][1])*m_dwTexHeight;
        }

        if( (c-32) < 0 || (c-32) >= 128-32 )
            continue;

        FLOAT tx1 = m_fTexCoords[c-32][0];
        FLOAT ty1 = m_fTexCoords[c-32][1];
        FLOAT tx2 = m_fTexCoords[c-32][2];
        FLOAT ty2 = m_fTexCoords[c-32][3];

        FLOAT w = (tx2-tx1) *  m_dwTexWidth / m_fTextScale;
        FLOAT h = (ty2-ty1) * m_dwTexHeight / m_fTextScale;

        if( c != _T(' ') )
        {
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+0-0.5f,sy+h-0.5f,0.9f,1.0f), dwColor, tx1, ty2 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+0-0.5f,sy+0-0.5f,0.9f,1.0f), dwColor, tx1, ty1 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+w-0.5f,sy+h-0.5f,0.9f,1.0f), dwColor, tx2, ty2 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+w-0.5f,sy+0-0.5f,0.9f,1.0f), dwColor, tx2, ty1 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+w-0.5f,sy+h-0.5f,0.9f,1.0f), dwColor, tx2, ty2 );
            *pVertices++ = InitFont2DVertex( D3DXVECTOR4(sx+0-0.5f,sy+0-0.5f,0.9f,1.0f), dwColor, tx1, ty1 );
            dwNumTriangles += 2;

            if( dwNumTriangles*3 > (MAX_NUM_VERTICES-6) )
            {
                // 정점 버퍼 잠금 해제, 렌더링 및 다시 잠그기
                m_pVB->Unlock();
                m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, dwNumTriangles );
                pVertices = NULL;
                m_pVB->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD );
                dwNumTriangles = 0L;
            }
        }

        sx += w - (2 * m_dwSpacing);
    }

    // 정점 버퍼 잠금 해제 및 렌더링
    m_pVB->Unlock();
    if( dwNumTriangles > 0 )
        m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, dwNumTriangles );
    // 수정된 렌더 상태를 복원합니다.
    m_pStateBlockSaved->Apply();

    return S_OK;
}

//3D 텍스트를 렌더링합니다.
HRESULT CD3DFont::Render3DText( const TCHAR* strText, DWORD dwFlags )
{
    if( m_pd3dDevice == NULL )
        return E_FAIL;

    // 렌더상태 설정
    m_pStateBlockSaved->Capture();
    m_pStateBlockDrawText->Apply();
    m_pd3dDevice->SetFVF( D3DFVF_FONT3DVERTEX );
    m_pd3dDevice->SetPixelShader( NULL );
    m_pd3dDevice->SetStreamSource( 0, m_pVB, 0, sizeof(FONT3DVERTEX) );

    // 필터 상태 설정
    if( dwFlags & D3DFONT_FILTERED )
    {
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    }

    // 각 텍스트 요소의 위치
    FLOAT x = 0.0f;
    FLOAT y = 0.0f;

    // 텍스트 블록을 원점(뷰포트 아님) 중앙에 배치합니다.
    if( dwFlags & D3DFONT_CENTERED_X )
    {
        SIZE sz;
        GetTextExtent( strText, &sz );
        x = -(((FLOAT)sz.cx)/10.0f)/2.0f;
    }
    if( dwFlags & D3DFONT_CENTERED_Y )
    {
        SIZE sz;
        GetTextExtent( strText, &sz );
        y = -(((FLOAT)sz.cy)/10.0f)/2.0f;
    }

    // 양면 텍스트에 대한 컬링을 끕니다.
    if( dwFlags & D3DFONT_TWOSIDED )
        m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

    // 문자 간격 조정
    x -= m_dwSpacing / 10.0f;
    FLOAT fStartX = x;
    TCHAR c;

    // 정점 버퍼 채우기
    FONT3DVERTEX* pVertices;
    DWORD         dwNumTriangles = 0L;
    m_pVB->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD );

    while( (c = *strText++) != 0 )
    {
        if( c == '\n' )
        {
            x = fStartX;
            y -= (m_fTexCoords[0][3]-m_fTexCoords[0][1])*m_dwTexHeight/10.0f;
        }

        if( (c-32) < 0 || (c-32) >= 128-32 )
            continue;

        FLOAT tx1 = m_fTexCoords[c-32][0];
        FLOAT ty1 = m_fTexCoords[c-32][1];
        FLOAT tx2 = m_fTexCoords[c-32][2];
        FLOAT ty2 = m_fTexCoords[c-32][3];

        FLOAT w = (tx2-tx1) * m_dwTexWidth  / ( 10.0f * m_fTextScale );
        FLOAT h = (ty2-ty1) * m_dwTexHeight / ( 10.0f * m_fTextScale );

        if( c != _T(' ') )
        {
            *pVertices++ = InitFont3DVertex( D3DXVECTOR3(x+0,y+0,0), D3DXVECTOR3(0,0,-1), tx1, ty2 );
            *pVertices++ = InitFont3DVertex( D3DXVECTOR3(x+0,y+h,0), D3DXVECTOR3(0,0,-1), tx1, ty1 );
            *pVertices++ = InitFont3DVertex( D3DXVECTOR3(x+w,y+0,0), D3DXVECTOR3(0,0,-1), tx2, ty2 );
            *pVertices++ = InitFont3DVertex( D3DXVECTOR3(x+w,y+h,0), D3DXVECTOR3(0,0,-1), tx2, ty1 );
            *pVertices++ = InitFont3DVertex( D3DXVECTOR3(x+w,y+0,0), D3DXVECTOR3(0,0,-1), tx2, ty2 );
            *pVertices++ = InitFont3DVertex( D3DXVECTOR3(x+0,y+h,0), D3DXVECTOR3(0,0,-1), tx1, ty1 );
            dwNumTriangles += 2;

            if( dwNumTriangles*3 > (MAX_NUM_VERTICES-6) )
            {
                // 정점 버퍼 잠금 해제, 렌더링 및 다시 잠그기
                m_pVB->Unlock();
                m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, dwNumTriangles );
                m_pVB->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD );
                dwNumTriangles = 0L;
            }
        }

        x += w - (2 * m_dwSpacing) / 10.0f;
    }

    // 정점 버퍼 잠금 해제 및 렌더링
    m_pVB->Unlock();
    if( dwNumTriangles > 0 )
        m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, dwNumTriangles );

    // 수정된 렌더 상태를 복원합니다.
    m_pStateBlockSaved->Apply();

    return S_OK;
}