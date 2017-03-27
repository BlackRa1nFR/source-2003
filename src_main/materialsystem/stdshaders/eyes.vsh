vs.1.1
;------------------------------------------------------------------------------
;	 c90	 = eyeball origin			FIXME
;	 c91	 = eyeball up * 0.5			FIXME
;	 c92	 = iris projection U		FIXME
;	 c93	 = iris projection V		FIXME
;	 c94	 = glint projection U		FIXME
;	 c95	 = glint projection V		FIXME
;------------------------------------------------------------------------------

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending (whacks r1-r7, positions in r7)
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
; Normal is based on vertex position 
;------------------------------------------------------------------------------
&AllocateRegister( \$worldNormal );
&AllocateRegister( \$normalDotUp );

sub $worldNormal, $worldPos, c90		; Normal = (Pos - Eye origin)
dp3 $normalDotUp, $worldNormal, c91		; Normal -= 0.5f * (Normal dot Eye Up) * Eye Up
mul $normalDotUp, $normalDotUp, c0.w
mad $worldNormal, -$normalDotUp, c91, $worldNormal

&FreeRegister( \$normalDotUp );

; normalize the normal
&Normalize( $worldNormal );

;------------------------------------------------------------------------------
; Lighting
;------------------------------------------------------------------------------
&DoLighting( $g_staticLightType, $g_ambientLightType, $g_localLightType1, 
			 $g_localLightType2, $worldPos, $worldNormal );

&FreeRegister( \$worldNormal );
 
;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

&CalcFog( $worldPos, $projPos );

&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Texture coordinates
; Texture 0 is the base texture
; Texture 1 is a planar projection used for the iris
; Texture 2 is a planar projection used for the glint
;------------------------------------------------------------------------------

mov oT0, $vTexCoord0
dp4 oT1.x, c92, $worldPos		; FIXME
dp4 oT1.y, c93, $worldPos		; FIXME
dp4 oT2.x, c94, $worldPos		; FIXME
dp4 oT2.y, c95, $worldPos		; FIXME

&FreeRegister( \$worldPos );
