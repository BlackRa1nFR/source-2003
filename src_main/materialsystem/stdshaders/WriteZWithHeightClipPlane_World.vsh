vs.1.1

#include "macros.vsh"


;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );

;------------------------------------------------------------------------------
; Transform the position from world to proj space
;------------------------------------------------------------------------------

dp4 oPos.x, $vPos, $cModelViewProj0
dp4 oPos.y, $vPos, $cModelViewProj1
dp4 oPos.z, $vPos, $cModelViewProj2
dp4 oPos.w, $vPos, $cModelViewProj3

; Transform the position to world space
dp4 $worldPos.x, $vPos, $cModel0
dp4 $worldPos.y, $vPos, $cModel1
dp4 $worldPos.z, $vPos, $cModel2

&DoHeightClip( $worldPos, "oT0" );

mov oD0, $cOne

&FreeRegister( \$worldPos );
