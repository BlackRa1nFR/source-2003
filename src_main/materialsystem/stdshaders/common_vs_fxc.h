// This is where all common code for pixel shaders go.
#include "common_fxc.h"

#define LIGHTTYPE_NONE				0
#define LIGHTTYPE_STATIC			1
#define LIGHTTYPE_SPOT				2
#define LIGHTTYPE_POINT				3
#define LIGHTTYPE_DIRECTIONAL		4
#define LIGHTTYPE_AMBIENT			5

static const int g_StaticLightTypeArray[22] = {
	LIGHTTYPE_NONE, LIGHTTYPE_STATIC, 
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, 
	LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC, LIGHTTYPE_STATIC
};
 
static const int g_AmbientLightTypeArray[22] = {
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, 
	LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, 
	LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, 
	LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT, LIGHTTYPE_AMBIENT
};

static const int g_LocalLightType0Array[22] = {
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_NONE, LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_SPOT, LIGHTTYPE_SPOT, 
	LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL,
	LIGHTTYPE_NONE, LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_SPOT, LIGHTTYPE_SPOT, 
	LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL
};

static const int g_LocalLightType1Array[22] = {
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, 
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_SPOT, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_DIRECTIONAL,
	LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_NONE, LIGHTTYPE_SPOT, LIGHTTYPE_POINT, 
	LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_POINT, LIGHTTYPE_DIRECTIONAL, LIGHTTYPE_DIRECTIONAL
};

#define FOGTYPE_RANGE				0
#define FOGTYPE_HEIGHT				1

#define COMPILE_ERROR ( 1/0; )

// -------------------------
// CONSTANTS
// -------------------------
//const float4 cConstants0				: register(c0);

#define cZero	0.0f
#define cOne	1.0f
#define cTwo	2.0f
#define cHalf	0.5f

#pragma def ( vs, c0, 0.0f, 1.0f, 2.0f, 0.5f )

const float4 cConstants1				: register(c1);
#define cOOGamma			cConstants1.x
#define cOverbright			cConstants1.y
#define cOneThird			cConstants1.z
#define cOOOverbright		cConstants1.w

const float4 cEyePosWaterZ				: register(c2);
#define cEyePos			cEyePosWaterZ.xyz

// This is still used by asm stuff.
const float4 cObsoleteLightIndex		: register(c3);

const float4x4 cModelViewProj			: register(c4);
const float4x4 cViewProj				: register(c8);
const float4x4 cModelView				: register(c12);

const float4 cFogParams					: register(c16);
#define cFogEndOverFogRange cFogParams.x
#define cFogOne cFogParams.y
#define cHeightClipZ cFogParams.z
#define cOOFogRange cFogParams.w

const float4x4 cViewModel				: register(c17);

const float3 cAmbientCube[6]			: register(c21);

struct LightInfo
{
	float4 color;
	float4 dir;
	float4 pos;
	float4 spotParams;
	float4 atten;
};

LightInfo cLightInfo[2]					: register(c27);

const float4 cClipParams				: register(c37);
#define cClipDirection cClipParams.x
#define cClipDirectionTimesHeightClipZ cClipParams.y

const float4 cModulationColor			: register(c38);

static const int cModel0Index = 42;
const float4x3 cModel[16]				: register(c42);

// last cmodel is c89
// c90-c95 are reserved for shader specific constants. .need to make sure that we don't
// shadow these, or we need to reserve them here.

float RangeFog( const float3 projPos )
{
	return -projPos.z * cOOFogRange + cFogEndOverFogRange;
}

float WaterFog( const float3 worldPos, const float3 projPos )
{
	float4 tmp;
	
	tmp.xy = cEyePosWaterZ.wz - worldPos.z;

	// tmp.x is the distance from the water surface to the vert
	// tmp.y is the distance from the eye position to the vert

	// if $tmp.x < 0, then set it to 0
	// This is the equivalent of moving the vert to the water surface if it's above the water surface
	
	tmp.x = max( cZero, tmp.x );

	// $tmp.w = $tmp.x / $tmp.y
	tmp.w = tmp.x / tmp.y;

	tmp.w *= projPos.z;

	// $tmp.w is now the distance that we see through water.

	return -tmp.w * cOOFogRange + cFogOne;
}

float CalcFog( const float3 worldPos, const float3 projPos, const int fogType )
{
	if( fogType == FOGTYPE_RANGE )
	{
		return RangeFog( projPos );
	}
	else
	{
		return WaterFog( worldPos, projPos );
	}
}

