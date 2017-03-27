vs.1.1
;------------------------------------------------------------------------------
;	 c90	 = xyz = mouth forward direction vector, w = illum factor
;------------------------------------------------------------------------------

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending 
;------------------------------------------------------------------------------
alloc $worldPos
alloc $worldNormal
&SkinPositionAndNormal( $g_numBones, $worldPos, $worldNormal );

;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------

alloc $projPos

dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3
mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

&CalcFog( $worldPos, $projPos );

free $projPos

;------------------------------------------------------------------------------
; Lighting
;------------------------------------------------------------------------------
alloc $linearColor
&DoDynamicLightingToLinear( $g_ambientLightType, $g_localLightType1, $g_localLightType2, 
                            $worldPos, $worldNormal, $linearColor );

;------------------------------------------------------------------------------
; Factor in teeth darkening factors
;------------------------------------------------------------------------------

alloc $tmp

mul $linearColor.xyz, c90.w, $linearColor	; FIXME Color darkened by illumination factor
dp3 $tmp, $worldNormal, c90					; Figure out mouth forward dot normal
max	$tmp, $cZero, $tmp						; clamp from 0 to 1
mul $linearColor.xyz, $tmp, $linearColor	; Darken by forward dot normal too

;------------------------------------------------------------------------------
; Output color (gamma correction)
;------------------------------------------------------------------------------

alloc $gammaColor
&LinearToGamma( $linearColor, $gammaColor );
free $linearColor
mul oD0.xyz, $gammaColor.xyz, $cOverbrightFactor
mov oD0.w, c0.y				; make sure all components are defined


free $gammaColor
free $worldPos
free $worldNormal
free $tmp

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

mov oT0, $vTexCoord0



