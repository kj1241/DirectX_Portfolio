#ifndef D3DFILE_H
#define D3DFILE_H
#include <tchar.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dxfile.h>

class CD3DMesh
{
public:
    TCHAR               m_strName[512];

    LPD3DXMESH          m_pSysMemMesh;    // SysMem 메시, 크기 조정을 통해 유지
    LPD3DXMESH          m_pLocalMesh;    // 로컬 메시, 크기 조정 시 다시 작성됨
    
    DWORD               m_dwNumMaterials;    // 메쉬 재료
    D3DMATERIAL9*       m_pMaterials;
    LPDIRECT3DTEXTURE9* m_pTextures;
    bool                m_bUseMaterials;

public:
    // 렌더링
    HRESULT Render( LPDIRECT3DDEVICE9 pd3dDevice,bool bDrawOpaqueSubsets = true, bool bDrawAlphaSubsets = true );

    // 메시 접근
    LPD3DXMESH GetSysMemMesh() { return m_pSysMemMesh; }
    LPD3DXMESH GetLocalMesh()  { return m_pLocalMesh; }

    // 렌더링 옵션
    void    UseMeshMaterials( bool bFlag ) { m_bUseMaterials = bFlag; }
    HRESULT SetFVF( LPDIRECT3DDEVICE9 pd3dDevice, DWORD dwFVF );

    // 초기화 중
    HRESULT RestoreDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice );
    HRESULT InvalidateDeviceObjects();

    // 생성/파괴
    HRESULT Create( LPDIRECT3DDEVICE9 pd3dDevice, TCHAR* strFilename );
    HRESULT Create( LPDIRECT3DDEVICE9 pd3dDevice, LPDIRECTXFILEDATA pFileData );
    HRESULT Destroy();

    CD3DMesh( TCHAR* strName = _T("CD3DFile_Mesh") );
    virtual ~CD3DMesh();
};

//파일 기반 메시를 로드하고 렌더링하기 위한 클래스
class CD3DFrame
{
public:
    TCHAR      m_strName[512];
    D3DXMATRIX m_mat;
    CD3DMesh*  m_pMesh;

    CD3DFrame* m_pNext;
    CD3DFrame* m_pChild;

public:
    // Matrix access
    void        SetMatrix( D3DXMATRIX* pmat ) { m_mat = *pmat; }
    D3DXMATRIX* GetMatrix()                   { return &m_mat; }

    CD3DMesh*   FindMesh( TCHAR* strMeshName );
    CD3DFrame*  FindFrame( TCHAR* strFrameName );
    bool        EnumMeshes( bool (*EnumMeshCB)(CD3DMesh*,void*),void* pContext );

    HRESULT Destroy();
    HRESULT RestoreDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice );
    HRESULT InvalidateDeviceObjects();
    HRESULT Render( LPDIRECT3DDEVICE9 pd3dDevice, bool bDrawOpaqueSubsets = true, bool bDrawAlphaSubsets = true, D3DXMATRIX* pmatWorldMartix = NULL);
    
    CD3DFrame( TCHAR* strName = _T("CD3DFile_Frame") );
    virtual ~CD3DFrame();
};

class CD3DFile : public CD3DFrame
{
    HRESULT LoadMesh( LPDIRECT3DDEVICE9 pd3dDevice, LPDIRECTXFILEDATA pFileData, 
                      CD3DFrame* pParentFrame );
    HRESULT LoadFrame( LPDIRECT3DDEVICE9 pd3dDevice, LPDIRECTXFILEDATA pFileData, 
                       CD3DFrame* pParentFrame );
public:
    HRESULT Create( LPDIRECT3DDEVICE9 pd3dDevice, TCHAR* strFilename );
    HRESULT CreateFromResource( LPDIRECT3DDEVICE9 pd3dDevice, TCHAR* strResource, TCHAR* strType );

    // 순수 장치의 경우 월드 변환을 지정합니다. 월드 트랜스폼이 아닌 경우
    // 순수 장치에 지정하면 이 함수는 실패합니다.
    HRESULT Render( LPDIRECT3DDEVICE9 pd3dDevice, D3DXMATRIX* pmatWorldMatrix = NULL );

    CD3DFile() : CD3DFrame( _T("CD3DFile_Root") ) {}
};
#endif