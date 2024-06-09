//DirectX .X 파일을 로드하기 위한 지원 코드입니다.
#define STRICT
#include <tchar.h>
#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dxfile.h>
#include <rmxfguid.h>
#include <rmxftmpl.h>
#include "D3DFile.h"
#include "DXUtil.h"

CD3DMesh::CD3DMesh( TCHAR* strName )
{
    _tcsncpy( m_strName, strName, sizeof(m_strName) / sizeof(TCHAR) );
    m_strName[sizeof(m_strName) / sizeof(TCHAR) - 1] = _T('\0');
    m_pSysMemMesh        = NULL;
    m_pLocalMesh         = NULL;
    m_dwNumMaterials     = 0L;
    m_pMaterials         = NULL;
    m_pTextures          = NULL;
    m_bUseMaterials      = TRUE;
}

CD3DMesh::~CD3DMesh()
{
    Destroy();
}

HRESULT CD3DMesh::Create(LPDIRECT3DDEVICE9 pd3dDevice, TCHAR* strFilename)
{
    TCHAR        strPath[MAX_PATH];
    LPD3DXBUFFER pAdjacencyBuffer = NULL;
    LPD3DXBUFFER pMtrlBuffer = NULL;
    HRESULT      hr;

    // 파일 경로를 찾아 ANSI로 변환합니다(D3DX API용).
    DXUtil_FindMediaFileCb(strPath, sizeof(strPath), strFilename);

    // 메쉬 로드
    hr = D3DXLoadMeshFromX(strPath, D3DXMESH_SYSTEMMEM, pd3dDevice, &pAdjacencyBuffer, &pMtrlBuffer, NULL, &m_dwNumMaterials, &m_pSysMemMesh);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pAdjacencyBuffer);
        SAFE_RELEASE(pMtrlBuffer);
        return hr;
    }

    // 성능을 위해 메시를 최적화합니다.
    hr = m_pSysMemMesh->OptimizeInplace(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE, (DWORD*)pAdjacencyBuffer->GetBufferPointer(), NULL, NULL, NULL);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pAdjacencyBuffer);
        SAFE_RELEASE(pMtrlBuffer);
        return hr;
    }

    // 메쉬에 대한 머티리얼 정보를 얻습니다.
    // 버퍼에서 재료 배열을 가져옵니다.
    if (pMtrlBuffer && m_dwNumMaterials > 0)
    {
        // 머티리얼과 텍스처에 메모리를 할당합니다.
        D3DXMATERIAL* d3dxMtrls = (D3DXMATERIAL*)pMtrlBuffer->GetBufferPointer();
        m_pMaterials = new D3DMATERIAL9[m_dwNumMaterials];
        if (!m_pMaterials)
        {
            hr = E_OUTOFMEMORY;
            SAFE_RELEASE(pAdjacencyBuffer);
            SAFE_RELEASE(pMtrlBuffer);
            return hr;
        }

        m_pTextures = new LPDIRECT3DTEXTURE9[m_dwNumMaterials];
        if (!m_pTextures)
        {
            hr = E_OUTOFMEMORY;
            SAFE_RELEASE(pAdjacencyBuffer);
            SAFE_RELEASE(pMtrlBuffer);
            delete[] m_pMaterials;
            m_pMaterials = NULL;
            return hr;
        }

        // 각 재료를 복사하고 텍스처를 만듭니다.
        for (DWORD i = 0; i < m_dwNumMaterials; i++)
        {
            // 자료 복사
            m_pMaterials[i] = d3dxMtrls[i].MatD3D;
            m_pTextures[i] = NULL;

            // 텍스처 생성
            if (d3dxMtrls[i].pTextureFilename)
            {
                TCHAR strTexture[MAX_PATH];
                TCHAR strTextureTemp[MAX_PATH];
                DXUtil_ConvertAnsiStringToGenericCb(strTextureTemp, d3dxMtrls[i].pTextureFilename, sizeof(strTextureTemp));
                DXUtil_FindMediaFileCb(strTexture, sizeof(strTexture), strTextureTemp);

                if (FAILED(D3DXCreateTextureFromFile(pd3dDevice, strTexture, &m_pTextures[i])))
                {
                    m_pTextures[i] = NULL;
                }
            }
        }
    }

    SAFE_RELEASE(pAdjacencyBuffer);
    SAFE_RELEASE(pMtrlBuffer);

    return S_OK;
}

