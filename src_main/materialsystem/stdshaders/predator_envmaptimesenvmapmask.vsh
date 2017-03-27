vs.1.1

#include "macros.vsh"

local( $worldPos, $worldNormal, $projPos, $reflectionVector );

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&SkinPositionAndNormal( $g_numBones, $worldPos, $worldNormal );

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

&ComputeReflectionVector( $worldPos, $worldNormal, "oT0" );

&FreeRegister( \$worldPos );
&FreeRegister( \$worldNormal );

; Modulation color
mov oD0, $cModulationColor
