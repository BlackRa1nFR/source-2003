vs.1.1
#include "macros.vsh"

$cTangentSpaceBumpBasisTranspose1 = "c90";	# [vector 1.x, vector2.x, vector 3.x]
$cTangentSpaceBumpBasisTranspose2 = "c91";	# [vector 1.y, vector2.y, vector 3.y]
$cTangentSpaceBumpBasisTranspose3 = "c92";	# [vector 1.z, vector2.z, vector 3.z]
$cTangentSpaceBumpBasis1 = "c93";			# [vector 1.x, vector1.y, vector 1.z]
$cTangentSpaceBumpBasis2 = "c94";			# [vector 2.x, vector2.y, vector 2.z]
$cTangentSpaceBumpBasis3 = "c95";			# [vector 3.x, vector3.y, vector 3.z]

$cOOSqrt3 = "$cTangentSpaceBumpBasis1.z";

sub CalculateWorldBumpBasis
{
	local( $worldTangentS ) = shift;
	local( $worldTangentT ) = shift;
	local( $worldNormal ) = shift;
	local( $worldBumpBasis1 ) = shift; # was r7
	local( $worldBumpBasis2 ) = shift; # was r8
	local( $worldBumpBasis3 ) = shift; # was r9

#	$worldBumpBasis1 = "r7";
#	$worldBumpBasis2 = "r8";
#	$worldBumpBasis3 = "r9";
#	$worldTangentS = $vUserData;	
#	$worldTangentT = "r10";
#	$worldNormal = $vNormal;

	; Bumped Ambient lighting
	; Optimize: use the bump basis as THE tangent space so that we
	; only have to use three dots to get the world space normal
	; would save us 6 instructions
	; Transform the bump basis into world space.
	; compute world space bump vec 1
	dp3 $worldBumpBasis1.x, $cTangentSpaceBumpBasisTranspose1, $worldTangentS
	dp3 $worldBumpBasis1.y, $cTangentSpaceBumpBasisTranspose1, $worldTangentT
	dp3 $worldBumpBasis1.z, $cTangentSpaceBumpBasisTranspose1, $worldNormal
	; compute world space bump vec 2
	dp3 $worldBumpBasis2.x, $cTangentSpaceBumpBasisTranspose2, $worldTangentS
	dp3 $worldBumpBasis2.y, $cTangentSpaceBumpBasisTranspose2, $worldTangentT
	dp3 $worldBumpBasis2.z, $cTangentSpaceBumpBasisTranspose2, $worldNormal
	; compute world space bump vec 3
	dp3 $worldBumpBasis3.x, $cTangentSpaceBumpBasisTranspose3, $worldTangentS
	dp3 $worldBumpBasis3.y, $cTangentSpaceBumpBasisTranspose3, $worldTangentT
	dp3 $worldBumpBasis3.z, $cTangentSpaceBumpBasisTranspose3, $worldNormal
	; (world space normal already in r8)
}

