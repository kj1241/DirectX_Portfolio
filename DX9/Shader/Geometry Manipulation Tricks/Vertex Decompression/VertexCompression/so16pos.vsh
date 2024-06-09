vs_1_1
dcl_position v0
dcl_normal v3

; position is compressed via the scale and offset 16 bit method
; Input:
;
; v0.xyz - position in the range 0.f - 1.f
; v3.xyz - normal f
; c0-c3 - world view projection matrix
; c4 - <scale.x,  scale.y, scale.z, 0>
; c5 - <offset.x, offset.y, offset.z, 0>
; c6 - <0.5f, 1, 1.f/65535, 32767>
mad r0, v0, c6.z, c6.x				; get into 0 - 1 range
mov r1, c5							; not allowed 2 constants in 1 instruction
mad r0, r0, c4, r1					; decompress position
m4x4 r1, r0, c0					; transform position

mov oPos.xyzw, r1
mov oD0.rgb, v3.xyz					; use normal data as colour
