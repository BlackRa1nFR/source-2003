vs.1.1

#include "macros.vsh"

;------------------------------------------------------------------------------
;	 c90-c91 = Base texture transform
;    c92-c93 = Mask texture transform
;	 c94	 = Modulation color 
;------------------------------------------------------------------------------

&AllocateRegister( \$projPos );

dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3
mov oPos, $projPos

&FreeRegister( \$projPos );

