vs.1.1

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending 
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&SkinPositionAndNormal( $g_numBones, $worldPos, $worldNormal );

;------------------------------------------------------------------------------
; Transform the position from model to proj
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3
mov oPos, $projPos

&CalcFog( $worldPos, $projPos );

&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Lighting
;------------------------------------------------------------------------------
&DoLighting( $g_staticLightType, $g_ambientLightType, $g_localLightType1, 
			 $g_localLightType2, $worldPos, $worldNormal );

&FreeRegister( \$worldPos );
&FreeRegister( \$worldNormal );

