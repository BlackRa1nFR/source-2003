;------------------------------------
; RULES FOR AUTHORING VERTEX SHADERS:
;------------------------------------
; - never use "def" . . .set constants in code instead. . our constant shadowing will break otherwise.
;	(same goes for pixel shaders)
; - use cN notation instead of c[N] notation. .makes grepping for registers easier.
;   The only exception is c[a0.x+blah] where you have no choice.
$g_NumRegisters = 12;

# NOTE: These must match the same values in vsh_prep.pl!
$vPos				= "v0";
$vBoneWeights		= "v1";
$vBoneIndices		= "v2";
$vNormal			= "v3";
$vColor				= "v5";
$vSpecular			= "v6";
$vTexCoord0			= "v7";
$vTexCoord1			= "v8";
$vTexCoord2			= "v9";
$vTexCoord3			= "v10";
$vTangentS			= "v11";
$vTangentT			= "v12";
$vUserData			= "v14";

if( $g_dx9 )
{
	if( $g_usesPos )
	{
		dcl_position $vPos;
	}
	if( $g_usesBoneWeights )
	{
		dcl_blendweight $vBoneWeights;
	}
	if( $g_usesBoneIndices )
	{
		dcl_blendindices $vBoneIndices;
	}
	if( $g_usesNormal )
	{
		dcl_normal $vNormal;
	}
	if( $g_usesColor )
	{
		dcl_color0 $vColor;
	}
	if( $g_usesSpecular )
	{
		dcl_color1 $vSpecular;
	}
	if( $g_usesTexCoord0 )
	{
		dcl_texcoord0 $vTexCoord0;
	}
	if( $g_usesTexCoord1 )
	{
		dcl_texcoord1 $vTexCoord1;
	}
	if( $g_usesTexCoord2 )
	{
		dcl_texcoord2 $vTexCoord2;
	}
	if( $g_usesTexCoord3 )
	{
		dcl_texcoord3 $vTexCoord3;
	}
	if( $g_usesTangentS )
	{
		dcl_tangent $vTangentS;
	}
	if( $g_usesTangentT )
	{
		dcl_binormal0 $vTangentT;
	}
	if( $g_usesUserData )
	{
		dcl_tangent $vUserData;
	}
}


$cConstants0		= "c0";
$cZero				= "c0.x";
$cOne				= "c0.y";
$cTwo				= "c0.z";
$cHalf				= "c0.w";

$cConstants1		= "c1";
$cOOGamma			= "c1.x";
#$cThree				= "c1.y";  # NOTE NOTE NOTE: This is overbright now!!!!  Don't use $cThree!!!
$cOneThird			= "c1.z";
$cOverbrightFactor	= "c1.w";

$cEyePos			= "c2";
$cWaterZ			= "c2.w";
$cEyePosWaterZ		= "c2";

$cLightIndex		= "c3";
$cLight0Offset		= "c3.x"; # 27
$cLight1Offset		= "c3.y"; # 32
$cColorToIntScale	= "c3.z"; # 3.0f * 255.0f ~= 765.01
$cModel0Index		= "c3.w"; # 42

# NOTE: These must match the same values in vsh_prep.pl!
$cModelViewProj0	= "c4";
$cModelViewProj1	= "c5";
$cModelViewProj2	= "c6";
$cModelViewProj3	= "c7";

$cViewProj0			= "c8";
$cViewProj1			= "c9";
$cViewProj2			= "c10";
$cViewProj3			= "c11";

# NOTE: These must match the same values in vsh_prep.pl!
$cModelView0		= "c12";
$cModelView1		= "c13";
$cModelView2		= "c14";
$cModelView3		= "c15";

$cFogParams			= "c16";
$cFogEndOverFogRange = "c16.x";
$cFogOne			= "c16.y";
$cHeightClipZ		= "c16.z";
$cOOFogRange		= "c16.w"; # (1/(fogEnd-fogStart))

$cViewModel0		= "c17";
$cViewModel1		= "c18";
$cViewModel2		= "c19";
$cViewModel3		= "c20";

$cAmbientColorPosX	= "c21";
$cAmbientColorNegX	= "c22";
$cAmbientColorPosY	= "c23";
$cAmbientColorNegY	= "c24";
$cAmbientColorPosZ	= "c25";
$cAmbientColorNegZ	= "c26";

$cAmbientColorPosXOffset	= "21";
$cAmbientColorPosYOffset	= "23";
$cAmbientColorPosZOffset	= "25";

$cLight0DiffColor	= "c27";
$cLight0Dir			= "c28";
$cLight0Pos			= "c29";
$cLight0SpotParams  = "c30"; # [ exponent, stopdot, stopdot2, 1 / (stopdot - stopdot2)
$cLight0Atten		= "c31"; # [ constant, linear, quadratic, 0.0f ]

$cLight1DiffColor	= "c32";
$cLight1Dir			= "c33";
$cLight1Pos			= "c34";
$cLight1SpotParams  = "c35"; # [ exponent, stopdot, stopdot2, 1 / (stopdot - stopdot2)
$cLight1Atten		= "c36"; # [ constant, linear, quadratic, 0.0f ]

# c37-c41 unused! (would be used for a third light if we had one)
$cClipDirection		= "c37.x";
$cClipDirectionTimesHeightClipZ	= "c37.y";
$cModulationColor	= "c38";
$cThree				= "c39.x";

# There are 16 model matrices for skinning
# NOTE: These must match the same values in vsh_prep.pl!
$cModel0			= "c42";
$cModel1			= "c43";
$cModel2			= "c44";

# the last cmodel is c89

# c90-c95 are reserved for shader specific constants

