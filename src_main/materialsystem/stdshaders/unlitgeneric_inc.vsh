#include "macros.vsh"

;------------------------------------------------------------------------------
;	 c20	 = Modulation color  BUGBUGBUG
;	 c90-c91 = Base texture transform
;    c92-c93 = Mask texture transform
;    c94-c95 = Detail texture transform
;------------------------------------------------------------------------------

sub UnlitGeneric
{
	local( $detail ) = shift;
	local( $envmap ) = shift;
	local( $envmapcameraspace ) = shift;
	local( $envmapsphere ) = shift;
	local( $vertexcolor ) = shift;

	local( $worldPos, $worldNormal, $projPos, $reflectionVector );

	;------------------------------------------------------------------------------
	; Vertex blending
	;------------------------------------------------------------------------------
	&AllocateRegister( \$worldPos );
	if( $envmap )
	{
		&AllocateRegister( \$worldNormal );
		&SkinPositionAndNormal( $g_numBones, $worldPos, $worldNormal );
	}
	else
	{
		&SkinPosition( $g_numBones, $worldPos );
	}

	;------------------------------------------------------------------------------
	; Transform the position from world to proj space
	;------------------------------------------------------------------------------

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

	if( !$envmap )
	{
		&FreeRegister( \$worldPos );
	}


	
	;------------------------------------------------------------------------------
	; Texture coordinates (use world-space normal for envmap, tex transform for mask)
	;------------------------------------------------------------------------------
	dp4 oT0.x, $vTexCoord0, c90	; FIXME
	dp4 oT0.y, $vTexCoord0, c91	; FIXME

	if( $envmap )
	{
		if( $envmapcameraspace )
		{
			&AllocateRegister( \$reflectionVector );
			&ComputeReflectionVector( $worldPos, $worldNormal, $reflectionVector );

			; transform reflection vector into view space
			dp3 oT1.x, $reflectionVector, $cViewModel0
			dp3 oT1.y, $reflectionVector, $cViewModel1
			dp3 oT1.z, $reflectionVector, $cViewModel2

			&FreeRegister( \$reflectionVector );
		}
		elsif( $envmapsphere )
		{
			&AllocateRegister( \$reflectionVector );
			&ComputeReflectionVector( $worldPos, $worldNormal, $reflectionVector );
			&ComputeSphereMapTexCoords( $reflectionVector, "oT1" );

			&FreeRegister( \$reflectionVector );
		}
		else
		{
			&ComputeReflectionVector( $worldPos, $worldNormal, "oT1" );
		}

		; envmap mask
		dp4 oT2.x, $vTexCoord0, c92	; FIXME
		dp4 oT2.y, $vTexCoord0, c93 ; FIXME

		&FreeRegister( \$worldPos );
		&FreeRegister( \$worldNormal );
	}

	if( $detail )
	{
		dp4 oT3.x, $vTexCoord0, c94		; FIXME
		dp4 oT3.y, $vTexCoord0, c95		; FIXME
	}

	if( $vertexcolor )
	{
		; Modulation color
		mul oD0, $vColor, $cModulationColor
	}
	else
	{
		; Modulation color
		mov oD0, $cModulationColor
	}
}
