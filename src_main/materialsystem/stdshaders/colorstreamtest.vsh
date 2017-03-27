vs.1.1

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&SkinPosition( $g_numBones, $worldPos );

;------------------------------------------------------------------------------
; Transform the position from world to proj space
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3
mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

&CalcFog( $worldPos, $projPos );

&FreeRegister( \$projPos );
&FreeRegister( \$worldPos );

;------------------------------------------------------------------------------
; Lighting (whacks r1-r3, result in r6)
;------------------------------------------------------------------------------

mov oD0, $vColor

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

dp4 oT0.x, $vTexCoord0, c90 // FIXME
dp4 oT0.y, $vTexCoord0, c91 // FIXME