void SkinPosition( int numBones, const float4 modelPos, 
                   const float4 boneWeights, float4 fBoneIndices,
				   out float3 worldPos )
{
	int3 boneIndices = D3DCOLORtoUBYTE4( fBoneIndices );

	if( numBones == 0 )
	{
		worldPos = mul4x3( modelPos, cModel[0] );
	}
	else if( numBones == 1 )
	{
		worldPos = mul4x3( modelPos, cModel[boneIndices[0]] );
	}
	else if( numBones == 2 )
	{
		float4x3 blendMatrix = cModel[boneIndices[0]] * boneWeights[0] +
							   cModel[boneIndices[1]] * boneWeights[1];
		worldPos = mul4x3( modelPos, blendMatrix );
	}
	else // if( numBones == 3 )
	{
		float4x3 mat1 = cModel[boneIndices[0]];
		float4x3 mat2 = cModel[boneIndices[1]];
		float4x3 mat3 = cModel[boneIndices[2]];

		float weight2 = 1.0f - boneWeights[0] - boneWeights[1];

		float4x3 blendMatrix = mat1 * boneWeights[0] + mat2 * boneWeights[1] + mat3 * weight2;
		worldPos = mul4x3( modelPos, blendMatrix );
	}
}

void SkinPositionAndNormal( int numBones, const float4 modelPos, const float3 modelNormal,
                            const float4 boneWeights, float4 fBoneIndices,
						    out float3 worldPos, out float3 worldNormal )
{
	int3 boneIndices = D3DCOLORtoUBYTE4( fBoneIndices );

	if( numBones == 0 )
	{
		worldPos = mul4x3( modelPos, cModel[0] );
		worldNormal = mul3x3( modelNormal, ( const float3x3 )cModel[0] );
	}
	else if( numBones == 1 )
	{
		worldPos = mul4x3( modelPos, cModel[boneIndices[0]] );
		worldNormal = mul3x3( modelNormal, ( const float3x3 )cModel[boneIndices[0]] );
	}
	else if( numBones == 2 )
	{
		float4x3 blendMatrix = cModel[boneIndices[0]] * boneWeights[0] +
							   cModel[boneIndices[1]] * boneWeights[1];
		worldPos = mul4x3( modelPos, blendMatrix );
		worldNormal = mul3x3( modelNormal, ( float3x3 )blendMatrix );
	}
	else // if( numBones == 3 )
	{
		float4x3 mat1 = cModel[boneIndices[0]];
		float4x3 mat2 = cModel[boneIndices[1]];
		float4x3 mat3 = cModel[boneIndices[2]];

		float weight2 = 1.0f - boneWeights[0] - boneWeights[1];

		float4x3 blendMatrix = mat1 * boneWeights[0] + mat2 * boneWeights[1] + mat3 * weight2;
		worldPos = mul4x3( modelPos, blendMatrix );
		worldNormal = mul3x3( modelNormal, ( float3x3 )blendMatrix );
	}
}

// Is it worth keeping SkinPosition and SkinPositionAndNormal around since the optimizer
// gets rid of anything that isn't used?
void SkinPositionNormalAndTangentSpace( 
#ifdef USE_CONDITIONALS
						   bool bZeroBones, bool bOneBone, bool bTwoBones,
#else
						   int numBones, 
#endif
						    const float4 modelPos, const float3 modelNormal, 
							const float4 modelTangentS,
                            const float4 boneWeights, float4 fBoneIndices,
						    out float3 worldPos, out float3 worldNormal, 
							out float3 worldTangentS, out float3 worldTangentT )
{
	int3 boneIndices = D3DCOLORtoUBYTE4( fBoneIndices );

#ifdef USE_CONDITIONALS
	if( bZeroBones )
#else
	if( numBones == 0 )
#endif
	{
//		worldPos = mul( float4( modelPos, 1.0f ), cModel[0] );
		worldPos = mul4x3( modelPos, cModel[0] );
		worldNormal = mul3x3( modelNormal, ( const float3x3 )cModel[0] );
		worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )cModel[0] );
	}
#ifdef USE_CONDITIONALS
	else if( bOneBone )
#else
	else if( numBones == 1 )
#endif
	{
		worldPos = mul4x3( modelPos, cModel[boneIndices[0]] );
		worldNormal = mul3x3( modelNormal, ( const float3x3 )cModel[boneIndices[0]] );
		worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )cModel[boneIndices[0]] );
	}
#ifdef USE_CONDITIONALS
	else if( bTwoBones )
#else
	else if( numBones == 2 )