sub OutputUsedRegisters
{
	local( $i );
	; USED REGISTERS
	for( $i = 0; $i < $g_NumRegisters; $i++ )
	{
		if( $g_allocated[$i] )
		{
			; $g_allocatedname[$i] = r$i
		}
	}
	;
}

sub AllocateRegister
{
	local( *reg ) = shift;
	local( $regname ) = shift;
	local( $i );
	for( $i = 0; $i < $g_NumRegisters; $i++ )
	{
		if( !$g_allocated[$i] )
		{
			$g_allocated[$i] = 1;
			$g_allocatedname[$i] = $regname;
			; AllocateRegister $regname = r$i
			$reg = "r$i";
			&OutputUsedRegisters();
			return;
		}
	}
	; Out of registers allocating $regname!
	$reg = "rERROR_OUT_OF_REGISTERS";
	&OutputUsedRegisters();
}

# pass in a reference to a var that contains a register. . ie \$var where var will constain "r1", etc
sub FreeRegister
{
	local( *reg ) = shift;
	local( $regname ) = shift;
	; FreeRegister $regname = $reg
	if( $reg =~ m/rERROR_DEALLOCATED/ )
	{
		; $regname already deallocated
		; $reg = "rALREADY_DEALLOCATED";
		&OutputUsedRegisters();
		return;
	}
#	if( $regname ne g_allocatedname[$reg] )
#	{
#		; Error freeing $reg
#		mov compileerror, freed unallocated register $regname
#	}

	if( ( $reg =~ m/r(.*)/ ) )
	{
		$g_allocated[$1] = 0;
	}
	$reg = "rERROR_DEALLOCATED";
	&OutputUsedRegisters();
}

sub CheckUnfreedRegisters()
{
	local( $i );
	for( $i = 0; $i < $g_NumRegisters; $i++ )
	{
		if( $g_allocated[$i] )
		{
			print "ERROR: r$i allocated to $g_allocatedname[$i] at end of program\n";
			$g_allocated[$i] = 0;
		}
	}
}

sub Normalize
{
	local( $r ) = shift;
	dp3 $r.w, $r, $r
	rsq $r.w, $r.w
	mul $r, $r, $r.w
}

sub Cross
{
	local( $result ) = shift;
	local( $a ) = shift;
	local( $b ) = shift;

	mul $result.xyz, $a.yzx, $b.zxy
	mad $result.xyz, -$b.yzx, $a.zxy, $result
}

sub RangeFog
{
	; Can either be viewPos or projPos since z should be the same for both.
	local( $viewPos ) = shift;

	;------------------------------
	; Regular range fog
	;------------------------------

	; oFog.x = 1.0f = no fog
	; oFog.x = 0.0f = full fog
	; compute fog factor f = (fog_end - dist)*(1/(fog_end-fog_start))
	; this is == to: (fog_end/(fog_end-fog_start) - dist/(fog_end-fog_start)
	; which can be expressed with a single mad instruction!
	if( $g_dx9 )
	{
		mad oFog, -$viewPos.z, $cOOFogRange, $cFogEndOverFogRange
	}
	else
	{
		mad oFog.x, -$viewPos.z, $cOOFogRange, $cFogEndOverFogRange
	}
}

sub WaterFog
{
	; oFog.x = 1.0f = no fog
	; oFog.x = 0.0f = full fog
;	mov oFog.x, $cOne
;	return;
	; only $worldPos.z is used out of worldPos
	local( $worldPos ) = shift;
	local( $viewPos ) = shift;
	
	; $viewPos.z is the distance from the eye to the vertex
	local( $tmp );
	&AllocateRegister( \$tmp );
	; Calculate the ratio of the line of sight integral through the water to the total line
	; integral
	; These could both be done in a single add if cWaterZ and cEyePos.z were in the same constant
;	add $tmp.x, $cWaterZ, -$worldPos.z
;	add $tmp.y, $cEyePos.z, -$worldPos.z
	add $tmp.xy, $cEyePosWaterZ.wz, -$worldPos.z

	; $tmp.x is the distance from the water surface to the vert
	; $tmp.y is the distance from the eye position to the vert

	; if $tmp.x < 0, then set it to 0
	; This is the equivalent of moving the vert to the water surface if it's above the water surface
	max $tmp.x, $tmp.x, $cZero

	; $tmp.w = $tmp.x / $tmp.y
	rcp $tmp.z, $tmp.y
	mul $tmp.w, $tmp.x, $tmp.z

	; If the eye is under water, then always use the whole fog amount
	; Duh, if the eye is under water, use regular fog!
;	sge $tmp.z, $tmp.y, $cZero
	; $tmp.z = 0 if the eye is underwater, otherwise $tmp.z = 1
;	mul $tmp.w, $tmp.w, $tmp.z
;	add $tmp.z, $cOne, -$tmp.z
;	add $tmp.w, $tmp.w, $tmp.z

	mul $tmp.w, $tmp.w, $viewPos.z
	; $tmp.w is now the distance that we see through water.

	if( $g_dx9 )
	{
		mad oFog, -$tmp.w, $cOOFogRange, $cFogOne
	}
	else
	{
		mad oFog.x, -$tmp.w, $cOOFogRange, $cFogOne
	}

	&FreeRegister( \$tmp );
}


#------------------------------------------------------------------------------
# Main fogging routine
#------------------------------------------------------------------------------
sub CalcFog
{
	; CalcFog
	local( $worldPos ) = shift;
	local( $projPos ) = shift;

	if( $g_fogType eq "rangefog" )
	{
		&RangeFog( $projPos );
	}
	elsif( $g_fogType eq "heightfog" )
	{
		&WaterFog( $worldPos, $projPos );
	}
	else
	{
		die;
	}	
}


sub DoHeightClip
{
	; DoHeightClip
;	$texReg = $cClipDirection * ( $cHeightClipZ - $worldPos.z )
;	$texReg = $cClipDirection * $cHeightClipZ - $cClipDirection * $worldPos.z
;	$const = $cClipDirection * $cHeightClipZ;
;	$texReg = $const - $cClipDirection * $worldPos.z
;	$texReg = ( - $cClipDirection * $worldPos.z ) + $const

	local( $worldPos ) = shift;
	local( $texReg ) = shift;
	local( $tmp );

	# Do a user clip plan using texkill in the case that we don't have
	# a detail texture.
	# optimize!  Can probably do an arbitrary plane in one or two instructions.
	if( 0 )
	{
		&AllocateRegister( \$tmp );
		add $tmp, -$worldPos.z, $cHeightClipZ
		# This determines which side we are clipping on.
		mul $texReg, $tmp, $cClipDirection
		&FreeRegister( \$tmp );
	}
	else
	{
		mad $texReg, -$cClipDirection, $worldPos.z, $cClipDirectionTimesHeightClipZ	
	}
}

sub GammaToLinear
{
	local( $gamma ) = shift;
	local( $linear ) = shift;

	local( $tmp );
	&AllocateRegister( \$tmp );

	; Is rcp more expensive than just storing 2.2 somewhere and doing a mov?
	rcp $gamma.w, $cOOGamma							; $gamma.w = 2.2
	lit $linear.z, $gamma.zzzw						; r0.z = linear blue
	lit $tmp.z, $gamma.yyyw							; r2.z = linear green
	mov $linear.y, $tmp.z							; r0.y = linear green
	lit $tmp.z, $gamma.xxxw							; r2.z = linear red
	mov $linear.x, $tmp.z							; r0.x = linear red

	&FreeRegister( \$tmp );
}

sub LinearToGamma
{
	local( $linear ) = shift;
	local( $gamma ) = shift;

	local( $tmp );
	&AllocateRegister( \$tmp );

	mov $linear.w, $cOOGamma						; $linear.w = 1.0/2.2
	lit $gamma.z, $linear.zzzw						; r0.z = gamma blue
	lit $tmp.z, $linear.yyyw						; r2.z = gamma green
	mov $gamma.y, $tmp.z							; r0.y = gamma green
	lit $tmp.z, $linear.xxxw						; r2.z = gamma red
	mov $gamma.x, $tmp.z							; r0.x = gamma red

	&FreeRegister( \$tmp );
}

sub ComputeReflectionVector
{
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $reflectionVector ) = shift;

	local( $vertToEye ); &AllocateRegister( \$vertToEye );
	local( $tmp ); &AllocateRegister( \$tmp );

	; compute reflection vector r = 2 * (n dot v) n - v
	sub $vertToEye.xyz, $cEyePos.xyz, $worldPos  ; $tmp1 = v = c - p
	dp3 $tmp, $worldNormal, $vertToEye			; $tmp = n dot v
	mul $tmp.xyz, $tmp.xyz, $worldNormal	; $tmp = (n dot v ) n
	mad $reflectionVector.xyz, $tmp, $cTwo, -$vertToEye

	&FreeRegister( \$vertToEye );
	&FreeRegister( \$tmp );
}

