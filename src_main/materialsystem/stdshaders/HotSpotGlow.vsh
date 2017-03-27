vs.1.1
#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

; Transform position from object to projection space
dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3

mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

alloc $worldPos
if( $g_fogType eq "heightfog" )
{
	; Get the worldpos z component only since that's all we need for height fog
	dp4 $worldPos.z, $vPos, $cModel2
}
&CalcFog( $worldPos, $projPos );
free $worldPos

&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; Just copy the texture coordinates	with an offset
;add oT0, $vTexCoord0, c90
add oT0, $vTexCoord0, c95.yyzz	; FIXME
add oT1, $vTexCoord0, c95.xyzz	; FIXME
add oT2, $vTexCoord0, c95.xxzz	; FIXME
add oT3, $vTexCoord0, c95.yxzz	; FIXME