HRESULT CD3DMesh::Create(LPDIRECT3DDEVICE9 pd3dDevice, LPDIRECTXFILEDATA pFileData)
{
    LPD3DXBUFFER pMtrlBuffer = NULL;
    LPD3DXBUFFER pAdjacencyBuffer = NULL;
    HRESULT hr = E_FAIL; // 초기화

    // DXFILEDATA 개체로부터 메시 로드
    hr = D3DXLoadMeshFromXof((LPD3DXFILEDATA)pFileData, D3DXMESH_SYSTEMMEM, pd3dDevice, &pAdjacencyBuffer, &pMtrlBuffer, NULL, &m_dwNumMaterials, &m_pSysMemMesh);
    if (FAILED(hr))
    {
        // 실패 시 메모리 해제 후 종료
        SAFE_RELEASE(pAdjacencyBuffer);
        SAFE_RELEASE(pMtrlBuffer);
        return hr;
    }

    // 성능 최적화를 위해 메시 최적화
    hr = m_pSysMemMesh->OptimizeInplace(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE, (DWORD*)pAdjacencyBuffer->GetBufferPointer(), NULL, NULL, NULL);
    if (FAILED(hr))
    {
        // 실패 시 메모리 해제 후 종료
        SAFE_RELEASE(pAdjacencyBuffer);
        SAFE_RELEASE(pMtrlBuffer);
        return hr;
    }

    // 메시의 머티리얼 정보를 얻습니다.
    // 버퍼에서 머티리얼 배열을 가져옵니다.
    if (pMtrlBuffer && m_dwNumMaterials > 0)
    {
        // 머티리얼 및 텍스처에 메모리를 할당합니다.
        D3DXMATERIAL* d3dxMtrls = (D3DXMATERIAL*)pMtrlBuffer->GetBufferPointer();
        m_pMaterials = new D3DMATERIAL9[m_dwNumMaterials];
        m_pTextures = new LPDIRECT3DTEXTURE9[m_dwNumMaterials];
        if (!m_pMaterials || !m_pTextures)
        {
            // 할당 실패 시 메모리 해제 후 종료
            hr = E_OUTOFMEMORY;
            SAFE_RELEASE(pAdjacencyBuffer);
            SAFE_RELEASE(pMtrlBuffer);
            return hr;
        }

        // 각 머티리얼을 복사하고 해당 텍스처를 만듭니다.
        for (DWORD i = 0; i < m_dwNumMaterials; i++)
        {
            // 머티리얼 복사
            m_pMaterials[i] = d3dxMtrls[i].MatD3D;
            m_pTextures[i] = NULL;

            // 텍스처 생성
            if (d3dxMtrls[i].pTextureFilename)
            {
                TCHAR strTexture[MAX_PATH];
                TCHAR strTextureTemp[MAX_PATH];
                DXUtil_ConvertAnsiStringToGenericCb(strTextureTemp, d3dxMtrls[i].pTextureFilename, sizeof(strTextureTemp));
                DXUtil_FindMediaFileCb(strTexture, sizeof(strTexture), strTextureTemp);

                if (FAILED(D3DXCreateTextureFromFile(pd3dDevice, strTexture, &m_pTextures[i])))
                {
                    // 텍스처 생성 실패 시 NULL로 설정
                    m_pTextures[i] = NULL;
                }
            }
        }
    }

    // 정상적인 경우 성공 코드 반환
    hr = S_OK;

    // 메모리 해제 후 종료
    SAFE_RELEASE(pAdjacencyBuffer);
    SAFE_RELEASE(pMtrlBuffer);

    return hr;
}