sub ComputeSphereMapTexCoords
{
	local( $reflectionVector ) = shift;
	local( $sphereMapTexCoords ) = shift;

	local( $tmp ); &AllocateRegister( \$tmp );

	; transform reflection vector into view space
	dp3 $tmp.x, $reflectionVector, $cViewModel0
	dp3 $tmp.y, $reflectionVector, $cViewModel1
	dp3 $tmp.z, $reflectionVector, $cViewModel2

	; generate <rx ry rz+1>
	add $tmp.z, $tmp.z, $cOne

	; find 1 / the length of r2
	dp3 $tmp.w, $tmp, $tmp
	rsq $tmp.w, $tmp.w

	; r1 = r2/|r2| + 1
	mad $tmp.xy, $tmp.w, $tmp, $cOne
	mul $sphereMapTexCoords.xy, $tmp.xy, $cHalf
	
	&FreeRegister( \$tmp );
}

sub SkinPosition
{
	local( $numBones ) = shift;
	local( $worldPos ) = shift;
	if( $numBones == 0 )
	{
		;
		; 0 bone skinning (4 instructions)
		;
		; Transform position into world space
		; position
		dp4 $worldPos.x, $vPos, $cModel0
		dp4 $worldPos.y, $vPos, $cModel1
		dp4 $worldPos.z, $vPos, $cModel2
		mov $worldPos.w, $cOne
	} 
	elsif( $numBones == 1 )
	{
		;
		; 1 bone skinning (6 instructions)
		;

		local( $boneIndices );
		&AllocateRegister( \$boneIndices );

		; Perform 1 bone skinning
		; Transform position into world space
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		mov a0.x, $boneIndices.z

		; position
		dp4 $worldPos.x, $vPos, c[a0.x]
		dp4 $worldPos.y, $vPos, c[a0.x + 1]
		dp4 $worldPos.z, $vPos, c[a0.x + 2]
		mov $worldPos.w, $cOne

		&FreeRegister( \$boneIndices );
	}
	elsif( $numBones == 2 )
	{
		;
		; 2 bone skinning (13 instructions)
		;

		local( $boneIndices );
		local( $blendedMatrix0 );
		local( $blendedMatrix1 );
		local( $blendedMatrix2 );
		&AllocateRegister( \$boneIndices );
		&AllocateRegister( \$blendedMatrix0 );
		&AllocateRegister( \$blendedMatrix1 );
		&AllocateRegister( \$blendedMatrix2 );

		; Transform position into world space using all bones
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		; r11 = boneindices at this point
		; first matrix
		mov a0.x, $boneIndices.z
		mul $blendedMatrix0, $vBoneWeights.x, c[a0.x]
		mul $blendedMatrix1, $vBoneWeights.x, c[a0.x+1]
		mul $blendedMatrix2, $vBoneWeights.x, c[a0.x+2]
		; second matrix
		mov a0.x, $boneIndices.y
		mad $blendedMatrix0, $vBoneWeights.y, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $vBoneWeights.y, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $vBoneWeights.y, c[a0.x+2], $blendedMatrix2

		; position
		dp4 $worldPos.x, $vPos, $blendedMatrix0
		dp4 $worldPos.y, $vPos, $blendedMatrix1
		dp4 $worldPos.z, $vPos, $blendedMatrix2
		mov $worldPos.w, $cOne

		&FreeRegister( \$boneIndices );
		&FreeRegister( \$blendedMatrix0 );
		&FreeRegister( \$blendedMatrix1 );
		&FreeRegister( \$blendedMatrix2 );
	}
	elsif( $numBones == 3 )
	{
		;
		; 3 bone skinning  (19 instructions)
		;
		local( $boneIndices );
		local( $blendedMatrix0 );
		local( $blendedMatrix1 );
		local( $blendedMatrix2 );
		&AllocateRegister( \$boneIndices );
		&AllocateRegister( \$blendedMatrix0 );
		&AllocateRegister( \$blendedMatrix1 );
		&AllocateRegister( \$blendedMatrix2 );

		; Transform position into world space using all bones
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		; r11 = boneindices at this point
		; first matrix
		mov a0.x, $boneIndices.z
		mul $blendedMatrix0, $vBoneWeights.x, c[a0.x]
		mul $blendedMatrix1, $vBoneWeights.x, c[a0.x+1]
		mul $blendedMatrix2, $vBoneWeights.x, c[a0.x+2]
		; second matrix
		mov a0.x, $boneIndices.y
		mad $blendedMatrix0, $vBoneWeights.y, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $vBoneWeights.y, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $vBoneWeights.y, c[a0.x+2], $blendedMatrix2

		; Calculate third weight
		; compute 1-(weight1+weight2) to calculate weight2
		; Use $boneIndices.w as a temp since we aren't using it for anything.
		add $boneIndices.w, $vBoneWeights.x, $vBoneWeights.y
		sub $boneIndices.w, $cOne, $boneIndices.w

		; third matrix
		mov a0.x, $boneIndices.x
		mad $blendedMatrix0, $boneIndices.w, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $boneIndices.w, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $boneIndices.w, c[a0.x+2], $blendedMatrix2
		
		; position
		dp4 $worldPos.x, $vPos, $blendedMatrix0
		dp4 $worldPos.y, $vPos, $blendedMatrix1
		dp4 $worldPos.z, $vPos, $blendedMatrix2
		mov $worldPos.w, $cOne

		&FreeRegister( \$boneIndices );
		&FreeRegister( \$blendedMatrix0 );
		&FreeRegister( \$blendedMatrix1 );
		&FreeRegister( \$blendedMatrix2 );
	}
}