sub BumpedAmbientLight
{
	; BUMPED AMBIENT LIGHT
	local( $worldBumpBasis1 ) = shift;	# was r7
	local( $worldBumpBasis2 ) = shift;	# was r8
	local( $worldBumpBasis3 ) = shift;	# was r9
	local( $linearColor1 ) = shift;		# was r3
	local( $linearColor2 ) = shift;		# was r4
	local( $linearColor3 ) = shift;		# was r5
	local( $linearColorNormal ) = shift;# was r6
	
#	$worldBumpBasis1 = "r7";
#	$worldBumpBasis2 = "r8";
#	$worldBumpBasis3 = "r9";
#	$linearColor1 = "r3";
#	$linearColor2 = "r4";
#	$linearColor3 = "r5";
#	$linearColorNormal = "r6";

	alloc $nTimesN
	alloc $condition

	; Ambient lighting - bump vec1
	mul $nTimesN.xyz, $worldBumpBasis1.xyz, $worldBumpBasis1.xyz				; compute n times n
	slt $condition.xyz, $worldBumpBasis1.xyz, $cZero				; Figure out whether each component is >0
	mov a0.x, $condition.x
	mul $linearColor1, $nTimesN.x, c[a0.x + $cAmbientColorPosXOffset]			; r3 = normal[0]*normal[0] * box color of appropriate x side
	mov a0.x, $condition.y
	mad $linearColor1, $nTimesN.y, c[a0.x + $cAmbientColorPosYOffset], $linearColor1
	mov a0.x, $condition.z
	mad $linearColor1, $nTimesN.z, c[a0.x + $cAmbientColorPosZOffset], $linearColor1

	; Ambient lighting - bump vec2
	mul $nTimesN.xyz, $worldBumpBasis2.xyz, $worldBumpBasis2.xyz				; compute n times n
	slt $condition.xyz, $worldBumpBasis2.xyz, $cZero				; Figure out whether each component is >0
	mov a0.x, $condition.x
	mul $linearColor2, $nTimesN.x, c[a0.x + $cAmbientColorPosXOffset]			; r4 = normal[0]*normal[0] * box color of appropriate x side
	mov a0.x, $condition.y
	mad $linearColor2, $nTimesN.y, c[a0.x + $cAmbientColorPosYOffset], $linearColor2
	mov a0.x, $condition.z
	mad $linearColor2, $nTimesN.z, c[a0.x + $cAmbientColorPosZOffset], $linearColor2
	
	; Ambient lighting - bump vec3
	mul $nTimesN.xyz, $worldBumpBasis3.xyz, $worldBumpBasis3.xyz				; compute n times n
	slt $condition.xyz, $worldBumpBasis3.xyz, $cZero				; Figure out whether each component is >0
	mov a0.x, $condition.x
	mul $linearColor3, $nTimesN.x, c[a0.x + $cAmbientColorPosXOffset]			; r5 = normal[0]*normal[0] * box color of appropriate x side
	mov a0.x, $condition.y
	mad $linearColor3, $nTimesN.y, c[a0.x + $cAmbientColorPosYOffset], $linearColor3
	mov a0.x, $condition.z
	mad $linearColor3, $nTimesN.z, c[a0.x + $cAmbientColorPosZOffset], $linearColor3
	
	; Ambient lighting - smooth normal
	mul $nTimesN.xyz, $worldNormal.xyz, $worldNormal.xyz				; compute n times n
	slt $condition.xyz, $worldNormal.xyz, $cZero				; Figure out whether each component is >0
	mov a0.x, $condition.x
	mul $linearColorNormal, $nTimesN.x, c[a0.x + $cAmbientColorPosXOffset]			; r6 = normal[0]*normal[0] * box color of appropriate x side
	mov a0.x, $condition.y
	mad $linearColorNormal, $nTimesN.y, c[a0.x + $cAmbientColorPosYOffset], $linearColorNormal
	mov a0.x, $condition.z
	mad $linearColorNormal, $nTimesN.z, c[a0.x + $cAmbientColorPosZOffset], $linearColorNormal

	free $nTimesN
	free $condition
}

sub BumpedDirectionalLight
{
	; BUMPED DIRECTIONAL LIGHT
	local( $worldTangentS ) = shift;
	local( $worldTangentT ) = shift;
	local( $worldNormal ) = shift;
	local( $linearColor1 ) = shift;		# was r3
	local( $linearColor2 ) = shift;		# was r4
	local( $linearColor3 ) = shift;		# was r5
	local( $linearColorNormal ) = shift;# was r6

	alloc $tangentLightDirection # was r0

	; transform light direction into tangent space
	dp3 $tangentLightDirection.x, $worldTangentS, -c[a0.x+1]
	dp3 $tangentLightDirection.y, $worldTangentT, -c[a0.x+1]
	dp3 $tangentLightDirection.z, $worldNormal,   -c[a0.x+1]
	
	; dot against the three tangent space bump vectors
	
	alloc $directionalAttenuation # was r1

	dp3 $directionalAttenuation.x, $cTangentSpaceBumpBasis1, $tangentLightDirection
	dp3 $directionalAttenuation.y, $cTangentSpaceBumpBasis2, $tangentLightDirection
	dp3 $directionalAttenuation.z, $cTangentSpaceBumpBasis3, $tangentLightDirection
	dp3 $directionalAttenuation.w, c0.xxy, $tangentLightDirection		; unbumped normal ( [0 0 1] in tangent space )
	max $directionalAttenuation.xyzw, $directionalAttenuation.xyzw, $cZero	; clamp to zero
	
	mad $linearColor1.xyz, $directionalAttenuation.x, c[a0.x], $linearColor1 ; mult by color
	mad $linearColor2.xyz, $directionalAttenuation.y, c[a0.x], $linearColor2 ; mult by color
	mad $linearColor3.xyz, $directionalAttenuation.z, c[a0.x], $linearColor3 ; mult by color
	mad $linearColorNormal.xyz, $directionalAttenuation.w, c[a0.x], $linearColorNormal ; mult by color

	free $directionalAttenuation
	free $tangentLightDirection
}

