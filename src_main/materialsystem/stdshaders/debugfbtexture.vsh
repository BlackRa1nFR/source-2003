vs.1.1

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&SkinPosition( $g_numBones, $worldPos );

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
; Fog
;------------------------------------------------------------------------------


&CalcFog( $worldPos, $projPos );

&FreeRegister( \$worldPos );

&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; hack scale for nvidia
mul oT0.xy, $vTexCoord0.xy, c95.xy // FIXME
mov oT0.zw, $cConstants0.xy