sub SkinPositionAndNormal
{
	local( $numBones ) = shift;
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;

	if( $numBones == 0 )
	{
		;
		; 0 bone skinning (13 instructions)
		;
		; Transform position + normal + tangentS + tangentT into world space
		; position
		dp4 $worldPos.x, $vPos, $cModel0
		dp4 $worldPos.y, $vPos, $cModel1
		dp4 $worldPos.z, $vPos, $cModel2
		mov $worldPos.w, $cOne
		; normal
		dp3 $worldNormal.x, $vNormal, $cModel0
		dp3 $worldNormal.y, $vNormal, $cModel1
		dp3 $worldNormal.z, $vNormal, $cModel2
	}
	elsif( $numBones == 1 )
	{
		;
		; 1 bone skinning (17 instructions)
		;

		local( $boneIndices );
		&AllocateRegister( \$boneIndices );

		; Perform 1 bone skinning
		; Transform position into world space
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		mov a0.x, $boneIndices.z

		; position
		dp4 $worldPos.x, $vPos, c[a0.x]
		dp4 $worldPos.y, $vPos, c[a0.x + 1]
		dp4 $worldPos.z, $vPos, c[a0.x + 2]
		mov $worldPos.w, $cOne

		; normal
		dp3 $worldNormal.x, $vNormal, c[a0.x]
		dp3 $worldNormal.y, $vNormal, c[a0.x + 1]
		dp3 $worldNormal.z, $vNormal, c[a0.x + 2]

		&FreeRegister( \$boneIndices );
	}
	elsif( $numBones == 2 )
	{
		;
		; 2 bone skinning (16 instructions)
		;

		local( $boneIndices );
		local( $blendedMatrix0 );
		local( $blendedMatrix1 );
		local( $blendedMatrix2 );
		&AllocateRegister( \$boneIndices );
		&AllocateRegister( \$blendedMatrix0 );
		&AllocateRegister( \$blendedMatrix1 );
		&AllocateRegister( \$blendedMatrix2 );

		; Transform position into world space using all bones
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		; r11 = boneindices at this point
		; first matrix
		mov a0.x, $boneIndices.z
		mul $blendedMatrix0, $vBoneWeights.x, c[a0.x]
		mul $blendedMatrix1, $vBoneWeights.x, c[a0.x+1]
		mul $blendedMatrix2, $vBoneWeights.x, c[a0.x+2]
		; second matrix
		mov a0.x, $boneIndices.y
		mad $blendedMatrix0, $vBoneWeights.y, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $vBoneWeights.y, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $vBoneWeights.y, c[a0.x+2], $blendedMatrix2

		; position
		dp4 $worldPos.x, $vPos, $blendedMatrix0
		dp4 $worldPos.y, $vPos, $blendedMatrix1
		dp4 $worldPos.z, $vPos, $blendedMatrix2
		mov $worldPos.w, $cOne

		; normal
		dp3 $worldNormal.x, $vNormal, $blendedMatrix0
		dp3 $worldNormal.y, $vNormal, $blendedMatrix1
		dp3 $worldNormal.z, $vNormal, $blendedMatrix2

		&FreeRegister( \$boneIndices );
		&FreeRegister( \$blendedMatrix0 );
		&FreeRegister( \$blendedMatrix1 );
		&FreeRegister( \$blendedMatrix2 );
	}
	elsif( $numBones == 3 )
	{
		local( $boneIndices );
		local( $blendedMatrix0 );
		local( $blendedMatrix1 );
		local( $blendedMatrix2 );
		&AllocateRegister( \$boneIndices );
		&AllocateRegister( \$blendedMatrix0 );
		&AllocateRegister( \$blendedMatrix1 );
		&AllocateRegister( \$blendedMatrix2 );

		; Transform position into world space using all bones
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		; r11 = boneindices at this point
		; first matrix
		mov a0.x, $boneIndices.z
		mul $blendedMatrix0, $vBoneWeights.x, c[a0.x]
		mul $blendedMatrix1, $vBoneWeights.x, c[a0.x+1]
		mul $blendedMatrix2, $vBoneWeights.x, c[a0.x+2]
		; second matrix
		mov a0.x, $boneIndices.y
		mad $blendedMatrix0, $vBoneWeights.y, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $vBoneWeights.y, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $vBoneWeights.y, c[a0.x+2], $blendedMatrix2

		; Calculate third weight
		; compute 1-(weight1+weight2) to calculate weight2
		; Use $boneIndices.w as a temp since we aren't using it for anything.
		add $boneIndices.w, $vBoneWeights.x, $vBoneWeights.y
		sub $boneIndices.w, $cOne, $boneIndices.w

		; third matrix
		mov a0.x, $boneIndices.x
		mad $blendedMatrix0, $boneIndices.w, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $boneIndices.w, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $boneIndices.w, c[a0.x+2], $blendedMatrix2
		
		; position
		dp4 $worldPos.x, $vPos, $blendedMatrix0
		dp4 $worldPos.y, $vPos, $blendedMatrix1
		dp4 $worldPos.z, $vPos, $blendedMatrix2
		mov $worldPos.w, $cOne

		; normal
		dp3 $worldNormal.x, $vNormal, $blendedMatrix0
		dp3 $worldNormal.y, $vNormal, $blendedMatrix1
		dp3 $worldNormal.z, $vNormal, $blendedMatrix2

		&FreeRegister( \$boneIndices );
		&FreeRegister( \$blendedMatrix0 );
		&FreeRegister( \$blendedMatrix1 );
		&FreeRegister( \$blendedMatrix2 );
	}	
}

