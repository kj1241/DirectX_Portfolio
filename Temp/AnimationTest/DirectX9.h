#pragma once
#include "DirectXPipline.h"
#include "Camera.h"

class DirectX9:public DirectXPipline
{

public:

    DirectX9();
    virtual ~DirectX9();

    bool OnInit(HWND hWnd) override;
    void OnUpdate() override;
    void OnRender() override;
    void OnDestroy() override;

private:
    bool InitDevice(HWND hWnd);
    bool InitGeometry();
    void InitMatrix();
    void Animate();
    void SetupLights();

    Camera* pCamera;

    LPDIRECT3D9 pD3D = nullptr; /// D3D 디바이스를 생성할 D3D객체변수
    LPDIRECT3DDEVICE9 pD3dDevice = nullptr; /// 렌더링에 사용될 D3D디바이스

    D3DXMATRIXA16 matWorld; //매트릭스 월드좌표
    D3DXMATRIXA16 matView; //매트릭스 뷰좌표
    D3DXMATRIXA16 matProj; //매트릭스 프로젝트좌표

    bool bWireframe = false; //그리기유형

};