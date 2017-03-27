vs.1.1

;------------------------------------------------------------------------------
; Constants specified by the app
;	 c20	 = Shadow color
;	 c90-c92 = Shadow texture matrix
;	 c93	 = Tex origin
;	 c94	 = Tex Scale
;	 c95	 = [Shadow falloff offset, 1/Shadow distance, Shadow scale, 0 ]
;------------------------------------------------------------------------------

#include "macros.vsh"

;------------------------------------------------------------------------------
; Vertex blending (whacks r1-r7, positions in r7, normals in r8)
;------------------------------------------------------------------------------
&AllocateRegister( \$worldPos );
&AllocateRegister( \$worldNormal );
&SkinPositionAndNormal( $g_numBones, $worldPos, $worldNormal );

; Transform the position from world to view space
&AllocateRegister( \$projPos );

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
; Transform position into texture space (from 0 to 1)
;------------------------------------------------------------------------------
&AllocateRegister( \$texturePos );
dp4 $texturePos.x, $worldPos, c90
dp4 $texturePos.y, $worldPos, c91
dp4 $texturePos.z, $worldPos, c92
&FreeRegister( \$worldPos );

;------------------------------------------------------------------------------
; Figure out the shadow fade amount
;------------------------------------------------------------------------------
&AllocateRegister( \$shadowFade );
sub $shadowFade, $texturePos.z, c95.x
mul $shadowFade, $shadowFade, c95.y
  
;------------------------------------------------------------------------------
; Offset it into the texture
;------------------------------------------------------------------------------
&AllocateRegister( \$actualTextureCoord );
mul $actualTextureCoord.xyz, c94, $texturePos
add oT0.xyz, $actualTextureCoord, c93
;mov oT0.xyz, $texturePos
&FreeRegister( \$actualTextureCoord );

;------------------------------------------------------------------------------
; We're doing clipping by using texkill
;------------------------------------------------------------------------------
mov oT1.xyz, $texturePos		; also clips when shadow z < 0 !
sub oT2.xyz, c0.y, $texturePos
sub oT2.z, c0.y, $shadowFade.z	; clips when shadow z > shadow distance	
&FreeRegister( \$texturePos );

;------------------------------------------------------------------------------
; We're doing backface culling by using texkill also (wow yucky)
;------------------------------------------------------------------------------
; Transform z component of normal in texture space
; If it's negative, then don't draw the pixel
dp3 oT3, $worldNormal, -c92
&FreeRegister( \$worldNormal );

;------------------------------------------------------------------------------
; Shadow color, falloff
;------------------------------------------------------------------------------
mov oD0, $cModulationColor
mul oD0.w, $shadowFade.x, c95.z
&FreeRegister( \$shadowFade );