HRESULT CD3DMesh::SetFVF( LPDIRECT3DDEVICE9 pd3dDevice, DWORD dwFVF )
{
    LPD3DXMESH pTempSysMemMesh = NULL;
    LPD3DXMESH pTempLocalMesh  = NULL;

    if( m_pSysMemMesh )
    {
        if( FAILED( m_pSysMemMesh->CloneMeshFVF( D3DXMESH_SYSTEMMEM, dwFVF, pd3dDevice, &pTempSysMemMesh ) ) )
            return E_FAIL;
    }
    if( m_pLocalMesh )
    {
        if( FAILED( m_pLocalMesh->CloneMeshFVF( 0L, dwFVF, pd3dDevice, &pTempLocalMesh ) ) )
        {
            SAFE_RELEASE( pTempSysMemMesh );
            return E_FAIL;
        }
    }

    SAFE_RELEASE( m_pSysMemMesh );
    SAFE_RELEASE( m_pLocalMesh );

    if( pTempSysMemMesh ) m_pSysMemMesh = pTempSysMemMesh;
    if( pTempLocalMesh )  m_pLocalMesh  = pTempLocalMesh;

    // 메시에 법선이 있는 경우 법선을 계산합니다.
    if( m_pSysMemMesh )
        D3DXComputeNormals( m_pSysMemMesh, NULL );
    if( m_pLocalMesh )
        D3DXComputeNormals( m_pLocalMesh, NULL );

    return S_OK;
}

HRESULT CD3DMesh::RestoreDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice )
{
    if( NULL == m_pSysMemMesh )
        return E_FAIL;


    // 메시의 로컬 메모리 버전을 만듭니다. 참고: 우리가 지나가고 있기 때문에
    // 플래그가 없습니다. 기본 동작은 로컬 메모리에 복제하는 것입니다.
    if( FAILED( m_pSysMemMesh->CloneMeshFVF( 0L, m_pSysMemMesh->GetFVF(),pd3dDevice, &m_pLocalMesh ) ) )
        return E_FAIL;

    return S_OK;
}

HRESULT CD3DMesh::InvalidateDeviceObjects()
{
    SAFE_RELEASE( m_pLocalMesh );

    return S_OK;
}

HRESULT CD3DMesh::Destroy()
{
    InvalidateDeviceObjects();
    for( UINT i=0; i<m_dwNumMaterials; i++ )
        SAFE_RELEASE( m_pTextures[i] );
    SAFE_DELETE_ARRAY( m_pTextures );
    SAFE_DELETE_ARRAY( m_pMaterials );

    SAFE_RELEASE( m_pSysMemMesh );

    m_dwNumMaterials = 0L;

    return S_OK;
}

HRESULT CD3DMesh::Render( LPDIRECT3DDEVICE9 pd3dDevice, bool bDrawOpaqueSubsets, bool bDrawAlphaSubsets )
{
    if( NULL == m_pLocalMesh )
        return E_FAIL;

    // 먼저, 알파 없이 부분 집합을 그립니다.
    if( bDrawOpaqueSubsets )
    {
        for( DWORD i=0; i<m_dwNumMaterials; i++ )
        {
            if( m_bUseMaterials )
            {
                if( m_pMaterials[i].Diffuse.a < 1.0f )
                    continue;
                pd3dDevice->SetMaterial( &m_pMaterials[i] );
                pd3dDevice->SetTexture( 0, m_pTextures[i] );
            }
            m_pLocalMesh->DrawSubset( i );
        }
    }

    // 그런 다음 알파를 사용하여 하위 집합을 그립니다.
    if( bDrawAlphaSubsets && m_bUseMaterials )
    {
        for( DWORD i=0; i<m_dwNumMaterials; i++ )
        {
            if( m_pMaterials[i].Diffuse.a == 1.0f )
                continue;

            // 재질과 질감을 설정합니다.
            pd3dDevice->SetMaterial( &m_pMaterials[i] );
            pd3dDevice->SetTexture( 0, m_pTextures[i] );
            m_pLocalMesh->DrawSubset( i );
        }
    }

    return S_OK;
}

CD3DFrame::CD3DFrame( TCHAR* strName )
{
    _tcsncpy( m_strName, strName, sizeof(m_strName) / sizeof(TCHAR) );
    m_strName[sizeof(m_strName) / sizeof(TCHAR) - 1] = _T('\0');
    D3DXMatrixIdentity( &m_mat );
    m_pMesh  = NULL;

    m_pChild = NULL;
    m_pNext  = NULL;
}

CD3DFrame::~CD3DFrame()
{
    SAFE_DELETE( m_pChild );
    SAFE_DELETE( m_pNext );
}

