vs.1.1

#include "macros.vsh"

;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; Vertex blending 
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&SkinPositionAndNormal( $g_numBones, $worldPos, $worldNormal );

;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3
mov oPos, $projPos

;------------------------------------------------------------------------------
; Transform the normal from world to view space
;------------------------------------------------------------------------------

&AllocateRegister( \$projNormal );

; only do X and Y since that's all we care about
dp3 $projNormal.x, $worldNormal, $cViewProj0
dp3 $projNormal.y, $worldNormal, $cViewProj1
dp3 $projNormal.z, $worldNormal, $cViewProj2

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------
&CalcFog( $worldPos, $projPos );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; divide by z
rcp $worldPos.w, $worldPos.w
mul $worldPos.xy, $worldPos.w, $worldPos.xy

; map from -1..1 to 0..1
mad $worldPos.xy, $worldPos.xy, $cHalf, $cHalf

; x *= 1
; y *= 3/4
mul $worldPos.xy, $worldPos.xy, c93.xy ; FIXME

; tweak the texcoord offset by alpha
mul $projNormal.xy, $projNormal.xy, $vColor.w

; tweak with the texcoords based on the normal and $basetextureoffset (need to rename)
mad $worldPos.xy, $projNormal.xy, -c94.xy, $worldPos.xy ; FIXME

; y = .75-y
add $worldPos.y, c95.x, -$worldPos.y ; FIXME

; clamp the texcoords
min $worldPos.y, c95.w, $worldPos.y ; FIXME

mov oT0.xy, $worldPos.xy

mov oD0, $cConstants0.wyyw

&FreeRegister( \$projNormal );
&FreeRegister( \$worldNormal );
&FreeRegister( \$projPos );
&FreeRegister( \$worldPos );