#endif
	{
		float4x3 blendMatrix = cModel[boneIndices[0]] * boneWeights[0] +
							   cModel[boneIndices[1]] * boneWeights[1];
		worldPos = mul4x3( modelPos, blendMatrix );
		worldNormal = mul3x3( modelNormal, ( float3x3 )blendMatrix );
		worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )blendMatrix );
	}
	else // if( numBones == 3 )
	{
		float4x3 mat1 = cModel[boneIndices[0]];
		float4x3 mat2 = cModel[boneIndices[1]];
		float4x3 mat3 = cModel[boneIndices[2]];

		float weight2 = 1.0f - boneWeights[0] - boneWeights[1];

		float4x3 blendMatrix = mat1 * boneWeights[0] + mat2 * boneWeights[1] + mat3 * weight2;
		worldPos = mul4x3( modelPos, blendMatrix );
		worldNormal = mul3x3( modelNormal, ( float3x3 )blendMatrix );
		worldTangentS = mul3x3( ( float3 )modelTangentS, ( const float3x3 )blendMatrix );
	}
	worldTangentT = cross( worldNormal, worldTangentS ) * modelTangentS.w;
}

// FIXME: change this to half3 when ATI fixes their driver bug.
// FIXME: Try to get this to generate a lit instruction
float3 GammaToLinear( const float3 gamma )
{
#if 0
	return 0.962491f * gamma * gamma;
#else
	return pow( gamma, 1.0f / cOOGamma );
#endif
}

// FIXME: change this to half3 when ATI fixes their driver bug.
// FIXME: Try to get this to generate a lit instruction
float3 LinearToGamma( const float3 linear )
{
#if 0
	// This is a good approx.
	static const float a = 2.314217438285954f;
	static const float b = -4.377606146152142f;
	static const float c = 3.116082349104176f;

	float3 gamma, tmp;
	tmp = a * linear + b;
	tmp = tmp * linear + c;
	gamma = tmp * linear;
	return gamma;
#else
	return pow( linear, cOOGamma );
#endif
}

float3 AmbientLight( const float3 worldNormal )
{
	float3 nSquared = worldNormal * worldNormal;
	int3 isNegative = ( worldNormal < 0.0 );
	float3 linearColor;
	linearColor = nSquared.x * cAmbientCube[isNegative.x] +
	              nSquared.y * cAmbientCube[isNegative.y+2] +
	              nSquared.z * cAmbientCube[isNegative.z+4];
	return linearColor;
}

float3 SpotLight( const float3 worldPos, const float3 worldNormal, int lightNum )
{
	// Direct mapping of current code
	float3 lightDir = cLightInfo[lightNum].pos - worldPos;

	// normalize light direction, maintain temporaries for attenuation
	float lightDistSquared = dot( lightDir, lightDir );
	float ooLightDist = rsqrt( lightDistSquared );
	lightDir *= ooLightDist;
	
	float3 attenuationFactors;
	attenuationFactors = dst( lightDistSquared, ooLightDist );

	float flDistanceAttenuation = dot( attenuationFactors, cLightInfo[lightNum].atten );
	flDistanceAttenuation = 1.0f / flDistanceAttenuation;
	
	// There's an additional falloff we need to make to get the edges looking good
	// and confine the light within a sphere.
	float flLinearFactor = saturate( 1.0f - lightDistSquared * cLightInfo[lightNum].atten.w ); 
	flDistanceAttenuation *= flLinearFactor;

	// compute n dot l
	float nDotL = dot( worldNormal, lightDir );
	nDotL = max( cZero, nDotL );
						 
	// compute angular attenuation
	float flCosTheta = dot( cLightInfo[lightNum].dir, -lightDir );
	float flAngularAtten = (flCosTheta - cLightInfo[lightNum].spotParams.z) * cLightInfo[lightNum].spotParams.w;
	flAngularAtten = max( cZero, flAngularAtten );
	flAngularAtten = pow( flAngularAtten, cLightInfo[lightNum].spotParams.x );
	flAngularAtten = min( cOne, flAngularAtten );

	return cLightInfo[lightNum].color * flDistanceAttenuation * flAngularAtten * nDotL;
}

