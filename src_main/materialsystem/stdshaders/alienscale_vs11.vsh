vs.1.1

;------------------------------------------------------------------------------
; Shader specific constant:
;   c19 last row of world->camera transform
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

&FreeRegister( \$worldPos );

;------------------------------------------------------------------------------
; Effects!
;------------------------------------------------------------------------------
; The pixel shader is going to need <sz, tz, nz> measured in *camera space*
; which isn't exactly correct, but good enough for ps1.1. We can do better
; in ps1.4. We've got s, t, and n in *world space*, so let's transform em.
; In order for this to work, we must have the last row of the world->camera
; transform in c19.

dp3 oT3.x, $worldTangentS, c19	; sz
dp3 oT3.y, $worldTangentT, c19	; tz
dp3 oT3.z, $worldNormal, c19	; nz

&FreeRegister( \$worldTangentS );
&FreeRegister( \$worldTangentT );
&FreeRegister( \$worldNormal );
 

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------
dp4 oT0.x, $vTexCoord0, c90	; albedo 1
dp4 oT0.y, $vTexCoord0, c91

dp4 oT1.x, $vTexCoord0, c92	; albedo 2
dp4 oT1.y, $vTexCoord0, c93

dp4 oT2.x, $vTexCoord0, c94 ; normalmap
dp4 oT2.y, $vTexCoord0, c95
