#ifndef D3DUTIL_H
#define D3DUTIL_H
#include <D3D9.h>
#include <D3DX9Math.h>

// D3DMATERIAL9 구조를 초기화하여 확산 및 주변 환경을 설정합니다.
// 색상. 방출 또는 반사 색상을 설정하지 않습니다.
VOID D3DUtil_InitMaterial( D3DMATERIAL9& mtrl, FLOAT r=0.0f, FLOAT g=0.0f, FLOAT b=0.0f, FLOAT a=1.0f );

//D3DLIGHT 구조를 초기화하여 조명 위치를 설정합니다. 그만큼 확산 색상은 흰색, 반사광 및 주변 색상은 검정색으로 설정됩니다.
VOID D3DUtil_InitLight( D3DLIGHT9& light, D3DLIGHTTYPE ltType, FLOAT x=0.0f, FLOAT y=0.0f, FLOAT z=0.0f );

//// 설명: 텍스처를 생성하는 도우미 함수입니다. 먼저 루트 경로를 확인하고, 그런 다음 DXSDK 미디어 경로(시스템 레지스트리에 지정된 대로)를 시도합니다.
HRESULT D3DUtil_CreateTexture( LPDIRECT3DDEVICE9 pd3dDevice, TCHAR* strTexture, LPDIRECT3DTEXTURE9* ppTexture, D3DFORMAT d3dFormat = D3DFMT_UNKNOWN );

//큐브맵의 면에 렌더링하기 위한 뷰 매트릭스를 반환합니다.
D3DXMATRIX D3DUtil_GetCubeMapViewMatrix( DWORD dwFace );

//창 커서에 의해 암시된 회전에 대한 쿼터니언을 반환합니다.
//위치.
D3DXQUATERNION D3DUtil_GetRotationFromCursor( HWND hWnd,
                                              FLOAT fTrackBallRadius=1.0f );
//hCursor를 기반으로 D3D 장치에 대한 커서를 빌드하고 설정합니다.
HRESULT D3DUtil_SetDeviceCursor( LPDIRECT3DDEVICE9 pd3dDevice, HCURSOR hCursor, BOOL bAddWatermark );

//주어진 D3DFORMAT에 대한 문자열을 반환합니다.
//bWithPrefix는 문자열에 "D3DFMT_"가 포함되어야 하는지 여부를 결정합니다.
TCHAR* D3DUtil_D3DFormatToString( D3DFORMAT format, bool bWithPrefix = true );

class CD3DArcBall
{
    INT            m_iWidth;   // ArcBall의 창 너비
    INT            m_iHeight;  // ArcBall 창 높이
    FLOAT          m_fRadius;  // 화면 좌표로 표시된 ArcBall의 반경
    FLOAT          m_fRadiusTranslation;    // 타겟을 이동하기 위한 ArcBall의 반경

    D3DXQUATERNION m_qDown;    // 버튼을 누르기 전의 쿼터니언
    D3DXQUATERNION m_qNow;               // 현재 드래그에 대한 복합 쿼터니언
    D3DXMATRIX  m_matRotation;        // 원호구 방향에 대한 행렬
    D3DXMATRIX  m_matRotationDelta;    // 원호구 방향에 대한 행렬
    D3DXMATRIX  m_matTranslation;    // 원호구의 위치에 대한 행렬
    D3DXMATRIX  m_matTranslationDelta;    // 원호구의 위치에 대한 행렬
    BOOL           m_bDrag;               // 사용자가 아크볼을 드래그하고 있는지 여부
    BOOL           m_bRightHanded;    // RH 좌표계 사용 여부

    D3DXVECTOR3 ScreenToVector( int sx, int sy );

public:
    LRESULT     HandleMouseMessages( HWND, UINT, WPARAM, LPARAM );

    D3DXMATRIX* GetRotationMatrix()         { return &m_matRotation; }
    D3DXMATRIX* GetRotationDeltaMatrix()    { return &m_matRotationDelta; }
    D3DXMATRIX* GetTranslationMatrix()      { return &m_matTranslation; }
    D3DXMATRIX* GetTranslationDeltaMatrix() { return &m_matTranslationDelta; }
    BOOL        IsBeingDragged()            { return m_bDrag; }

    VOID        SetRadius( FLOAT fRadius );
    VOID        SetWindow( INT w, INT h, FLOAT r=0.9 );
    VOID        SetRightHanded( BOOL bRightHanded ) { m_bRightHanded = bRightHanded; }

                CD3DArcBall();
    VOID        Init();
};

class CD3DCamera
{
    D3DXVECTOR3 m_vEyePt;    // 뷰 매트릭스의 속성
    D3DXVECTOR3 m_vLookatPt;
    D3DXVECTOR3 m_vUpVec;

    D3DXVECTOR3 m_vView;
    D3DXVECTOR3 m_vCross;

    D3DXMATRIX  m_matView;
    D3DXMATRIX  m_matBillboard; // 빌보드 효과를 위한 특수 매트릭스

    FLOAT       m_fFOV;    // 투영 행렬의 속성
    FLOAT       m_fAspect;
    FLOAT       m_fNearPlane;
    FLOAT       m_fFarPlane;
    D3DXMATRIX  m_matProj;

public:
    // Access functions
    D3DXVECTOR3 GetEyePt()           { return m_vEyePt; }
    D3DXVECTOR3 GetLookatPt()        { return m_vLookatPt; }
    D3DXVECTOR3 GetUpVec()           { return m_vUpVec; }
    D3DXVECTOR3 GetViewDir()         { return m_vView; }
    D3DXVECTOR3 GetCross()           { return m_vCross; }

    FLOAT       GetFOV()             { return m_fFOV; }
    FLOAT       GetAspect()          { return m_fAspect; }
    FLOAT       GetNearPlane()       { return m_fNearPlane; }
    FLOAT       GetFarPlane()        { return m_fFarPlane; }

    D3DXMATRIX  GetViewMatrix()      { return m_matView; }
    D3DXMATRIX  GetBillboardMatrix() { return m_matBillboard; }
    D3DXMATRIX  GetProjMatrix()      { return m_matProj; }

    VOID SetViewParams( D3DXVECTOR3 &vEyePt, D3DXVECTOR3& vLookatPt,
                        D3DXVECTOR3& vUpVec );
    VOID SetProjParams( FLOAT fFOV, FLOAT fAspect, FLOAT fNearPlane,
                        FLOAT fFarPlane );

    CD3DCamera();
};
#endif // D3DUTIL_H
