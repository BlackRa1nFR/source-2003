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

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; Just copy the texture coordinates	for the texture
mov oT0, $vTexCoord0

