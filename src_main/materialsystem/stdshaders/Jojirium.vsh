vs.1.1

;------------------------------------------------------------------------------
; Shader specific constant:
;	 c94, c95	= normal map transform
;------------------------------------------------------------------------------

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending
;------------------------------------------------------------------------------

&AllocateRegister( \$worldPos );

; Transform position from object to world
dp4 $worldPos.x, $vPos, $cModel0
dp4 $worldPos.y, $vPos, $cModel1
dp4 $worldPos.z, $vPos, $cModel2

&AllocateRegister( \$projPos );

; Transform position from object to projection space
dp4 $projPos.x, $vPos, $cModelViewProj0
dp4 $projPos.y, $vPos, $cModelViewProj1
dp4 $projPos.z, $vPos, $cModelViewProj2
dp4 $projPos.w, $vPos, $cModelViewProj3

mov oPos, $projPos

;------------------------------------------------------------------------------
; Lighting
;------------------------------------------------------------------------------

; Transform tangent space basis vectors to env map space (world space)
; This will produce a set of vectors mapping from tangent space to env space
; We'll use this to transform normals from the normal map from tangent space
; to environment map space. 
; NOTE: use dp3 here since the basis vectors are vectors, not points

dp3 oT1.x, $vTangentS, c90
dp3 oT2.x, $vTangentS, c91
dp3 oT3.x, $vTangentS, c92

dp3 oT1.y, $vTangentT, c90
dp3 oT2.y, $vTangentT, c91
dp3 oT3.y, $vTangentT, c92

dp3 oT1.z, $vNormal, c90
dp3 oT2.z, $vNormal, c91
dp3 oT3.z, $vNormal, c92
 
&AllocateRegister( \$worldToEye );

; Compute the vector from vertex to camera
sub $worldToEye.xyz, $cEyePos, $worldPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

&CalcFog( $worldPos, $projPos );

&FreeRegister( \$worldPos );

; Move it into the w component of the texture coords, as the wacky
; pixel shader wants it there.
mov oT1.w, $worldToEye.x
mov oT2.w, $worldToEye.y
mov oT3.w, $worldToEye.z

&FreeRegister( \$worldToEye );

;------------------------------------------------------------------------------
; Texture coordinates (normal map)
;------------------------------------------------------------------------------
dp4 oT0.x, $vTexCoord0, c94
dp4 oT0.y, $vTexCoord0, c95


&FreeRegister( \$projPos );