sub SkinPositionNormalAndTangentSpace
{
	local( $numBones ) = shift;
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $worldTangentS ) = shift;
	local( $worldTangentT ) = shift;
	if( $numBones == 0 )
	{
		;
		; 0 bone skinning (13 instructions)
		;
		; Transform position + normal + tangentS + tangentT into world space

		; position
		dp4 $worldPos.x, $vPos, $cModel0
		dp4 $worldPos.y, $vPos, $cModel1
		dp4 $worldPos.z, $vPos, $cModel2
		mov $worldPos.w, $cOne

		; normal
		dp3 $worldNormal.x, $vNormal, $cModel0
		dp3 $worldNormal.y, $vNormal, $cModel1
		dp3 $worldNormal.z, $vNormal, $cModel2

		; tangents
		dp3 $worldTangentS.x, $vUserData, $cModel0
		dp3 $worldTangentS.y, $vUserData, $cModel1
		dp3 $worldTangentS.z, $vUserData, $cModel2

		; calculate tangent t via cross( N, S ) * S[3]
		&Cross( $worldTangentT, $worldNormal, $worldTangentS );
		mul $worldTangentT.xyz, $vUserData.w, $worldTangentT.xyz
	}
	elsif( $numBones == 1 )
	{
		;
		; 1 bone skinning (17 instructions)
		;

		local( $boneIndices );
		&AllocateRegister( \$boneIndices );

		; Perform 1 bone skinning
		; Transform position into world space
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		mov a0.x, $boneIndices.z

		; position
		dp4 $worldPos.x, $vPos, c[a0.x]
		dp4 $worldPos.y, $vPos, c[a0.x + 1]
		dp4 $worldPos.z, $vPos, c[a0.x + 2]
		mov $worldPos.w, $cOne

		; normal
		dp3 $worldNormal.x, $vNormal, c[a0.x]
		dp3 $worldNormal.y, $vNormal, c[a0.x + 1]
		dp3 $worldNormal.z, $vNormal, c[a0.x + 2]

		; tangents
		dp3 $worldTangentS.x, $vUserData, c[a0.x]
		dp3 $worldTangentS.y, $vUserData, c[a0.x + 1]
		dp3 $worldTangentS.z, $vUserData, c[a0.x + 2]

		; calculate tangent t via cross( N, S ) * S[3]
		&Cross( $worldTangentT, $worldNormal, $worldTangentS );
		mul $worldTangentT.xyz, $vUserData.w, $worldTangentT.xyz

		&FreeRegister( \$boneIndices );
	}
	elsif( $numBones == 2 )
	{
		;
		; 2 bone skinning (22 instructions)
		;

		local( $boneIndices );
		local( $blendedMatrix0 );
		local( $blendedMatrix1 );
		local( $blendedMatrix2 );
		&AllocateRegister( \$boneIndices );
		&AllocateRegister( \$blendedMatrix0 );
		&AllocateRegister( \$blendedMatrix1 );
		&AllocateRegister( \$blendedMatrix2 );

		; Transform position into world space using all bones
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		; r11 = boneindices at this point
		; first matrix
		mov a0.x, $boneIndices.z
		mul $blendedMatrix0, $vBoneWeights.x, c[a0.x]
		mul $blendedMatrix1, $vBoneWeights.x, c[a0.x+1]
		mul $blendedMatrix2, $vBoneWeights.x, c[a0.x+2]
		; second matrix
		mov a0.x, $boneIndices.y
		mad $blendedMatrix0, $vBoneWeights.y, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $vBoneWeights.y, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $vBoneWeights.y, c[a0.x+2], $blendedMatrix2

		; position
		dp4 $worldPos.x, $vPos, $blendedMatrix0
		dp4 $worldPos.y, $vPos, $blendedMatrix1
		dp4 $worldPos.z, $vPos, $blendedMatrix2
		mov $worldPos.w, $cOne

		; normal
		dp3 $worldNormal.x, $vNormal, $blendedMatrix0
		dp3 $worldNormal.y, $vNormal, $blendedMatrix1
		dp3 $worldNormal.z, $vNormal, $blendedMatrix2

		; tangents
		dp3 $worldTangentS.x, $vUserData, $blendedMatrix0
		dp3 $worldTangentS.y, $vUserData, $blendedMatrix1
		dp3 $worldTangentS.z, $vUserData, $blendedMatrix2

		; calculate tangent t via cross( N, S ) * S[3]
		&Cross( $worldTangentT, $worldNormal, $worldTangentS );
		mul $worldTangentT.xyz, $vUserData.w, $worldTangentT.xyz

		&FreeRegister( \$boneIndices );
		&FreeRegister( \$blendedMatrix0 );
		&FreeRegister( \$blendedMatrix1 );
		&FreeRegister( \$blendedMatrix2 );
	}
	elsif( $numBones == 3 )
	{
		local( $boneIndices );
		local( $blendedMatrix0 );
		local( $blendedMatrix1 );
		local( $blendedMatrix2 );
		&AllocateRegister( \$boneIndices );
		&AllocateRegister( \$blendedMatrix0 );
		&AllocateRegister( \$blendedMatrix1 );
		&AllocateRegister( \$blendedMatrix2 );

		; Transform position into world space using all bones
		; denormalize d3dcolor to matrix index
		mad $boneIndices, $vBoneIndices, $cColorToIntScale, $cModel0Index
		; r11 = boneindices at this point
		; first matrix
		mov a0.x, $boneIndices.z
		mul $blendedMatrix0, $vBoneWeights.x, c[a0.x]
		mul $blendedMatrix1, $vBoneWeights.x, c[a0.x+1]
		mul $blendedMatrix2, $vBoneWeights.x, c[a0.x+2]
		; second matrix
		mov a0.x, $boneIndices.y
		mad $blendedMatrix0, $vBoneWeights.y, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $vBoneWeights.y, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $vBoneWeights.y, c[a0.x+2], $blendedMatrix2

		; Calculate third weight
		; compute 1-(weight1+weight2) to calculate weight2
		; Use $boneIndices.w as a temp since we aren't using it for anything.
		add $boneIndices.w, $vBoneWeights.x, $vBoneWeights.y
		sub $boneIndices.w, $cOne, $boneIndices.w

		; third matrix
		mov a0.x, $boneIndices.x
		mad $blendedMatrix0, $boneIndices.w, c[a0.x], $blendedMatrix0
		mad $blendedMatrix1, $boneIndices.w, c[a0.x+1], $blendedMatrix1
		mad $blendedMatrix2, $boneIndices.w, c[a0.x+2], $blendedMatrix2
		
		; position
		dp4 $worldPos.x, $vPos, $blendedMatrix0
		dp4 $worldPos.y, $vPos, $blendedMatrix1
		dp4 $worldPos.z, $vPos, $blendedMatrix2
		mov $worldPos.w, $cOne

		; normal
		dp3 $worldNormal.x, $vNormal, $blendedMatrix0
		dp3 $worldNormal.y, $vNormal, $blendedMatrix1
		dp3 $worldNormal.z, $vNormal, $blendedMatrix2

		; tangents
		dp3 $worldTangentS.x, $vUserData, $blendedMatrix0
		dp3 $worldTangentS.y, $vUserData, $blendedMatrix1
		dp3 $worldTangentS.z, $vUserData, $blendedMatrix2

		; calculate tangent t via cross( N, S ) * S[3]
		&Cross( $worldTangentT, $worldNormal, $worldTangentS );
		mul $worldTangentT.xyz, $vUserData.w, $worldTangentT.xyz

		&FreeRegister( \$boneIndices );
		&FreeRegister( \$blendedMatrix0 );
		&FreeRegister( \$blendedMatrix1 );
		&FreeRegister( \$blendedMatrix2 );
	}
}

