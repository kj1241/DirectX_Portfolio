#pragma once

//해야할 일 나중에 사원수로 바꿔야됨.. 왜케 귀찮지
class Camera
{
public:
	Camera();
	~Camera();

	D3DXMATRIXA16* GetViewMatrix();// 카메라 행렬
	D3DXMATRIXA16* GetBillMatrix();// 빌보드 행렬
	D3DXMATRIXA16* SetInitView(D3DXVECTOR3& pvEye, D3DXVECTOR3& pvLookat, D3DXVECTOR3& pvUp); //카메라 행렬을 생성하기 위한 백터값초기화
	
	//카메라의 위치
	void SetPosition(D3DXVECTOR3& pv);
	D3DXVECTOR3* GetPosition();

	//카메라의 시선 위치
	void SeLookatPosition(D3DXVECTOR3& pv);
	D3DXVECTOR3* GeLookatPosition();







	///// 카메라의 상방벡터값을 설정한다.
	//void			SetUp(D3DXVECTOR3* pv) { m_vUp = *pv; }

	///// 카메라의 상방벡터값을 얻어낸다.
	//D3DXVECTOR3* GetUp() { return &m_vUp; }

	///// 값을 갱신한다.
	//void			Flush() { SetView(&m_vEye, &m_vLookat, &m_vUp); }

	///// 카메라 좌표계의 X축으로 angle만큼 회전한다.
	//D3DXMATRIXA16* RotateLocalX(float angle);

	///// 카메라 좌표계의 Y축으로 angle만큼 회전한다.
	//D3DXMATRIXA16* RotateLocalY(float angle);

	////	D3DXMATRIXA16*	RotateLocalZ( float angle );

	//	/// 월드좌표계의 *pv값의 위치로 이동한다.
	//D3DXMATRIXA16* MoveTo(D3DXVECTOR3* pv);

	///// 카메라 좌표계의 X축방향으로 dist만큼 전진한다.(후진은 -dist를 넣으면 된다.)
	//D3DXMATRIXA16* MoveLocalX(float dist);

	///// 카메라 좌표계의 Y축방향으로 dist만큼 전진한다.(후진은 -dist를 넣으면 된다.)
	//D3DXMATRIXA16* MoveLocalY(float dist);

	///// 카메라 좌표계의 Z축방향으로 dist만큼 전진한다.(후진은 -dist를 넣으면 된다.)
	//D3DXMATRIXA16* MoveLocalZ(float dist);



private:

	D3DXVECTOR3 vecEye;			/// 카메라의 현재 위치
	D3DXVECTOR3 vecLookat;		/// 카메라의 시선 위치
	D3DXVECTOR3 vecUp;			/// 카메라의 상방벡터

	D3DXVECTOR3 vView;		/// 카메라가 향하는 단위방향벡터
	D3DXVECTOR3 vCross;		/// 카마레의 측면벡터 cross( view, up )

	D3DXMATRIXA16 matView;		/// 카메라 행렬
	D3DXMATRIXA16 matBill;		/// 빌보드 행렬(카메라의 역행렬)
};