bool CD3DFrame::EnumMeshes( bool (*EnumMeshCB)(CD3DMesh*,void*),
                            void* pContext )
{
    if( m_pMesh )
        EnumMeshCB( m_pMesh, pContext );
    if( m_pChild )
        m_pChild->EnumMeshes( EnumMeshCB, pContext );
    if( m_pNext )
        m_pNext->EnumMeshes( EnumMeshCB, pContext );

    return TRUE;
}

CD3DMesh* CD3DFrame::FindMesh( TCHAR* strMeshName )
{
    CD3DMesh* pMesh;

    if( m_pMesh )
        if( !lstrcmpi( m_pMesh->m_strName, strMeshName ) )
            return m_pMesh;

    if( m_pChild )
        if( NULL != ( pMesh = m_pChild->FindMesh( strMeshName ) ) )
            return pMesh;

    if( m_pNext )
        if( NULL != ( pMesh = m_pNext->FindMesh( strMeshName ) ) )
            return pMesh;

    return NULL;
}

CD3DFrame* CD3DFrame::FindFrame( TCHAR* strFrameName )
{
    CD3DFrame* pFrame;

    if( !lstrcmpi( m_strName, strFrameName ) )
        return this;

    if( m_pChild )
        if( NULL != ( pFrame = m_pChild->FindFrame( strFrameName ) ) )
            return pFrame;

    if( m_pNext )
        if( NULL != ( pFrame = m_pNext->FindFrame( strFrameName ) ) )
            return pFrame;

    return NULL;
}

HRESULT CD3DFrame::Destroy()
{
    if( m_pMesh )  m_pMesh->Destroy();
    if( m_pChild ) m_pChild->Destroy();
    if( m_pNext )  m_pNext->Destroy();

    SAFE_DELETE( m_pMesh );
    SAFE_DELETE( m_pNext );
    SAFE_DELETE( m_pChild );

    return S_OK;
}

HRESULT CD3DFrame::RestoreDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice )
{
    if( m_pMesh )  m_pMesh->RestoreDeviceObjects( pd3dDevice );
    if( m_pChild ) m_pChild->RestoreDeviceObjects( pd3dDevice );
    if( m_pNext )  m_pNext->RestoreDeviceObjects( pd3dDevice );
    return S_OK;
}

HRESULT CD3DFrame::InvalidateDeviceObjects()
{
    if( m_pMesh )  m_pMesh->InvalidateDeviceObjects();
    if( m_pChild ) m_pChild->InvalidateDeviceObjects();
    if( m_pNext )  m_pNext->InvalidateDeviceObjects();
    return S_OK;
}

HRESULT CD3DFrame::Render( LPDIRECT3DDEVICE9 pd3dDevice, bool bDrawOpaqueSubsets, bool bDrawAlphaSubsets, D3DXMATRIX* pmatWorldMatrix )
{
    // 순수 장치의 경우 월드 변환을 지정합니다. 월드 트랜스폼이 아닌 경우
    // 순수 장치에 지정하면 이 함수는 실패합니다.

    D3DXMATRIXA16 matSavedWorld, matWorld;

    if ( NULL == pmatWorldMatrix )
        pd3dDevice->GetTransform( D3DTS_WORLD, &matSavedWorld );
    else
        matSavedWorld = *pmatWorldMatrix;

    D3DXMatrixMultiply( &matWorld, &m_mat, &matSavedWorld );
    pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    if( m_pMesh )
        m_pMesh->Render( pd3dDevice, bDrawOpaqueSubsets, bDrawAlphaSubsets );

    if( m_pChild )
        m_pChild->Render( pd3dDevice, bDrawOpaqueSubsets, bDrawAlphaSubsets, &matWorld );

    pd3dDevice->SetTransform( D3DTS_WORLD, &matSavedWorld );

    if( m_pNext )
        m_pNext->Render( pd3dDevice, bDrawOpaqueSubsets, bDrawAlphaSubsets, &matSavedWorld );

    return S_OK;
}

