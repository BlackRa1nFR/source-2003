vs.1.1

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------

&AllocateRegister( \$worldPos );

; Transform position from object to world
dp4 $worldPos.x, $vPos, $cModel0
dp4 $worldPos.y, $vPos, $cModel1
dp4 $worldPos.z, $vPos, $cModel2
mov $worldPos.w, $cOne

&AllocateRegister( \$projPos );

; Transform to camera space
dp4 $projPos.x, $worldPos, $cViewProj0
dp4 $projPos.y, $worldPos, $cViewProj1
dp4 $projPos.z, $worldPos, $cViewProj2
dp4 $projPos.w, $worldPos, $cViewProj3

mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

&CalcFog( $worldPos, $projPos );
&FreeRegister( \$projPos );

;------------------------------------------------------------------------------
; Lighting
;------------------------------------------------------------------------------

; Transform tangent space basis vectors to env map space (world space)
; This will produce a set of vectors mapping from tangent space to env space
; We'll use this to transform normals from the normal map from tangent space
; to environment map space. 
; NOTE: use dp3 here since the basis vectors are vectors, not points

dp3 oT1.x, $vTangentS, $cModel0
dp3 oT1.y, $vTangentS, $cModel1
dp3 oT1.z, $vTangentS, $cModel2

dp3 oT2.x, $vTangentT, $cModel0
dp3 oT2.y, $vTangentT, $cModel1
dp3 oT2.z, $vTangentT, $cModel2

dp3 oT3.x, $vNormal, $cModel0
dp3 oT3.y, $vNormal, $cModel1
dp3 oT3.z, $vNormal, $cModel2
 
&AllocateRegister( \$vertexToCamera );

; Compute the vector from vertex to camera
sub $vertexToCamera, $cEyePos, $worldPos

&FreeRegister( \$worldPos );

; Move it into the w component of the texture coords, as the wacky
; pixel shader wants it there.
mov oT1.w, $vertexToCamera.x
mov oT2.w, $vertexToCamera.y
mov oT3.w, $vertexToCamera.z

&FreeRegister( \$vertexToCamera );

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

; Just copy the texture coordinates	for the normal map
mov oT0, $vTexCoord0

