vs.1.1

#include "macros.vsh"

;------------------------------------------------------------------------------
;	 c90-c91 = Base texture transform
;    c92-c93 = Mask texture transform
;	 c94	 = Modulation color 
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

; YUCK!  This is to make texcoords continuous for mat_softwaretl
mov oT0, $cZero
; Texture coordinates
mov oT1, $vTexCoord1

mov oD0, $cOne