HRESULT CD3DFile::LoadFrame( LPDIRECT3DDEVICE9 pd3dDevice, LPDIRECTXFILEDATA pFileData, CD3DFrame* pParentFrame )
{
    LPDIRECTXFILEDATA   pChildData = NULL;
    LPDIRECTXFILEOBJECT pChildObj = NULL;
    const GUID* pGUID;
    DWORD       cbSize;
    CD3DFrame*  pCurrentFrame;
    HRESULT     hr;

    //객체의 유형을 가져옵니다.
    if( FAILED( hr = pFileData->GetType( &pGUID ) ) )
        return hr;

    if( *pGUID == TID_D3DRMMesh )
    {
        hr = LoadMesh( pd3dDevice, pFileData, pParentFrame );
        if( FAILED(hr) )
            return hr;
    }
    if( *pGUID == TID_D3DRMFrameTransformMatrix )
    {
        D3DXMATRIX* pmatMatrix;
        hr = pFileData->GetData( NULL, &cbSize, (void**)&pmatMatrix );
        if( FAILED(hr) )
            return hr;

        // 부모의 행렬을 새 행렬로 업데이트합니다.
        pParentFrame->SetMatrix( pmatMatrix );
    }
    if( *pGUID == TID_D3DRMFrame )
    {
        // 프레임 이름을 가져옵니다.
        CHAR  strAnsiName[512] = "";
        TCHAR strName[512];
        DWORD dwNameLength = 512;
        if( FAILED( hr = pFileData->GetName( strAnsiName, &dwNameLength ) ) )
            return hr;
        DXUtil_ConvertAnsiStringToGenericCb( strName, strAnsiName, sizeof(strName) );

        // 프레임 생성
        pCurrentFrame = new CD3DFrame( strName );
        if( pCurrentFrame == NULL )
            return E_OUTOFMEMORY;

        pCurrentFrame->m_pNext = pParentFrame->m_pChild;
        pParentFrame->m_pChild = pCurrentFrame;


        // 자식 객체 열거
        while( SUCCEEDED( pFileData->GetNextObject( &pChildObj ) ) )
        {
            // 자식에게 FileData를 쿼리합니다.
            hr = pChildObj->QueryInterface( IID_IDirectXFileData,
                                            (void**)&pChildData );
            if( SUCCEEDED(hr) )
            {
                hr = LoadFrame( pd3dDevice, pChildData, pCurrentFrame );
                pChildData->Release();
            }

            pChildObj->Release();

            if( FAILED(hr) )
                return hr;
        }
    }

    return S_OK;
}

HRESULT CD3DFile::LoadMesh( LPDIRECT3DDEVICE9 pd3dDevice, LPDIRECTXFILEDATA pFileData, CD3DFrame* pParentFrame )
{
    // 현재는 프레임당 메시 하나만 허용됩니다.
    if( pParentFrame->m_pMesh )
        return E_FAIL;

    // 메쉬 이름을 가져옵니다
    CHAR  strAnsiName[512] = {0};
    TCHAR strName[512];
    DWORD dwNameLength = 512;
    HRESULT hr;
    if( FAILED( hr = pFileData->GetName( strAnsiName, &dwNameLength ) ) )
        return hr;
    DXUtil_ConvertAnsiStringToGenericCb( strName, strAnsiName, sizeof(strName) );

    // 메쉬 생성
    pParentFrame->m_pMesh = new CD3DMesh( strName );
    if( pParentFrame->m_pMesh == NULL )
        return E_OUTOFMEMORY;
    pParentFrame->m_pMesh->Create( pd3dDevice, pFileData );

    return S_OK;
}

HRESULT CD3DFile::CreateFromResource( LPDIRECT3DDEVICE9 pd3dDevice, TCHAR* strResource, TCHAR* strType )
{
    LPDIRECTXFILE           pDXFile   = NULL;
    LPDIRECTXFILEENUMOBJECT pEnumObj  = NULL;
    LPDIRECTXFILEDATA       pFileData = NULL;
    HRESULT hr;

    // x 파일 객체 생성
    if( FAILED( hr = DirectXFileCreate( &pDXFile ) ) )
        return E_FAIL;

    // d3drm 및 패치 확장을 위한 템플릿을 등록합니다.
    if( FAILED( hr = pDXFile->RegisterTemplates( (void*)D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES ) ) )
    {
        pDXFile->Release();
        return E_FAIL;
    }
    
    CHAR strTypeAnsi[MAX_PATH];
    DXUtil_ConvertGenericStringToAnsiCb( strTypeAnsi, strType, sizeof(strTypeAnsi) );

    DXFILELOADRESOURCE dxlr;
    dxlr.hModule = NULL;
    dxlr.lpName = strResource;
    dxlr.lpType = (TCHAR*) strTypeAnsi;

    // 열거형 객체 생성
    hr = pDXFile->CreateEnumObject( (void*)&dxlr, DXFILELOAD_FROMRESOURCE, 
                                    &pEnumObj );
    if( FAILED(hr) )
    {
        pDXFile->Release();
        return hr;
    }

    // 최상위 객체(항상 프레임임)를 열거합니다.
    while( SUCCEEDED( pEnumObj->GetNextDataObject( &pFileData ) ) )
    {
        hr = LoadFrame( pd3dDevice, pFileData, this );
        pFileData->Release();
        if( FAILED(hr) )
        {
            pEnumObj->Release();
            pDXFile->Release();
            return E_FAIL;
        }
    }

    SAFE_RELEASE( pFileData );
    SAFE_RELEASE( pEnumObj );
    SAFE_RELEASE( pDXFile );

    return S_OK;
}