sub BumpedPointLight
{
	; BUMPED POINT LIGHT
	local( $worldPos ) = shift;
	local( $worldTangentS ) = shift;
	local( $worldTangentT ) = shift;
	local( $worldNormal ) = shift;
	local( $linearColor1 ) = shift;		# was r3
	local( $linearColor2 ) = shift;		# was r4
	local( $linearColor3 ) = shift;		# was r5
	local( $linearColorNormal ) = shift;# was r6

	alloc $lightDirection	# was r1
	alloc $tmp				# was r2
	alloc $tmp2				# was r0

	; get light direction
	sub $lightDirection, c[a0.x+2], $worldPos
	;
	; normalize light direction, maintain temporaries for attenuation
	;
	dp3 $tmp, $lightDirection, $lightDirection
	; at this point, $tmp = 'd*d d*d d*d d*d'  fixme: don't need to use the whole vector. . can get rid of a register
	rsq $tmp2, $tmp.x
	; at this point, $tmp2 = '1/d 1/d 1/d 1/d'
	mul $lightDirection, $lightDirection, $tmp2.x

	; compute attenuation amount
	alloc $tmp3				# was r11
	dst $tmp3, $tmp, $tmp2					; $tmp3 = ( 1, d, d*d, 1/d )
	dp3 $tmp2, $tmp3, c[a0.x+4]				; $tmp2 = atten0 + d * atten1 + d*d * atten2
	rcp $tmp.w, $tmp2						; $tmp.w = 1 / (atten0 + d * atten1 + d*d * atten2)
	min $tmp.w, $cOne, $tmp.w				; clamp distance attenuation to one

	free $tmp3
	free $tmp2

	alloc $tangentSpaceLightDirection

	; transform light direction into tangent space
	dp3 $tangentSpaceLightDirection.x, $worldTangentS, $lightDirection
	dp3 $tangentSpaceLightDirection.y, $worldTangentT, $lightDirection
	dp3 $tangentSpaceLightDirection.z, $worldNormal, $lightDirection

	free $lightDirection
	
	; dot against the three tangent space bump vectors
	;	 c91	 = tangent space bump basis vector 1
	;	 c92	 = tangent space bump basis vector 2
	;	 c93	 = tangent space bump basis vector 3
	
	alloc $directionalAttenuation

	dp3 $directionalAttenuation.x, $cTangentSpaceBumpBasis1, $tangentSpaceLightDirection
	dp3 $directionalAttenuation.y, $cTangentSpaceBumpBasis2, $tangentSpaceLightDirection
	dp3 $directionalAttenuation.z, $cTangentSpaceBumpBasis3, $tangentSpaceLightDirection
	dp3 $directionalAttenuation.w, c0.xxy, $tangentSpaceLightDirection		; unbumped normal ( [0 0 1] in tangent space )
	free $tangentSpaceLightDirection
	max $directionalAttenuation.xyzw, $directionalAttenuation.xyzw, $cZero	; clamp to zero
	
	mul $tmp.xyz, $tmp.w, c[a0.x]	; calculate color times distance attenuation
	mad $linearColor1.xyz, $directionalAttenuation.x, $tmp, $linearColor1 ; mult by color
	mad $linearColor2.xyz, $directionalAttenuation.y, $tmp, $linearColor2 ; mult by color
	mad $linearColor3.xyz, $directionalAttenuation.z, $tmp, $linearColor3 ; mult by color
	mad $linearColorNormal.xyz, $directionalAttenuation.w, $tmp, $linearColorNormal ; mult by color

	free $tmp
	free $directionalAttenuation
}

