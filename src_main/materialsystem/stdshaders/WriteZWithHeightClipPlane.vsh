vs.1.1

#include "macros.vsh"


;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&SkinPosition( $g_numBones, $worldPos );

;------------------------------------------------------------------------------
; Transform the position from world to proj space
;------------------------------------------------------------------------------

dp4 oPos.x, $worldPos, $cViewProj0
dp4 oPos.y, $worldPos, $cViewProj1
dp4 oPos.z, $worldPos, $cViewProj2
dp4 oPos.w, $worldPos, $cViewProj3

&AllocateRegister( \$tmp );

&DoHeightClip( $worldPos, "oT0" );
mov oD0, $cOne

&FreeRegister( \$worldPos );
&FreeRegister( \$tmp );
