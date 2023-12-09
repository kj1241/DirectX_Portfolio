#include "stdafx.h"
#include "Camera.h"

Camera::Camera()
{
}

Camera::~Camera()
{
}

D3DXMATRIXA16* Camera::GetViewMatrix()
{
	return &matView;
}

D3DXMATRIXA16* Camera::GetBillMatrix()
{
	return &matBill;
}

D3DXMATRIXA16* Camera::SetInitView(D3DXVECTOR3& vEye, D3DXVECTOR3& vLookat, D3DXVECTOR3& vUp)
{
	//�ʱⰪ
	vecEye = vEye;
	vecLookat = vLookat;
	vecUp = vUp;

	D3DXVECTOR3 tempView = vecLookat - vecEye; //error C2102
	D3DXVec3Normalize(&vView, &(tempView));
	D3DXVec3Cross(&vCross, &vecUp, &vView);

	D3DXMatrixLookAtLH(&matView, &vecEye, &vecLookat, &vecUp);
	D3DXMatrixInverse(&matBill, NULL, &matView);
	
	//4*3
	matBill._41 = 0.0f;
	matBill._42 = 0.0f;
	matBill._43 = 0.0f;

	return &matView;
}

void Camera::SetPosition(D3DXVECTOR3& pv)
{
	vecEye = pv;
}

D3DXVECTOR3* Camera::GetPosition()
{
	return &vecEye;
}

void Camera::SeLookatPosition(D3DXVECTOR3& pv)
{
	vecLookat = pv;
}

D3DXVECTOR3* Camera::GeLookatPosition()
{
	return &vecLookat;
}