sub BumpedSpotLight
{
	; BUMPED SPOT LIGHT
	local( $worldTangentS ) = shift;
	local( $worldTangentT ) = shift;
	local( $worldTangentNormal ) = shift;
	local( $worldBumpBasis1 ) = shift;
	local( $worldBumpBasis2 ) = shift;
	local( $worldBumpBasis3 ) = shift;
	local( $linearColor1 ) = shift;
	local( $linearColor2 ) = shift;
	local( $linearColor3 ) = shift;
	local( $linearColorNormal ) = shift;

	alloc $lightDirection	# was r1
	alloc $tmp				# was r2
	alloc $tmp2				# was r0
	alloc $distAttenuation  # was r6
	
	; get light direction
	sub $lightDirection, c[a0.x+2], $vPos
	; normalize light direction, maintain temporaries for attenuation
	dp3 $tmp.x, $lightDirection, $lightDirection
	; $tmp.x = |light direction|^2
	rsq $tmp.y, $tmp.x
	; $tmp.y = 1/|light direction|
	mul $lightDirection, $lightDirection, $tmp.y
	; compute attenuation amount (r2 = 'd*d d*d d*d d*d', r0 = '1/d 1/d 1/d 1/d')
	dst $tmp2, $tmp.x, $tmp.y			; r11 = ( 1, d, d*d, 1/d )
	dp3 $tmp2, $tmp2, c[a0.x+4]		; r0 = atten0 + d * atten1 + d*d * atten2
	rcp $distAttenuation.w, $tmp2				; r6.w = 1 / (atten0 + d * atten1 + d*d * atten2)
	min $distAttenuation.w, $cOne, $distAttenuation.w		; clamp to one
	
	; compute angular attenuation
	dp3 $tmp2, c[a0.x+1], -$lightDirection			; r11 = dot = -light direction * spot direction
	sub $tmp.y, $tmp2.x, c[a0.x+3].z				; r2.y = dot - stopdot2
	mul $tmp.y, $tmp.y, c[a0.x+3].w					; r2.y = (dot - stopdot2) / (stopdot - stopdot2)
	mov $tmp.w, c[a0.x+3].x							; r2.w = exponent
	lit $tmp2, $tmp									; r11.z = pow((dot - stopdot2) / (stopdot - stopdot2), exponent)
	min $tmp.w, $tmp2.z, $cOne						; clamp pow() to 1
	; $tmp.w = angular attenuation

	; dot against the three world space bump vectors
	;	 r7	 = world space bump basis vector 1
	;	 r8	 = world space bump basis vector 2
	;	 r9	 = world space bump basis vector 3
	

	dp3 $tmp2.x, $worldBumpBasis1, $lightDirection
	dp3 $tmp2.y, $worldBumpBasis2, $lightDirection
	dp3 $tmp2.z, $worldBumpBasis3, $lightDirection
	dp3 $tmp2.w, $worldNormal, $lightDirection		; unbumped normal in world space ( [0 0 1] in tangent space )
	free $lightDirection
	alloc $attenuation # was r1
	max $attenuation.xyzw, $tmp2.xyzw, c0.x	; clamp to zero
	
	mul $tmp.xyz, $distAttenuation.w, c[a0.x]	; calculate light color times distance attenuation
	mul $tmp.xyz, $tmp, $tmp.w		; * angular attenuation
	mad $linearColor1.xyz, $attenuation.x, $tmp, $linearColor1 ; mult by color
	mad $linearColor2.xyz, $attenuation.y, $tmp, $linearColor2 ; mult by color
	mad $linearColor3.xyz, $attenuation.z, $tmp, $linearColor3 ; mult by color
	mad $linearColorNormal.xyz, $attenuation.w, $tmp, $linearColorNormal ; mult by color

	free $attenuation		# was r1
	free $tmp				# was r2
	free $tmp2				# was r0
	free $distAttenuation	# was r6
}

sub DoBumpedLight
{
	local( $lightType ) = shift;
	local( $worldPos ) = shift;
	local( $worldTangentS ) = shift;
	local( $worldTangentT ) = shift;
	local( $worldNormal ) = shift;
	local( $worldBumpBasis1 ) = shift;
	local( $worldBumpBasis2 ) = shift;
	local( $worldBumpBasis3 ) = shift;
	local( $linearColor1 ) = shift;		# was r3
	local( $linearColor2 ) = shift;		# was r4
	local( $linearColor3 ) = shift;		# was r5
	local( $linearColorNormal ) = shift;# was r6

	if( $lightType eq "spot" )
	{
		&BumpedSpotLight( $worldTangentS, $worldTangentT, $worldNormal, 
						  $worldBumpBasis1, $worldBumpBasis2, $worldBumpBasis3, 
						  $linearColor1, $linearColor2, $linearColor3, $linearColorNormal );
	}
	elsif( $lightType eq "point" )
	{
		&BumpedPointLight( $worldPos, $worldTangentS, $worldTangentT, $worldNormal, 
						  $linearColor1, $linearColor2, $linearColor3, $linearColorNormal );
	}
	elsif( $lightType eq "directional" )
	{
		&BumpedDirectionalLight( $worldTangentS, $worldTangentT, $worldNormal,
								 $linearColor1, $linearColor2, $linearColor3, $linearColorNormal );
	}
	else
	{
		die "don't know about light type \"$lightType\"\n";
	}
}

