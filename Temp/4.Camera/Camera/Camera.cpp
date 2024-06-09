#include "stdafx.h"
#include "Camera.h"

Camera::Camera()
{
}

Camera::Camera(DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 target, DirectX::XMFLOAT4 up, DirectX::XMFLOAT4 right):
	position(position), target(target), up(up), right(right)
{
}

Camera::~Camera()
{
}

void Camera::SetIsOrthographic(bool isOrthographic)
{
	this->isOrthographic = isOrthographic;
}

void Camera::SetView(float width, float heigth)
{
	this->width = width;
	this->heigth = heigth;
}

void Camera::SetNearZ(float nearZ)
{
	this->nearZ = nearZ;
}

void Camera::SetFarZ(float farZ)
{
	this->farZ = farZ;
}

void Camera::SetFOV(float fov)
{
	this->fov = fov;
}

DirectX::XMMATRIX Camera::GetVPMatrix()
{
	DirectX::XMMATRIX result;
	result = DirectX::XMMatrixLookToLH(DirectX::XMLoadFloat4(&position), DirectX::XMLoadFloat4(&target), DirectX::XMLoadFloat4(&up))* DirectX::XMLoadFloat4x4(&transformationMatrix)* GetProjectionMatrix();

	return result;
}

DirectX::XMMATRIX Camera::GetProjectionMatrix()
{
	DirectX::XMMATRIX result;
	if (isOrthographic)
		result = DirectX::XMMatrixOrthographicLH(width, heigth, nearZ, farZ);
	else
		result= DirectX::XMMatrixPerspectiveFovLH(fov * (3.14f / 180.0f), width / heigth, nearZ, farZ);

	return result;
}