sub ColorClamp
{
	; ColorClamp; stomps $color.w
	local( $color ) = shift;
	local( $dst ) = shift;

	; Get the max of RGB and stick it in W
	max $color.w, $color.x, $color.y
	max $color.w, $color.w, $color.z

	; get the greater of one and the max color.
	max $color.w, $color.w, $cOne

	rcp $color.w, $color.w
	mul $dst.xyz, $color.w, $color.xyz
}

sub AmbientLight
{
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;
	local( $add ) = shift;

	; Ambient lighting
	&AllocateRegister( \$nSquared );
	&AllocateRegister( \$isNegative );

	mul $nSquared.xyz, $worldNormal.xyz, $worldNormal.xyz				; compute n times n
	slt $isNegative.xyz, $worldNormal.xyz, $cZero				; Figure out whether each component is >0
	mov a0.x, $isNegative.x
	if( $add )
	{
		mad $linearColor.xyz, $nSquared.x, c[a0.x + $cAmbientColorPosXOffset], $linearColor			; $linearColor = normal[0]*normal[0] * box color of appropriate x side
	}
	else
	{
		mul $linearColor.xyz, $nSquared.x, c[a0.x + $cAmbientColorPosXOffset]			; $linearColor = normal[0]*normal[0] * box color of appropriate x side
	}
	mov a0.x, $isNegative.y
	mad $linearColor.xyz, $nSquared.y, c[a0.x + $cAmbientColorPosYOffset], $linearColor
	mov a0.x, $isNegative.z
	mad $linearColor.xyz, $nSquared.z, c[a0.x + $cAmbientColorPosZOffset], $linearColor

	&FreeRegister( \$isNegative );
	&FreeRegister( \$nSquared );
}