float3 PointLight( const float3 worldPos, const float3 worldNormal, int lightNum )
{
	// Get light direction
	float3 lightDir = cLightInfo[lightNum].pos - worldPos;

	// Get light distance squared.
	float lightDistSquared = dot( lightDir, lightDir );

	// Get 1/lightDistance
	float ooLightDist = rsqrt( lightDistSquared );

	// Normalize light direction
	lightDir *= ooLightDist;

	// compute distance attenuation factors.
	float3 attenuationFactors;
	attenuationFactors.x = 1.0f;
	attenuationFactors.y = lightDistSquared * ooLightDist;
	attenuationFactors.z = lightDistSquared;

	float flDistanceAtten = 1.0f / dot( cLightInfo[lightNum].atten.xyz, attenuationFactors );

	// There's an additional falloff we need to make to get the edges looking good
	// and confine the light within a sphere.
	float flLinearFactor = saturate( 1.0f - lightDistSquared * cLightInfo[lightNum].atten.w ); 
	flDistanceAtten *= flLinearFactor;

	// Compute N dot L
	float NDotL = dot( worldNormal, lightDir );
	NDotL = max( cZero, NDotL );

	return cLightInfo[lightNum].color * NDotL * flDistanceAtten;
}

float3 DirectionalLight( const float3 worldNormal, int lightNum )
{
	// Compute N dot L
	float NDotL = dot( worldNormal, -cLightInfo[lightNum].dir );
	NDotL = max( cZero, NDotL );
	return cLightInfo[lightNum].color * NDotL;
}

float3 DoLight( const float3 worldPos, const float3 worldNormal, 
				int lightNum, int lightType )
{
	float3 color = 0.0f;
	if( lightType == LIGHTTYPE_SPOT )
	{
		color = SpotLight( worldPos, worldNormal, lightNum );
	}
	else if( lightType == LIGHTTYPE_POINT )
	{
		color = PointLight( worldPos, worldNormal, lightNum );
	}
	else if( lightType == LIGHTTYPE_DIRECTIONAL )
	{
		color = DirectionalLight( worldNormal, lightNum );
	}
	return color;
}

float3 DoLightingLinear( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const int staticLightType,
				   const int ambientLightType, const int localLightType0,
				   const int localLightType1 )
{
	float3 linearColor = 0.0f;
	if( staticLightType == LIGHTTYPE_STATIC )
	{
		// The static lighting comes in in gamma space and has also been premultiplied by $cOOOverbright
		// need to get it into
		// linear space so that we can do adds.
		linearColor += GammaToLinear( staticLightingColor * cOverbright );
	}

	if( ambientLightType == LIGHTTYPE_AMBIENT )
	{
		linearColor += AmbientLight( worldNormal );
	}

	if( localLightType0 != LIGHTTYPE_NONE )
	{
		linearColor += DoLight( worldPos, worldNormal, 0, localLightType0 );
	}

	if( localLightType1 != LIGHTTYPE_NONE )
	{
		linearColor += DoLight( worldPos, worldNormal, 1, localLightType1 );
	}

	return linearColor;
}


float3 DoLighting( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const int staticLightType,
				   const int ambientLightType, const int localLightType0,
				   const int localLightType1, const float modulation )
{
	float3 returnColor;

	// special case for no lighting
	if( staticLightType == LIGHTTYPE_NONE && 
		ambientLightType == LIGHTTYPE_NONE &&
		localLightType0 == LIGHTTYPE_NONE &&
		localLightType1 == LIGHTTYPE_NONE )
	{
		returnColor = float3( 0.0f, 0.0f, 0.0f );
	}
	else if( staticLightType == LIGHTTYPE_STATIC && 
			 ambientLightType == LIGHTTYPE_NONE &&
			 localLightType0 == LIGHTTYPE_NONE &&
			 localLightType1 == LIGHTTYPE_NONE )
	{
		// special case for static lighting only
		// Don't need to bother converting to linear space in this case.
		returnColor = staticLightingColor;
	}
	else
	{
		float3 linearColor = DoLightingLinear( worldPos, worldNormal, staticLightingColor, 
			staticLightType, ambientLightType, localLightType0, localLightType1 );

		if (modulation != 1.0f)
		{
			linearColor *= modulation;
		}

		// for dx9, we don't need to scale back down to 0..1 for overbrighting.
		// FIXME: But we're going to because there's some visual difference between dx8 + dx9 if we don't
		// gotta look into that later.
		returnColor = HuePreservingColorClamp( cOOOverbright * LinearToGamma( linearColor ) );
	}

	return returnColor;
}

