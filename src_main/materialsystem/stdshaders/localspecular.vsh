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

&AllocateRegister( \$worldPos );

; Transform position from object to world space.
dp4 $worldPos.x, $vPos, $cModel0
dp4 $worldPos.y, $vPos, $cModel1
dp4 $worldPos.z, $vPos, $cModel2

; Get eye vector
&AllocateRegister( \$worldEyeVector );

add $worldEyeVector.rgb, $cEyePos, -$worldPos

&FreeRegister( \$worldPos );

&Normalize( $worldEyeVector );

&AllocateRegister( \$tangentEyeVector );

; move eye vector into tangent space
dp3 $tangentEyeVector.x, $vTangentS, $worldEyeVector
dp3 $tangentEyeVector.y, $vTangentT, $worldEyeVector
dp3 $tangentEyeVector.z, $vNormal, $worldEyeVector

&FreeRegister( \$worldEyeVector );

mul $tangentEyeVector.xyz, $tangentEyeVector, $cHalf

; move eye vector to the 0..1 range and stick in oD0
mad oD0.rgb, $tangentEyeVector, $cHalf, $cHalf

&FreeRegister( \$tangentEyeVector );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; Compute the texture coordinates given the offset between
; each bumped lightmap
&AllocateRegister( \$offset );
mov $offset.xy, $vTexCoord2
add oT0, $vTexCoord0, c90 ; normal map  FIXME
add oT1.xy, $offset, $vTexCoord1
mad oT2.xy, $offset, $cTwo, $vTexCoord1
mad oT3.xy, $offset, $cThree, $vTexCoord1

&FreeRegister( \$offset );
