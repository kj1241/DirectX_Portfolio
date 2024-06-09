vs_1_1
dcl_position v0
dcl_normal v3
; Input:
;
; v0.xyz - position
; v3.xyz - normal mapped -1.f -> -1.f, 1.f -> 1.f
; c0-c3 - world view projection matrix
; c4 - <0.5.f, 0.f, 0, 0>
m4x4 oPos, v0, c0					; transform position
mad oD0.rgb, v3.xyz, c4.x, c4.x		; use normal data as colour

//작업을 수행:
//1. 버텍스의 위치와 노멀 데이터를 입력으로 받습니다.
//2. 버텍스의 위치를 월드 뷰 프로젝션 행렬로 변환하여 출력 위치를 계산합니다.
//3. 버텍스의 노멀 데이터를 색상으로 사용합니다.

//동작을 수행:
//dcl_position v0: 버텍스의 위치 데이터가 v0 레지스터에 입력됩니다.
//dcl_normal v3: 버텍스의 노멀 데이터가 v3 레지스터에 입력됩니다.
//m4x4 oPos, v0, c0: 버텍스의 위치 데이터(v0)를 월드 뷰 프로젝션 행렬(c0)로 변환하여 출력 위치(oPos)를 계산합니다.
//mad oD0.rgb, v3.xyz, c4.x, c4.x: 버텍스의 노멀 데이터(v3.xyz)를 색상 값으로 사용합니다. 노멀 데이터의 x, y, z 값에 상수(c4.x)를 곱하여 색상으로 사용하고, 결과를 oD0.rgb에 저장합니다.