// returns a linear HDR light value
float3 DoLightingHDR( const float3 worldPos, const float3 worldNormal,
				   const float3 staticLightingColor, const int staticLightType,
				   const int ambientLightType, const int localLightType0,
				   const int localLightType1, const float modulation )
{
	float3 returnColor;

	// special case for no lighting
	if( staticLightType == LIGHTTYPE_NONE && 
		ambientLightType == LIGHTTYPE_NONE &&
		localLightType0 == LIGHTTYPE_NONE &&
		localLightType1 == LIGHTTYPE_NONE )
	{
		returnColor = float3( 0.0f, 0.0f, 0.0f );
	}
	else if( staticLightType == LIGHTTYPE_STATIC && 
			 ambientLightType == LIGHTTYPE_NONE &&
			 localLightType0 == LIGHTTYPE_NONE &&
			 localLightType1 == LIGHTTYPE_NONE )
	{
		// special case for static lighting only
		// FIXME!!  Should store HDR values per vertex for static prop lighting.
		returnColor = GammaToLinear( staticLightingColor * cOverbright );
	}
	else
	{
		float3 linearColor = DoLightingLinear( worldPos, worldNormal, staticLightingColor, 
			staticLightType, ambientLightType, localLightType0, localLightType1 );

		if (modulation != 1.0f)
		{
			linearColor *= modulation;
		}

		returnColor = linearColor;
	}

	return returnColor;
}

void CalculateWorldBumpBasis( const float3 worldTangentS, const float3 worldTangentT,
							 const float3 worldNormal, out float3 worldBumpBasis1,
							 out float3 worldBumpBasis2, out float3 worldBumpBasis3 )
{
	worldBumpBasis1.x = dot( bumpBasisTranspose[0], worldTangentS );
	worldBumpBasis1.y = dot( bumpBasisTranspose[0], worldTangentT );
	worldBumpBasis1.z = dot( bumpBasisTranspose[0], worldNormal );

	worldBumpBasis2.x = dot( bumpBasisTranspose[1], worldTangentS );
	worldBumpBasis2.y = dot( bumpBasisTranspose[1], worldTangentT );
	worldBumpBasis2.z = dot( bumpBasisTranspose[1], worldNormal );

	worldBumpBasis3.x = dot( bumpBasisTranspose[2], worldTangentS );
	worldBumpBasis3.y = dot( bumpBasisTranspose[2], worldTangentT );
	worldBumpBasis3.z = dot( bumpBasisTranspose[2], worldNormal );
}

void AddBumpedAmbientLight( const float3 worldBumpBasis1, const float3 worldBumpBasis2,
							const float3 worldBumpBasis3, const float3 worldNormal,
							inout float3 color1, inout float3 color2, inout float3 color3, 
							inout float3 normalColor )
{
	float3 nSquared;
	int3 isNegative;

	nSquared = worldBumpBasis1 * worldBumpBasis1;
	isNegative = ( worldBumpBasis1 < 0.0 );
	color1 += nSquared.x * cAmbientCube[isNegative.x] +
		      nSquared.y * cAmbientCube[isNegative.y+2] +
			  nSquared.z * cAmbientCube[isNegative.z+4];

	nSquared = worldBumpBasis2 * worldBumpBasis2;
	isNegative = ( worldBumpBasis2 < 0.0 );
	color2 += nSquared.x * cAmbientCube[isNegative.x] +
		      nSquared.y * cAmbientCube[isNegative.y+2] +
			  nSquared.z * cAmbientCube[isNegative.z+4];

	nSquared = worldBumpBasis3 * worldBumpBasis3;
	isNegative = ( worldBumpBasis3 < 0.0 );
	color3 += nSquared.x * cAmbientCube[isNegative.x] +
		      nSquared.y * cAmbientCube[isNegative.y+2] +
			  nSquared.z * cAmbientCube[isNegative.z+4];

	nSquared = worldNormal * worldNormal;
	isNegative = ( worldNormal < 0.0 );
	normalColor += nSquared.x * cAmbientCube[isNegative.x] +
		      nSquared.y * cAmbientCube[isNegative.y+2] +
			  nSquared.z * cAmbientCube[isNegative.z+4];
}

