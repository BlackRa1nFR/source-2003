vs.1.1

#include "macros.vsh"

; Transform the position from world to view space
dp4 oPos.x, $vPos, $cModelViewProj0
dp4 oPos.y, $vPos, $cModelViewProj1
dp4 oPos.z, $vPos, $cModelViewProj2
dp4 oPos.w, $vPos, $cModelViewProj3

mov oD0, $cOne
