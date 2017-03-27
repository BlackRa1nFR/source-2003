vs.1.1
;------------------------------------------------------------------------------
; Constants specified by the app
;    c0      = (0, 1, 2, 0.5)
;	 c1		 = (1/2.2, 0, 0, 0)
;    c2      = camera position *in world space*
;    c4-c7   = modelViewProj matrix	(transpose)
;    c8-c11  = ViewProj matrix (transpose)
;    c12-c15 = model->view matrix (transpose)
;	 c16	 = [fogStart, fogEnd, fogRange, undefined]
;
; Vertex components (as specified in the vertex DECL)
;    $vPos    = Position
;	 $vTexCoord0.xy = TexCoord0
;------------------------------------------------------------------------------

#include "macros.vsh"

; Vertex components
;    $vPos		= Position
;	 $vNormal		= normal
;	 $vTexCoord0.xy	= TexCoord0
;	 $vTangentS		= S axis of Texture space
;	 $vTangentT	= T axis of Texture space

;------------------------------------------------------------------------------
; Transform the position from world to view space
;------------------------------------------------------------------------------

alloc $projPos

; Transform position from object to projection space
dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3

alloc $worldPos

; Transform position from object to world space
dp4 $worldPos.x, $vPos, $cModel0
dp4 $worldPos.y, $vPos, $cModel1
dp4 $worldPos.z, $vPos, $cModel2

if( $g_fogType eq "rangefog" )
{
	&RangeFog( $projPos );
}
elsif( $g_fogType eq "heightfog" )
{
	&WaterFog( $worldPos, $projPos );
}
else
{
	die;
}	

alloc $worldEyeVect

; Get the eye vector in world space
add $worldEyeVect.xyz, -$worldPos, $cEyePos

; transform the eye vector to tangent space
dp3 oT3.x, $worldEyeVect, $vTangentS
dp3 oT3.y, $worldEyeVect, $vTangentT
dp3 oT3.z, $worldEyeVect, $vNormal


; dudv map
alloc $bumpTexCoord

dp4 $bumpTexCoord.x, $vTexCoord0, c91
dp4 $bumpTexCoord.y, $vTexCoord0, c92

mov oT0.xy, $bumpTexCoord

; normal map
mov oT2.xy, $bumpTexCoord

free $bumpTexCoord

alloc $newProjPos

mov oPos, $projPos

; special case perspective correct texture projection so that the texture fits exactly on the screen
mul $projPos.y, $projPos.y, c94.w
add $projPos.xy, $projPos.xy, $projPos.w
mul $projPos.xy, $projPos.xy, $cHalf

mov oT1.xy, $projPos.xy
mov oT1.z, $cZero
mov oT1.w, $projPos.w

free $projPos
free $worldPos
free $worldEyeVect
free $projTangentS
free $projTangentT
free $newProjPos
