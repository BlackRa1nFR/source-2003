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

; Compute the texture coordinates given the offset between
; each bumped lightmap
&AllocateRegister( \$offset );
mov $offset.xy, $vTexCoord2
dp4 oT0.x, $vTexCoord0, c90
dp4 oT0.y, $vTexCoord0, c91
add oT1.xy, $offset, $vTexCoord1
mad oT2.xy, $offset, $cTwo, $vTexCoord1
mad oT3.xy, $offset, $cThree, $vTexCoord1

&FreeRegister( \$offset );
