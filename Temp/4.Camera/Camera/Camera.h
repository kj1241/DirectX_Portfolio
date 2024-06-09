#pragma once

class Camera 
{
public:
    Camera();
    Camera(DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 target, DirectX::XMFLOAT4 up, DirectX::XMFLOAT4 right);
    ~Camera();

    void SetIsOrthographic(bool isOrthographic);
    void SetView(float width, float heigth);
    void SetNearZ(float nearZ);
    void SetFarZ(float farZ);
    void SetFOV(float fov);

    DirectX::XMMATRIX GetVPMatrix(); //매트릭스
    DirectX::XMMATRIX GetProjectionMatrix(); //프로젝트 매트릭스

private:
    bool isOrthographic;// 직교인가?
    float width;
    float heigth;
    float nearZ;
    float farZ;
    float fov;

    DirectX::XMFLOAT4 position;
    DirectX::XMFLOAT4 target;
    DirectX::XMFLOAT4 up;
    DirectX::XMFLOAT4 right;
    DirectX::XMFLOAT4X4 transformationMatrix;

};
