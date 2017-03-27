vs.1.1

;------------------------------------------------------------------------------
; Shader specific constant:
;	c90-c91 albedo 1 offset
;	c92-c93 albedo 2 offset
;	c94-c95 normalmap offset
;------------------------------------------------------------------------------

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------

&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&AllocateRegister( \$worldTangentS );
&AllocateRegister( \$worldTangentT );

&SkinPositionNormalAndTangentSpace( $g_numBones, $worldPos, $worldNormal, 
					$worldTangentS, $worldTangentT );

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

;------------------------------------------------------------------------------
; Lighting 
;------------------------------------------------------------------------------
&DoLighting( $g_staticLightType, $g_ambientLightType, $g_localLightType1, 
			 $g_localLightType2, $worldPos, $worldNormal );

;------------------------------------------------------------------------------
; Effects!
;------------------------------------------------------------------------------
; svect
mov oT3.x, $worldTangentS.x
mov oT4.x, $worldTangentS.y
mov oT5.x, $worldTangentS.z
&FreeRegister( \$worldTangentS );

; tvect
mov oT3.y, $worldTangentT.x
mov oT4.y, $worldTangentT.y
mov oT5.y, $worldTangentT.z
&FreeRegister( \$worldTangentT );

; normal
mov oT3.z, $worldNormal.x
mov oT4.z, $worldNormal.y
mov oT5.z, $worldNormal.z
&FreeRegister( \$worldNormal );

; Compute the vector from vertex to camera
&AllocateRegister( \$eyeDir );

sub $eyeDir, $cEyePos, $worldPos
&Normalize( $eyeDir );
mov oT2, $eyeDir;

&FreeRegister( \$eyeDir );
&FreeRegister( \$worldPos );
 

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------
dp4 oT0.x, $vTexCoord0, c90	; albedo 1
dp4 oT0.y, $vTexCoord0, c91

dp4 oT1.x, $vTexCoord0, c94 ; normalmap
dp4 oT1.y, $vTexCoord0, c95
