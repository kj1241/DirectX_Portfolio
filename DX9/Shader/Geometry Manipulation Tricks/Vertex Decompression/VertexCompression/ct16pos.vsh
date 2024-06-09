vs_1_1
dcl_position v0
dcl_normal v3

; position is compressed via the compressed transform 16 bit method
; Input:
;
; v0.xyz - position in the range -32767.f - 32767.f
; v3.xyz - normal
; c0-c3 - world view projection matrix
; c4-c7 - compression matrix
; c8 - <0.5f, 1, 1.f/65535, 32767>
mad r0, v0, c8.z, c8.x				; convert into 0.f to 1.f
m4x4 r1, r0, c4						; decompress position
m4x4 r0, r1, c0					; transform position

mov oPos.xyzw, r0
mov oD0.rgb, v3.xyz					; use normal data as colour

//작업을 수행
//1. 버텍스의 위치와 노멀 데이터를 입력으로 받습니다.
//2. 버텍스의 위치를 -32767.0부터 32767.0까지의 범위에서 0.0부터 1.0까지의 범위로 변환합니다.
//3. 변환된 위치를 월드 뷰 프로젝션 행렬로 변환하여 출력 위치를 계산합니다.
//4. 버텍스의 노멀 데이터를 색상으로 사용합니다.