sub DirectionalLight
{
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;
	local( $add ) = shift;

	&AllocateRegister( \$nDotL ); # FIXME: This only needs to be a scalar

	; NOTE: Gotta use -l here, since light direction = -l
	; DIRECTIONAL LIGHT
	; compute n dot l
	dp3 $nDotL.x, -c[a0.x + 1], $worldNormal
	max $nDotL.x, $nDotL.x, c0.x			; Clamp to zero
	if( $add )
	{
		mad $linearColor.xyz, c[a0.x], $nDotL.x, $linearColor
	}
	else
	{
		mov $linearColor.xyz, c[a0.x], $nDotL.x
	}

	&FreeRegister( \$nDotL );
}

sub PointLight
{
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;
	local( $add ) = shift;

	local( $lightDir );
	&AllocateRegister( \$lightDir );
	
	; POINT LIGHT
	; compute light direction
	sub $lightDir, c[a0.x+2], $worldPos
	
	local( $lightDistSquared );
	local( $ooLightDist );
	&AllocateRegister( \$lightDistSquared );
	&AllocateRegister( \$ooLightDist );

	; normalize light direction, maintain temporaries for attenuation
	dp3 $lightDistSquared, $lightDir, $lightDir
	rsq $ooLightDist, $lightDistSquared.x
	mul $lightDir, $lightDir, $ooLightDist.x
	
	local( $attenuationFactors );
	&AllocateRegister( \$attenuationFactors );

	; compute attenuation amount (r2 = 'd*d d*d d*d d*d', r3 = '1/d 1/d 1/d 1/d')
	dst $attenuationFactors, $lightDistSquared, $ooLightDist						; r4 = ( 1, d, d*d, 1/d )
	&FreeRegister( \$lightDistSquared );
	&FreeRegister( \$ooLightDist );
	local( $attenuation );
	&AllocateRegister( \$attenuation );
	dp3 $attenuation, $attenuationFactors, c[a0.x+4]				; r3 = atten0 + d * atten1 + d*d * atten2

	rcp $lightDir.w, $attenuation						; $lightDir.w = 1 / (atten0 + d * atten1 + d*d * atten2)

	&FreeRegister( \$attenuationFactors );
	&FreeRegister( \$attenuation );
	
	local( $tmp );
	&AllocateRegister( \$tmp ); # FIXME : really only needs to be a scalar

	; compute n dot l, fold in distance attenutation
	dp3 $tmp.x, $lightDir, $worldNormal
	max $tmp.x, $tmp.x, c0.x				; Clamp to zero
	mul $tmp.x, $tmp.x, $lightDir.w
	if( $add )
	{
		mad $linearColor.xyz, c[a0.x], $tmp.x, $linearColor
	}
	else
	{
		mov $linearColor.xyz, c[a0.x], $tmp.x
	}

	&FreeRegister( \$lightDir );
	&FreeRegister( \$tmp ); # FIXME : really only needs to be a scalar
}

sub SpotLight
{
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;
	local( $add ) = shift;
	
	local( $lightDir );
	&AllocateRegister( \$lightDir );

	; SPOTLIGHT
	; compute light direction
	sub $lightDir, c[a0.x+2], $worldPos
	
	local( $lightDistSquared );
	local( $ooLightDist );
	&AllocateRegister( \$lightDistSquared );
	&AllocateRegister( \$ooLightDist );

	; normalize light direction, maintain temporaries for attenuation
	dp3 $lightDistSquared, $lightDir, $lightDir
	rsq $ooLightDist, $lightDistSquared.x
	mul $lightDir, $lightDir, $ooLightDist.x
	
	local( $attenuationFactors );
	&AllocateRegister( \$attenuationFactors );

	; compute attenuation amount (r2 = 'd*d d*d d*d d*d', r3 = '1/d 1/d 1/d 1/d')
	dst $attenuationFactors, $lightDistSquared, $ooLightDist						; r4 = ( 1, d, d*d, 1/d )

	&FreeRegister( \$lightDistSquared );
	&FreeRegister( \$ooLightDist );
	local( $attenuation );	&AllocateRegister( \$attenuation );

	dp3 $attenuation, $attenuationFactors, c[a0.x+4]				; r3 = atten0 + d * atten1 + d*d * atten2
	rcp $lightDir.w, $attenuation						; r1.w = 1 / (atten0 + d * atten1 + d*d * atten2)

	&FreeRegister( \$attenuationFactors );
	&FreeRegister( \$attenuation );
	
	local( $litSrc ); &AllocateRegister( \$litSrc );
	local( $tmp ); &AllocateRegister( \$tmp ); # FIXME - only needs to be scalar

	; compute n dot l
	dp3 $litSrc.x, $worldNormal, $lightDir
	
	; compute angular attenuation
	dp3 $tmp.x, c[a0.x+1], -$lightDir				; dot = -delta * spot direction
	sub $litSrc.y, $tmp.x, c[a0.x+3].z				; r2.y = dot - stopdot2
	&FreeRegister( \$tmp );
	mul $litSrc.y, $litSrc.y, c[a0.x+3].w			; r2.y = (dot - stopdot2) / (stopdot - stopdot2)
	mov $litSrc.w, c[a0.x+3].x						; r2.w = exponent
	local( $litDst ); &AllocateRegister( \$litDst );
	lit $litDst, $litSrc							; r3.y = N dot L or 0, whichever is bigger
	&FreeRegister( \$litSrc );
													; r3.z = pow((dot - stopdot2) / (stopdot - stopdot2), exponent)
	min $litDst.z, $litDst.z, $cOne		 			; clamp pow() to 1
	
	local( $tmp1 ); &AllocateRegister( \$tmp1 );
	local( $tmp2 ); &AllocateRegister( \$tmp2 );  # FIXME - could be scalar

	; fold in distance attenutation with other factors
	mul $tmp1, c[a0.x], $lightDir.w
	mul $tmp2.x, $litDst.y, $litDst.z
	if( $add )
	{
		mad $linearColor.xyz, $tmp1, $tmp2.x, $linearColor
	}
	else
	{
		mov $linearColor.xyz, $tmp1, $tmp2.x
	}

	&FreeRegister( \$lightDir );
	&FreeRegister( \$litDst );
	&FreeRegister( \$tmp1 );
	&FreeRegister( \$tmp2 );
}