sub BumpedLightingPostfix
{
	; BUMPED LIGHTING POSTFIX
	local( $linearColor1 ) = shift;
	local( $linearColor2 ) = shift;
	local( $linearColor3 ) = shift;
	local( $linearColorNormal ) = shift;
	
	;; gamma correct the unbumped normal
	; OPTIMIZE?: Either use a cubic or quintic to approximate
	; the gamma curve
	
	alloc $gammaColorNormal # was r0
	&LinearToGamma( $linearColorNormal, $gammaColorNormal );
	mul $gammaColorNormal.xyz, $gammaColorNormal.xyz, $cOverbrightFactor	; overbrighting factor
;	&ColorClamp( $gammaColorNormal, $gammaColorNormal );
	
	alloc $averageLinearColor

	; OPTIMIZE: build overbright factor into something similar to $cOneThird
	; compute the average of the three colors for the three vects
	mul $averageLinearColor.xyz, $linearColor1.xyz, $cOOSqrt3			; mult by 1/sqrt(3)
	mad $averageLinearColor.xyz, $linearColor2.xyz, $cOOSqrt3, $averageLinearColor.xyz		
	mad $averageLinearColor.xyz, $linearColor3.xyz, $cOOSqrt3, $averageLinearColor.xyz	
	
	; compute a scale value to multiply all three vec colors by so
	; that the flat normal will match the gamma corrected normal
	rcp $averageLinearColor.x, $averageLinearColor.x
	rcp $averageLinearColor.y, $averageLinearColor.y
	rcp $averageLinearColor.z, $averageLinearColor.z
	mul $averageLinearColor.xyz, $gammaColorNormal, $averageLinearColor

	; $averageLinearColor is now a scale value to get from linear to gamma approximately.
	; is exact for the normal
	if( 1 )
	{
		mul oT2.xyz, $linearColor1, $averageLinearColor
		mul oT3.xyz, $linearColor2, $averageLinearColor
		mul oD0.xyz, $linearColor3, $averageLinearColor
;		mul oT2.xyz, $cOOSqrt3, $gammaColorNormal
;		mul oT3.xyz, $cOOSqrt3, $gammaColorNormal
;		mul oD0.xyz, $cOOSqrt3, $gammaColorNormal
	}
	else
	{
		mul $linearColor1.xyz, $linearColor1, $averageLinearColor
		mul $linearColor2.xyz, $linearColor2, $averageLinearColor
		mul $linearColor3.xyz, $linearColor3, $averageLinearColor
		&ColorClamp( $linearColor1, "oT2" );
		&ColorClamp( $linearColor2, "oT3" );
		&ColorClamp( $linearColor3, "oD0" );
	}
	
	free $gammaColorNormal
	free $averageLinearColor
}

