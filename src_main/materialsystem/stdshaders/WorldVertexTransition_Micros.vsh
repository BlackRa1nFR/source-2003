vs.1.1

#include "macros.vsh"

alloc $worldPos
alloc $projPos

dp4 $projPos.x, $vPos, $cViewProj0
dp4 $projPos.y, $vPos, $cViewProj1
dp4 $projPos.z, $vPos, $cViewProj2
dp4 $projPos.w, $vPos, $cViewProj3
mov oPos, $projPos


dp4 $worldPos.z, $vPos, $cModel2
&CalcFog( $worldPos, $projPos );

free $projPos
free $worldPos


mov oD0, $vColor

dp4 oT0.x, $vTexCoord0, c90
dp4 oT0.y, $vTexCoord0, c91

dp4 oT1.x, $vTexCoord0, c92
dp4 oT1.y, $vTexCoord0, c93

mov oT2, $vTexCoord1

dp4 oT3.x, $vTexCoord0, c94
dp4 oT3.y, $vTexCoord0, c95


&FreeRegister( \$worldPos ); # garymcthack