HRESULT CD3DFile::Create( LPDIRECT3DDEVICE9 pd3dDevice, TCHAR* strFilename )
{
    LPDIRECTXFILE           pDXFile   = NULL;
    LPDIRECTXFILEENUMOBJECT pEnumObj  = NULL;
    LPDIRECTXFILEDATA       pFileData = NULL;
    HRESULT hr;

    // x 파일 객체 생성
    if( FAILED( hr = DirectXFileCreate( &pDXFile ) ) )
        return E_FAIL;

    // d3drm 및 패치 확장을 위한 템플릿을 등록합니다.
    if( FAILED( hr = pDXFile->RegisterTemplates( (void*)D3DRM_XTEMPLATES,
                                                 D3DRM_XTEMPLATE_BYTES ) ) )
    {
        pDXFile->Release();
        return E_FAIL;
    }

    // 파일 경로를 찾아 ANSI로 변환합니다(D3DXOF API용).
    TCHAR strPath[MAX_PATH];
    CHAR  strPathANSI[MAX_PATH];
    DXUtil_FindMediaFileCb( strPath, sizeof(strPath), strFilename );
    DXUtil_ConvertGenericStringToAnsiCb( strPathANSI, strPath, sizeof(strPathANSI) );
    
    // 열거형 객체 생성
    hr = pDXFile->CreateEnumObject( (void*)strPathANSI, DXFILELOAD_FROMFILE, &pEnumObj );
    if( FAILED(hr) )
    {
        pDXFile->Release();
        return hr;
    }

    // 최상위 객체(항상 프레임임)를 열거합니다.
    while( SUCCEEDED( pEnumObj->GetNextDataObject( &pFileData ) ) )
    {
        hr = LoadFrame( pd3dDevice, pFileData, this );
        pFileData->Release();
        if( FAILED(hr) )
        {
            pEnumObj->Release();
            pDXFile->Release();
            return E_FAIL;
        }
    }

    SAFE_RELEASE( pFileData );
    SAFE_RELEASE( pEnumObj );
    SAFE_RELEASE( pDXFile );

    return S_OK;
}

HRESULT CD3DFile::Render( LPDIRECT3DDEVICE9 pd3dDevice, D3DXMATRIX* pmatWorldMatrix )
{

    // 순수 장치의 경우 월드 변환을 지정합니다. 월드 트랜스폼이 아닌 경우
    // 순수 장치에 지정하면 이 함수는 실패합니다.

    // 월드 변환 설정
    D3DXMATRIX matSavedWorld, matWorld;

    if ( NULL == pmatWorldMatrix )
        pd3dDevice->GetTransform( D3DTS_WORLD, &matSavedWorld );
    else
        matSavedWorld = *pmatWorldMatrix;

    D3DXMatrixMultiply( &matWorld, &matSavedWorld, &m_mat );
    pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // 메시의 불투명한 하위 집합을 렌더링합니다.
    if( m_pChild )
        m_pChild->Render( pd3dDevice, TRUE, FALSE, &matWorld );

    // 알파 블렌딩 활성화
    pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

    // 메시의 알파 하위 집합을 렌더링합니다.
    if( m_pChild )
        m_pChild->Render( pd3dDevice, FALSE, TRUE, &matWorld );

    // 상태 복원
    pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pd3dDevice->SetTransform( D3DTS_WORLD, &matSavedWorld );

    return S_OK;
}