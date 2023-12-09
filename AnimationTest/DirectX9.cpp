#include "stdafx.h"
#include "DirectX9.h"

DirectX9::DirectX9()
{
    pCamera = new Camera;
}

DirectX9::~DirectX9()
{
    if (!pCamera)
        delete pCamera;
}

bool DirectX9::OnInit(HWND hWnd)
{
    if (!InitDevice(hWnd))
        return false;

    if (!InitGeometry())
        return false;
}

void DirectX9::OnUpdate()
{
}

void DirectX9::OnRender()
{
    if (NULL == pD3dDevice)
        return;

    /// 후면버퍼와 버퍼의 초기화 
    //pD3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0); //dx 튜토리얼에서는 파란색으로
    pD3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(200, 200, 200), 1.0f, 0);
    pD3dDevice->SetRenderState(D3DRS_FILLMODE, bWireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID); //그리기 유형 경고는 내가 어떻게 할수 있는 부분이 아닌데?

    Animate(); //에니메이션

    /// 렌더링 시작
    if (SUCCEEDED(pD3dDevice->BeginScene()))
    {
        /// 실제 렌더링 명령들이 나열될 곳

        /// 렌더링 종료
        pD3dDevice->EndScene();
    }

    /// 후면버퍼를 보이는 화면으로!
    pD3dDevice->Present(NULL, NULL, NULL, NULL);
}

void DirectX9::OnDestroy()
{
    if (pD3dDevice != NULL)
        pD3dDevice->Release();

    if (pD3D != NULL)
        pD3D->Release();
}

bool DirectX9::InitDevice(HWND hWnd)
{
    /// 디바이스를 생성하기위한 D3D객체 생성
    if (NULL == (pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
        return false;

    // 디바이스를 생성할 구조체
    // 복잡한 오브젝트를 그릴것이기때문에, 이번에는 Z버퍼가 필요하다.
    D3DPRESENT_PARAMETERS d3dpp;// 디바이스 생성을 위한 구조체
    ZeroMemory(&d3dpp, sizeof(d3dpp)); // 반드시 ZeroMemory()함수로 미리 구조체를 깨끗이 지워야 한다.
    d3dpp.Windowed = TRUE; // 창모드로 생성
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;// 가장 효율적인 SWAP효과
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;// 현재 바탕화면 모드에 맞춰서 후면버퍼를 생성
    d3dpp.EnableAutoDepthStencil = TRUE; //스텐실 버퍼
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16; // z 버퍼



    /// 디바이스를 다음과 같은 설정으로 생성한다.
    /// 1. 디폴트 비디오카드를 사용(대부분은 비디오카드가 1개 이다.)
    /// 2. HAL디바이스를 생성한다.(HW가속장치를 사용하겠다는 의미)
    /// 3. 정점처리는 모든 카드에서 지원하는 SW처리로 생성한다.(HW로 생성할경우 더욱 높은 성능을 낸다.)
    if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pD3dDevice)))
    {
        return false;
    }

    /// 디바이스 상태정보를 처리할경우 여기에서 한다.
    pD3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW); // 기본컬링, CCW
    pD3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE); //Z버퍼기능을 켠다.

    return true;
}

// 지오메트릭스 원형 이거 directx12에서는 쉐이더로 바뀌어버리는데 ....
bool DirectX9::InitGeometry()
{
    InitMatrix();
    return true;
}

// 행렬 설정
void DirectX9::InitMatrix()
{
    // 월드 행렬 설정
    D3DXMatrixIdentity(&matWorld);
    pD3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    // 뷰 행렬을 설정 (시작카메라)
    D3DXVECTOR3 vEyePt(0.0f, 150.0f, -(float)250.0f);
    D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
    D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
    pD3dDevice->SetTransform(D3DTS_VIEW, &matView);

    // 실제 프로젝션 행렬
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 1000.0f);
    pD3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 200.0f); // 프러스텀 컬링용 프로젝션 행렬

    pCamera->SetInitView(vEyePt, vLookatPt, vUpVec); // 카메라 초기화
}

void DirectX9::Animate()
{
    SetupLights();
}

void DirectX9::SetupLights()
{
    D3DXVECTOR3 vecDir; // 방향성 광원(directional light)이 향할 빛의 방향
    D3DLIGHT9 light; // 광원 구조체

    ZeroMemory(&light, sizeof(D3DLIGHT9));	// 구조체 초기화
    light.Type = D3DLIGHT_DIRECTIONAL; // 빛의 종류(점, 방향, 스포트라이트
    
        // 광원의 색깔과 밝기
    light.Diffuse.r = 1.0f;
    light.Diffuse.g = 1.0f;
    light.Diffuse.b = 1.0f;

    vecDir = D3DXVECTOR3(0, -1, 0);
    D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecDir);// 빛의 방향

    light.Range = 1000.0f; // 광원이 거리

    pD3dDevice->SetLight(0, &light); // 디바이스에 0번 빛 설치
    pD3dDevice->LightEnable(0, TRUE); // 0번 빛 on
    pD3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE); // 빛설정 on
    pD3dDevice->SetRenderState(D3DRS_AMBIENT, 0x00a0a0a0); // 환경광원(ambient light)의 값 설정
}

