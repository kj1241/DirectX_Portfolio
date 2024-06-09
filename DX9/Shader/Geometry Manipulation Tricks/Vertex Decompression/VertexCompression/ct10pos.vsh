vs_1_1
dcl_position v0
dcl_normal v3

; position is compressed via the compressed transform 10 bit method
; Input:
;
; v0.xyz - position in the range -1.f - 1.f
; v3.xyz - normal
; c0-c3 - world view projection matrix
; c4-c7 - compression matrix
; c8 - <0.5f, 1, ?, ?>
mad r0, v0, c8.x, c8.x				; convert into 0.f to 1.f
m4x4 r1, r0, c4						; decompress position
m4x4 r0, r1, c0					; transform position

mov oPos.xyzw, r0
mov oD0.rgb, v3.xyz					; use normal data as colour

//작업을 수행:
//1. 버텍스의 위치와 노멀 데이터를 입력으로 받습니다.
//2. 버텍스의 위치를 -1.0부터 1.0까지의 범위에서 0.0부터 1.0까지의 범위로 변환합니다.
//3. 변환된 위치를 월드 뷰 프로젝션 행렬로 변환하여 출력 위치를 계산합니다.
//4. 버텍스의 노멀 데이터를 색상으로 사용합니다.

//세부적으로 살펴보면:
//dcl_position v0: 버텍스의 위치 데이터가 v0 레지스터에 입력됩니다.
//dcl_normal v3: 버텍스의 노멀 데이터가 v3 레지스터에 입력됩니다.
//mad r0, v0, c8.x, c8.x: 버텍스의 위치 데이터(v0)를 -1.0부터 1.0까지의 범위에서 0.0부터 1.0까지의 범위로 변환합니다. 이때, 상수(c8.x)를 사용하여 변환합니다.
//m4x4 r1, r0, c4: 변환된 위치 데이터(r0)를 압축된 변환 행렬(c4)로 역압축하여 실제 위치로 복원합니다.
//m4x4 r0, r1, c0: 복원된 위치를 월드 뷰 프로젝션 행렬(c0)로 변환하여 출력 위치(oPos)를 계산합니다.
//mov oPos.xyzw, r0: 계산된 출력 위치를 출력 레지스터(oPos)에 복사합니다.
//mov oD0.rgb, v3.xyz: 버텍스의 노멀 데이터(v3.xyz)를 색상 값으로 사용합니다. 이를 출력 레지스터(oD0.rgb)에 복사합니다.