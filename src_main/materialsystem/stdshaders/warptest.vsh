vs.1.1

#include "macros.vsh"

;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
; Vertex blending (whacks r1-r8, positions in r7, normals in r8)
;------------------------------------------------------------------------------

&SkinPositionAndNormal( $g_numBones );

;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------

dp4 r0.x, r7, c8
dp4 r0.y, r7, c9
dp4 r0.z, r7, c10
dp4 r0.w, r7, c11
mov oPos, r0

;------------------------------------------------------------------------------
; Transform the normal from world to view space
;------------------------------------------------------------------------------

; only do X and Y since that's all we care about
dp3 r1.x, r8, c8
dp3 r1.y, r8, c9
dp3 r1.z, r8, c10

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

&RangeFog( "r1" );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; divide by z
rcp r0.w, r0.w
mul r0.xy, r0.w, r0.xy

; map from -1..1 to 0..1
mad r0.xy, r0.xy, c0.w, c0.w

; x *= 1
; y *= 3/4
;mul r0.xy, r0.xy, c93.xy

; tweak with the texcoords based on the normal and $basetextureoffset (need to rename)
mad r0.xy, r1.xy, -c94.xy, r0.xy

; y = .75-y
;add r0.y, c95.x, -r0.y
; y = 1-y
add r0.y, c0.y, -r0.y

; clamp the texcoords
;min r0.y, c95.w, r0.y

mov oT0.xy, r0.xy

;mov oD0, r6
;mov oD0.xyz, r1.xyz
;mov oD0.xyz, r8.xyz

