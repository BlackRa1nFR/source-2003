vs.1.1

#include "macros.vsh"

local( $worldPos, $worldNormal, $projPos, $reflectionVector );

&AllocateRegister( \$projPos );

dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

&AllocateRegister( \$worldPos );

; garymcthack
dp4 $worldPos.z, $vPos, $cModel2

&CalcFog( $worldPos, $projPos );

&DoHeightClip( $worldPos, "oT2" );

&FreeRegister( \$worldPos );
&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------
; base texcoords
mov oT0, $vTexCoord0

; lightmap texcoords
mov oT1, $vTexCoord1

&FreeRegister( \$worldPos ); # garymcthack

