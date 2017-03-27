vs.1.1

#include "macros.vsh"

dp4 oPos.x, $vPos, $cModelViewProj0
dp4 oPos.y, $vPos, $cModelViewProj1
dp4 oPos.z, $vPos, $cModelViewProj2
dp4 oPos.w, $vPos, $cModelViewProj3

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; Just copy the texture coordinates	for the texture
mov oT0, $vTexCoord0