sub DoLight
{
	local( $lightType ) = shift;
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;
	local( $add ) = shift;

	if( $lightType eq "spot" )
	{
		&SpotLight( $worldPos, $worldNormal, $linearColor, $add );
	}
	elsif( $lightType eq "point" )
	{
		&PointLight( $worldPos, $worldNormal, $linearColor, $add );
	}
	elsif( $lightType eq "directional" )
	{
		&DirectionalLight( $worldNormal, $linearColor, $add );
	}
	else
	{
		die "don't know about light type \"$lightType\"\n";
	}
}

sub DoLighting
{
	local( $staticLightType ) = shift;
	local( $ambientLightType ) = shift;
	local( $localLightType1 ) = shift;
	local( $localLightType2 ) = shift;
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;

	# special case for no lighting
	if( $staticLightType eq "none" && $ambientLightType eq "none" &&
		$localLightType1 eq "none" && $localLightType2 eq "none" )
	{
		return;
	}

	# special case for static lighting only
	# Don't need to bother converting to linear space in this case.
	if( $staticLightType eq "static" && $ambientLightType eq "none" &&
		$localLightType1 eq "none" && $localLightType2 eq "none" )
	{
		mov oD0, $vSpecular
		return;
	}

	alloc $linearColor
	alloc $gammaColor

	local( $add ) = 0;
	if( $staticLightType eq "static" )
	{
		; The static lighting comes in in gamma space and has also been premultiplied by $cOverbrightFactor
		; need to get it into
		; linear space so that we can do adds.
		rcp $gammaColor.w, $cOverbrightFactor
		mul $gammaColor.xyz, $vSpecular, $gammaColor.w
		&GammaToLinear( $gammaColor, $linearColor );
		$add = 1;
	}

	if( $ambientLightType eq "ambient" )
	{
		&AmbientLight( $worldNormal, $linearColor, $add );
		$add = 1;
	}

	if( $localLightType1 ne "none" )
	{
		mov a0.x, c3.x
		&DoLight( $localLightType1, $worldPos, $worldNormal, $linearColor, $add );
		$add = 1;
	}

	if( $localLightType2 ne "none" )
	{
		mov a0.x, c3.y
		&DoLight( $localLightType2, $worldPos, $worldNormal, $linearColor, $add );
		$add = 1;
	}

	;------------------------------------------------------------------------------
	; Output color (gamma correction)
	;------------------------------------------------------------------------------

	&LinearToGamma( $linearColor, $gammaColor );
	if( 0 )
	{
		mul oD0.xyz, $gammaColor.xyz, $cOverbrightFactor
	}
	else
	{
		mul $gammaColor.xyz, $gammaColor.xyz, $cOverbrightFactor
		&ColorClamp( $gammaColor, "oD0" );
	}

;	mov oD0.xyz, $linearColor
	mov oD0.w, c0.y				; make sure all components are defined

	free $linearColor
	free $gammaColor
}

sub DoDynamicLightingToLinear
{
	local( $ambientLightType ) = shift;
	local( $localLightType1 ) = shift;
	local( $localLightType2 ) = shift;
	local( $worldPos ) = shift;
	local( $worldNormal ) = shift;
	local( $linearColor ) = shift;

	# No lights at all. . note that we don't even consider static lighting here.
	if( $ambientLightType eq "none" &&
		$localLightType1 eq "none" && $localLightType2 eq "none" )
	{
		mov $linearColor, $cZero
		return;
	}

	local( $add ) = 0;
	if( $ambientLightType eq "ambient" )
	{
		&AmbientLight( $worldNormal, $linearColor, $add );
		$add = 1;
	}

	if( $localLightType1 ne "none" )
	{
		mov a0.x, c3.x
		&DoLight( $localLightType1, $worldPos, $worldNormal, $linearColor, $add );
		$add = 1;
	}

	if( $localLightType2 ne "none" )
	{
		mov a0.x, c3.y
		&DoLight( $localLightType2, $worldPos, $worldNormal, $linearColor, $add );
		$add = 1;
	}
}