void AddBumpedSpotLight( const float3 worldPos, int lightNum, const float3 worldBumpBasis1,
						const float3 worldBumpBasis2, const float3 worldBumpBasis3, const float3 worldNormal,
						inout float3 color1, inout float3 color2, inout float3 color3,
						inout float3 normalColor )
{
	// Get light direction
	float3 lightDir = cLightInfo[lightNum].pos - worldPos;

	// Get light distance squared.
	float lightDistSquared = dot( lightDir, lightDir );

	// Get 1/lightDistance
	float ooLightDist = rsqrt( lightDistSquared );

	// Normalize light direction
	lightDir *= ooLightDist;

	// compute distance attenuation factors.
	float3 attenuationFactors;
	attenuationFactors.x = 1.0f;
	attenuationFactors.y = lightDistSquared * ooLightDist;
	attenuationFactors.z = lightDistSquared;

	float3 distanceAtten = 1.0f / dot( cLightInfo[lightNum].atten, attenuationFactors );

	// Compute N dot L
	float4 lambertAtten = float4( dot( worldBumpBasis1, lightDir ),
		                          dot( worldBumpBasis2, lightDir ),
				 				  dot( worldBumpBasis3, lightDir ),
								  dot( worldNormal, lightDir ) );
	lambertAtten = max( lambertAtten, 0.0 );

	// Compute angular attenuation
	float exponent = cLightInfo[lightNum].spotParams.x;
	float stopdot = cLightInfo[lightNum].spotParams.y;
	float stopdot2 = cLightInfo[lightNum].spotParams.z;
	float ooStopDotMinusStopDot2 = cLightInfo[lightNum].spotParams.w;

	float lightDot = dot( -lightDir, cLightInfo[lightNum].dir );

	// compute the angular attenuation based on the lights params and the position of
	// the point that we are lighting.
	float flAngularAtten = ( lightDot - stopdot2 ) * ooStopDotMinusStopDot2;
	flAngularAtten = max( cZero, flAngularAtten );
	flAngularAtten = pow( flAngularAtten, exponent );
	flAngularAtten = min( flAngularAtten, cOne );

	float3 color = cLightInfo[lightNum].color * flAngularAtten * distanceAtten;
	color1 += color * lambertAtten.x;
	color2 += color * lambertAtten.y;
	color3 += color * lambertAtten.z;
	normalColor += color * lambertAtten.w;
}

void AddBumpedPointLight( const float3 worldPos, int lightNum, const float3 worldBumpBasis1,
						const float3 worldBumpBasis2, const float3 worldBumpBasis3, const float3 worldNormal,
						inout float3 color1, inout float3 color2, inout float3 color3, 
						inout float3 normalColor )
{
	// Get light direction
	float3 lightDir = cLightInfo[lightNum].pos - worldPos;

	// Get light distance squared.
	float lightDistSquared = dot( lightDir, lightDir );

	// Get 1/lightDistance
	float ooLightDist = rsqrt( lightDistSquared );

	// Normalize light direction
	lightDir *= ooLightDist;

	// compute distance attenuation factors.
	float4 attenuationFactors;
	attenuationFactors.x = 1.0f;
	attenuationFactors.y = lightDistSquared * ooLightDist;
	attenuationFactors.z = lightDistSquared;
	attenuationFactors.w = ooLightDist;

	float4 distanceAtten = 1.0f / dot( cLightInfo[lightNum].atten, attenuationFactors );

	// Compute N dot L
	float4 lambertAtten = float4( dot( worldBumpBasis1, lightDir ),
		                          dot( worldBumpBasis2, lightDir ),
				 				  dot( worldBumpBasis3, lightDir ),
								  dot( worldNormal, lightDir ) );
	lambertAtten = max( lambertAtten, 0.0 );

	float3 color = cLightInfo[lightNum].color * distanceAtten;
	color1 += color * lambertAtten.x;
	color2 += color * lambertAtten.y;
	color3 += color * lambertAtten.z;
	normalColor += color * lambertAtten.w;
}

void AddBumpedDirectionalLight( int lightNum, const float3 worldBumpBasis1,
						const float3 worldBumpBasis2, const float3 worldBumpBasis3, const float3 worldNormal,
						inout float3 color1, inout float3 color2, inout float3 color3, 
						inout float3 normalColor )
{
/*
	// Transform light direction into tangent space.
	float3 tangentLightDirection;
	tangentLightDirection.x = dot( worldBumpBasis1, -cLightInfo[lightNum].dir );
	tangentLightDirection.y = dot( worldBumpBasis2, -cLightInfo[lightNum].dir );
	tangentLightDirection.z = dot( worldBumpBasis3, -cLightInfo[lightNum].dir );

	float4 directionalAttenuation;
	directionalAttenuation.x = dot( bumpBasis[0], tangentLightDirection );
	directionalAttenuation.y = dot( bumpBasis[1], tangentLightDirection );
	directionalAttenuation.z = dot( bumpBasis[2], tangentLightDirection );
	directionalAttenuation.w = dot( float3( 0.0f, 0.0f, 1.0f ), tangentLightDirection );
	directionalAttenuation = max( directionalAttenuation, 0.0f );

	color1 += directionalAttenuation.x * cLightInfo[lightNum].color;
	color2 += directionalAttenuation.y * cLightInfo[lightNum].color;
	color3 += directionalAttenuation.z * cLightInfo[lightNum].color;
	normalColor += directionalAttenuation.w * cLightInfo[lightNum].color;
*/
	float4 NDotL;
	NDotL.x = dot( worldBumpBasis1, -cLightInfo[lightNum].dir );
	NDotL.y = dot( worldBumpBasis2, -cLightInfo[lightNum].dir );
	NDotL.z = dot( worldBumpBasis3, -cLightInfo[lightNum].dir );
	NDotL.w = dot( worldNormal, -cLightInfo[lightNum].dir );
	NDotL = max( 0.0f, NDotL );
	color1 += NDotL.x * cLightInfo[lightNum].color;
	color2 += NDotL.y * cLightInfo[lightNum].color;
	color3 += NDotL.z * cLightInfo[lightNum].color;
	normalColor += NDotL.w * cLightInfo[lightNum].color;
}

