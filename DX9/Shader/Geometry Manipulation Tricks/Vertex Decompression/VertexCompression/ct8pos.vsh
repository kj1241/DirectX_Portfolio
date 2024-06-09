vs_1_1
dcl_position v0
dcl_normal v3

; position is compressed via the compressed transform 8 bit method
; Input:
;
; v0.xyz - position in the range 0.f - 1.f
; v3.xyz - normal
; c0-c3 - world view projection matrix
; c4-c7 - compression matrix
; c8 - <0.5f, 1, 0, 0>
m4x4 r1, v0, c4						; decompress position
m4x4 r0, r1, c0					; transform position

mov oPos.xyzw, r0
mov oD0.rgb, v3.xyz					; use normal data as colour

//주요 기능:
//1. 버텍스의 위치 데이터가 압축되어 있습니다. 이 코드는 해당 데이터를 해제하고 변환합니다.
//2. 노멀 데이터는 색상 정보로 사용됩니다.

//동작을 수행
//dcl_position v0: 버텍스의 위치 데이터가 압축된 형태로 v0 레지스터에 입력됩니다.
//dcl_normal v3: 버텍스의 노멀 데이터가 v3 레지스터에 입력됩니다.
//m4x4 r1, v0, c4: 압축된 위치 데이터를 해제하기 위해 압축 해제 행렬(c4)을 사용하여 버텍스의 위치를 해제합니다. 결과는 임시 레지스터인 r1에 저장됩니다.
//m4x4 r0, r1, c0: 버텍스의 위치 데이터를 해제한 후, 월드 뷰 프로젝션 행렬(c0)을 사용하여 위치를 변환합니다. 결과는 r0에 저장됩니다.
//mov oPos.xyzw, r0: 변환된 위치를 출력 위치 레지스터(oPos)에 복사합니다.
//mov oD0.rgb, v3.xyz: 버텍스의 노멀 데이터를 색상으로 출력합니다.