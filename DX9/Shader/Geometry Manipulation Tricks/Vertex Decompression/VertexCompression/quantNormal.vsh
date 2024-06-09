vs_1_1
dcl_position v0
dcl_normal v3

; Input:
;
; v0.xyz - position
; v3.xyz - normal mapped -1.f -> 0.f, 1.f -> 1.f
; c0-c3 - world view projection matrix
m4x4 r0, v0, c0					; transform position

mov oPos.xyzw, r0
mov oD0.rgb, v3.xyz					; use normal data as colour

//작업을 수행:
//1. 버텍스의 위치와 노멀 데이터를 입력으로 받습니다.
//2. 위치 데이터는 월드 뷰 프로젝션 행렬을 사용하여 변환됩니다.
//3. 노멀 데이터는 그대로 사용되어 색상 값으로 출력됩니다.