void AddBumpedLight( const float3 worldPos, const float3 worldNormal,
					 const float3 worldBumpBasis1, const float3 worldBumpBasis2,
					 const float3 worldBumpBasis3,
					 int lightNum, int lightType, 
					 inout float3 color1, inout float3 color2, inout float3 color3, 
					 inout float3 normalColor )
{
	if( lightType == LIGHTTYPE_SPOT )
	{
		AddBumpedSpotLight( worldPos, lightNum, worldBumpBasis1, worldBumpBasis2, worldBumpBasis3, 
			worldNormal, color1, color2, color3, normalColor );
	}
	else if( lightType == LIGHTTYPE_POINT )
	{
		AddBumpedPointLight( worldPos, lightNum, worldBumpBasis1, worldBumpBasis2, worldBumpBasis3, 
			worldNormal, color1, color2, color3, normalColor );
	}
	else
	{
		AddBumpedDirectionalLight( lightNum, worldBumpBasis1, worldBumpBasis2, worldBumpBasis3, 
			worldNormal, color1, color2, color3, normalColor );
	}
}

void DoBumpedLightingLinear( const float3 worldPos, const float3 worldNormal,
						 const float3 worldTangentS, const float3 worldTangentT,
					     const float3 staticLightingColor1, 
					     const float3 staticLightingColor2, 
					     const float3 staticLightingColor3, 
						 const float3 staticLightingColorNormal,
					     const int staticLightType,
					     const int ambientLightType, const int localLightType0,
					     const int localLightType1,
					     out float3 color1, out float3 color2, out float3 color3, out float3 normalColor )
{
	float3 worldBumpBasis1, worldBumpBasis2, worldBumpBasis3;
	CalculateWorldBumpBasis( worldTangentS, worldTangentT, worldNormal,
							 worldBumpBasis1, worldBumpBasis2, worldBumpBasis3 );

	if( staticLightType == LIGHTTYPE_STATIC )
	{
		// The static lighting comes in in gamma space and has also been premultiplied by $cOOOverbright
		// need to get it into
		// linear space so that we can do adds.
		color1 = GammaToLinear( staticLightingColor1 * cOverbright );
		color2 = GammaToLinear( staticLightingColor2 * cOverbright );
		color3 = GammaToLinear( staticLightingColor3 * cOverbright );
		normalColor = GammaToLinear( staticLightingColorNormal * cOverbright );
	}
	else
	{
		color1 = color2 = color3 = 0.0f;
		normalColor = 0.0f;
	}

	if( ambientLightType == LIGHTTYPE_AMBIENT )
	{
		AddBumpedAmbientLight( worldBumpBasis1, worldBumpBasis2, worldBumpBasis3, worldNormal,
				               color1, color2, color3, normalColor );
	}

	if( localLightType0 != LIGHTTYPE_NONE )
	{
		AddBumpedLight( worldPos, worldNormal, 
			worldBumpBasis1, worldBumpBasis2, worldBumpBasis3,
			0, localLightType0, color1, color2, color3, normalColor );
	}

	if( localLightType1 != LIGHTTYPE_NONE )
	{
		AddBumpedLight( worldPos, worldNormal, 
			worldBumpBasis1, worldBumpBasis2, worldBumpBasis3,
			1, localLightType1, color1, color2, color3, normalColor );
	}
}

