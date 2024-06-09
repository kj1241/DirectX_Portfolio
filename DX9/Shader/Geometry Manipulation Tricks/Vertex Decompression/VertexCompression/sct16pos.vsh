vs_1_1
dcl_position0 v0
dcl_position1 v1
dcl_position2 v2

; position is compressed via the scale and offset method
; Input:
;
; v0.xyz - lower 8 bits of position in the range 0.f - 1.f
; v1.xyz - middle 8 bits of position in the range 0.f - 1.f
; v1.xyz - upper 8 bits of position in the range 0.f - 1.f
; v0.w - normal.x 0.f - 1.f
; v1.w - normal.y 0.f - 1.f
; v2.w - normal.z 0.f - 1.f
; c0-c3 - world view projection matrix
; c4-c7 - compression matrix
; c8 - <0.5f, 256.f, 65536.f, 16777216.f>
; c9 - <1.f/32767.f, 1.f, ?, ?>
mov r0.xyz, v0.xyz					; (lower) (we have to be careful because of 23bit mantissa!)
mad r0.xyz, v1.xyz, c8.y, r0.xyz	; (middle * 256) + lower
mad r0.xyz, v2.xyz, c8.z, r0.xyz	; (upper * 65536) + (middle * 256) + lower
mul r0.xyz, r0.xyz, c9.x			; convert into 0.f to 1.f
mov r0.w, c9.y						; set w = 1

m4x4 r1, r0, c4						; decompress position
m4x4 r0, r1, c0					; transform position

mov oPos.xyzw, r0

mov r1.x, v0.w						; unpack normal
mov r1.y, v1.w						; unpack normal
mov r1.z, v2.w						; unpack normal
mov oD0.rgb, r1.xyz					; use normal data as colour

//작업을 수행:
//1. 버텍스의 위치와 노멀 데이터를 입력으로 받습니다.
//2. 위치 데이터는 압축된 상태로 제공됩니다. 낮은 8비트(v0), 중간 8비트(v1), 상위 8비트(v2)로 구성됩니다.
//3. 각 노멀 데이터는 0부터 1까지의 값으로 입력됩니다.
//4. 버텍스의 위치 데이터와 노멀 데이터는 월드 뷰 프로젝션 행렬을 사용하여 변환됩니다.