sub DoBumpedLighting
{
	local( $staticLightType ) = shift;
	local( $ambientLightType ) = shift;
	local( $localLightType1 ) = shift;
	local( $localLightType2 ) = shift;
	local( $worldPos ) = shift;
	local( $worldTangentS ) = shift;
	local( $worldTangentT ) = shift;
	local( $worldNormal ) = shift;

	alloc $worldBumpBasis1
	alloc $worldBumpBasis2
	alloc $worldBumpBasis3

	&CalculateWorldBumpBasis( $worldTangentS, $worldTangentT, $worldNormal,
							  $worldBumpBasis1, $worldBumpBasis2, $worldBumpBasis3 );

	alloc $linearColor1
	alloc $linearColor2
	alloc $linearColor3
	alloc $linearColorNormal

	&BumpedAmbientLight( $worldBumpBasis1, $worldBumpBasis2, $worldBumpBasis3,
						 $linearColor1, $linearColor2, $linearColor3, $linearColorNormal );
#	mov $linearColor1, $cZero
#	mov $linearColor2, $cZero
#	mov $linearColor3, $cZero
#	mov $linearColorNormal, $cZero

	if( $localLightType1 ne "none" )
	{
		mov a0.x, $cLight0Offset
		&DoBumpedLight( $localLightType1, $worldPos, $worldTangentS, $worldTangentT, $worldNormal,
						$worldBumpBasis1, $worldBumpBasis2, $worldBumpBasis3, 
						$linearColor1, $linearColor2, $linearColor3, $linearColorNormal );
	}

	if( $localLightType2 ne "none" )
	{
		mov a0.x, $cLight1Offset
		&DoBumpedLight( $localLightType2, $worldPos, $worldTangentS, $worldTangentT, $worldNormal,
						$worldBumpBasis1, $worldBumpBasis2, $worldBumpBasis3, 
						$linearColor1, $linearColor2, $linearColor3, $linearColorNormal );
	}

	&BumpedLightingPostfix( $linearColor1, $linearColor2, $linearColor3, $linearColorNormal );

	free $worldBumpBasis1
	free $worldBumpBasis2
	free $worldBumpBasis3

	free $linearColor1
	free $linearColor2
	free $linearColor3
	free $linearColorNormal
}

;------------------------------------------------------------------------------
; Vertex blending (whacks everything? positions in r7, normals in r8, tangentS in r9, 
; tangentT in r10)
;------------------------------------------------------------------------------

alloc $worldTangentS # was r0
alloc $worldTangentT # was r10

; hard code to zero bones and use software skinning
; would normally do skinning here.

mov $worldTangentS, $vUserData ; have to do this since we can't use two "v" registers in the same instruction

; Calculate tangentT
&Cross( $worldTangentT, $vNormal, $worldTangentS );
; flip direction if necessary
mul $worldTangentT.xyz, $vUserData.w, $worldTangentT.xyz

free $worldTangentS

;------------------------------------------------------------------------------
; Transform the position from world to proj space
;------------------------------------------------------------------------------

alloc $projPos
#$projPos = "r0";
dp4 $projPos.x, $vPos, $cViewProj0
dp4 $projPos.y, $vPos, $cViewProj1
dp4 $projPos.z, $vPos, $cViewProj2
dp4 $projPos.w, $vPos, $cViewProj3
mov oPos, $projPos

;------------------------------------------------------------------------------
; Fog
;------------------------------------------------------------------------------

alloc $worldPos
if( $g_fogType eq "heightfog" )
{
	; Get the worldpos z component only since that's all we need for height fog
	dp4 $worldPos.z, $vPos, $cModel2
}
&CalcFog( $worldPos, $projPos );
free $worldPos
free $projPos

;------------------------------------------------------------------------------
; Lighting
; whacks: r3, r4, r5, r6, ???
; output: 
;		oT2 - color on first bump axis in tangent space
;		oT3 - color on second bump axis in tangent space
;		oD0 - color on third bump axis in tangent space
;------------------------------------------------------------------------------

; c[x]		: light color (r,g,b,magnitude) - not normalized here, but is in pixel shader.
; c[x+1]	: light direction (world space)
; c[x+2]	: light point (world space)
; c[x+3]	: exponent, stopdot, stopdot2, 1 / (stopdot	- stopdot2)
; c[x+4]	: attenuation0, attenuation1, attenuation2


if( 1 )
{
	&DoBumpedLighting( $g_staticLightType, $g_ambientLightType, $g_localLightType1, $g_localLightType2, 
					   $vPos, $vUserData, $worldTangentT, $vNormal );
}
else
{
	$worldNormal = $vNormal;
	$worldPos = $vPos;

	&DoLighting( $g_staticLightType, $g_ambientLightType, $g_localLightType1, 
				 $g_localLightType2, $worldPos, $worldNormal );
}

;------------------------------------------------------------------------------
; Texture coordinates
;------------------------------------------------------------------------------

if( 0 )
{
	; base texture texcoords + offset
	dp4 oT0.x, $vTexCoord0, c93 ; FIXME
	dp4 oT0.y, $vTexCoord0, c94 ; FIXME

	; normal map texcoords + offset
	add oT1, $vTexCoord0, c95
}
else
{
	; base texture texcoords + offset
	mov oT0, $vTexCoord0

	; normal map texcoords + offset
	mov oT1, $vTexCoord0
}

free $worldTangentT