void DoBumpedLighting( const float3 worldPos, const float3 worldNormal,
						 const float3 worldTangentS, const float3 worldTangentT,
					     const float3 staticLightingColor1, 
					     const float3 staticLightingColor2, 
					     const float3 staticLightingColor3, 
						 const float3 staticLightingColorNormal,
					     const int staticLightType,
					     const int ambientLightType, const int localLightType0,
					     const int localLightType1,
						 const float modulation,
					     out float3 color1, out float3 color2, out float3 color3,
						 out float3 gammaColorNormal )
{	
	// special case for no lighting
	if( staticLightType == LIGHTTYPE_NONE && 
		ambientLightType == LIGHTTYPE_NONE &&
		localLightType0 == LIGHTTYPE_NONE &&
		localLightType1 == LIGHTTYPE_NONE )
	{
		// do nothing;
		color1 = color2 = color3 = 0.0f;
		gammaColorNormal = 0.0f;
	}
	else if( staticLightType == LIGHTTYPE_STATIC && 
			 ambientLightType == LIGHTTYPE_NONE &&
			 localLightType0 == LIGHTTYPE_NONE &&
			 localLightType1 == LIGHTTYPE_NONE )
	{
		/// special case for static lighting only
		// Don't need to bother converting to linear space in this case.
		color1 = staticLightingColor1;
		color2 = staticLightingColor2;
		color3 = staticLightingColor3;
		gammaColorNormal = ( staticLightingColor1 + staticLightingColor2 + 
			staticLightingColor3 ) * ( 1.0f / 3.0f );
	}
	else
	{
		float3 normalColor;

		DoBumpedLightingLinear( worldPos, worldNormal, worldTangentS, worldTangentT,
			staticLightingColor1, staticLightingColor2, staticLightingColor3, staticLightingColorNormal,
			staticLightType, ambientLightType, localLightType0, localLightType1, 
			color1, color2, color3, normalColor );

		if (modulation != 1.0f)
		{
			color1 *= modulation;
			color2 *= modulation;
			color3 *= modulation;
		}

		// TODO: may want to add colorclamp here.
		float3 averageColor = ( color1 + color2 + color3 ) * OO_SQRT_3;
		gammaColorNormal = LinearToGamma( normalColor );
		gammaColorNormal *= cOOOverbright;

		float3 correctionFactor = gammaColorNormal / averageColor;
		color1 *= correctionFactor;
		color2 *= correctionFactor;
		color3 *= correctionFactor;
	}
}

void DoBumpedLightingHDR( const float3 worldPos, const float3 worldNormal,
						 const float3 worldTangentS, const float3 worldTangentT,
					     const float3 staticLightingColor1, 
					     const float3 staticLightingColor2, 
					     const float3 staticLightingColor3, 
						 const float3 staticLightingColorNormal,
					     const int staticLightType,
					     const int ambientLightType, const int localLightType0,
					     const int localLightType1,
						 const float modulation,
					     out float3 color1, out float3 color2, out float3 color3 )
{	
	// special case for no lighting
	if( staticLightType == LIGHTTYPE_NONE && 
		ambientLightType == LIGHTTYPE_NONE &&
		localLightType0 == LIGHTTYPE_NONE &&
		localLightType1 == LIGHTTYPE_NONE )
	{
		// do nothing;
		color1 = color2 = color3 = 0.0f;
	}
	else if( staticLightType == LIGHTTYPE_STATIC && 
			 ambientLightType == LIGHTTYPE_NONE &&
			 localLightType0 == LIGHTTYPE_NONE &&
			 localLightType1 == LIGHTTYPE_NONE )
	{
		/// special case for static lighting only
		// Don't need to bother converting to linear space in this case.
		color1 = GammaToLinear( staticLightingColor1 * cOverbright );
		color2 = GammaToLinear( staticLightingColor2 * cOverbright );
		color3 = GammaToLinear( staticLightingColor3 * cOverbright );
	}
	else
	{
		float3 normalColor;

		DoBumpedLightingLinear( worldPos, worldNormal, worldTangentS, worldTangentT,
			staticLightingColor1, staticLightingColor2, staticLightingColor3, staticLightingColorNormal,
			staticLightType, ambientLightType, localLightType0, localLightType1, 
			color1, color2, color3, normalColor );

		if (modulation != 1.0f)
		{
			color1 *= modulation;
			color2 *= modulation;
			color3 *= modulation;
		}
	}
}

float DoHeightClip( const float3 worldPos )
{
	return -cClipDirection * worldPos.z + cClipDirectionTimesHeightClipZ;
}

int4 FloatToInt( in float4 floats )
{
	return D3DCOLORtoUBYTE4( floats.zyxw / 255.001953125 );
}
