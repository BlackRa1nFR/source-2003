vs.1.1

#include "macros.vsh"

&AllocateRegister( \$projPos );

dp4 $projPos.x, $vPos, $cViewProj0
dp4 $projPos.y, $vPos, $cViewProj1
dp4 $projPos.z, $vPos, $cViewProj2
dp4 $projPos.w, $vPos, $cViewProj3
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

; Shadow color
mov oD0, $vColor

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------
dp4 r0.x, $vTexCoord0, c90
dp4 r0.y, $vTexCoord0, c91

; Jittered versions
mov oT0, r0
add oT1, r0, c92
sub oT2, r0, c92
add oT3, r0, c93
sub oT4, r0, c93

