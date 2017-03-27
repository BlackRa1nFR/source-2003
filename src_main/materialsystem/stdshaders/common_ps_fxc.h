// This is where all common code for pixel shaders go.
#include "common_fxc.h"


HALF Luminance( HALF3 color )
{
	return dot( color, HALF3( HALF_CONSTANT(0.30f), HALF_CONSTANT(0.59f), HALF_CONSTANT(0.11f) ) );
}

HALF LuminanceScaled( HALF3 color )
{
	return dot( color, HALF3( HALF_CONSTANT(0.30f) / MAX_HDR_OVERBRIGHT, HALF_CONSTANT(0.59f) / MAX_HDR_OVERBRIGHT, HALF_CONSTANT(0.11f) / MAX_HDR_OVERBRIGHT ) );
}


HALF AvgColor( HALF3 color )
{
	return dot( color, HALF3( HALF_CONSTANT(0.33333f), HALF_CONSTANT(0.33333f), HALF_CONSTANT(0.33333f) ) );
}

HALF4 DiffuseBump( sampler lightmapSampler,
                   float2  lightmapTexCoord1,
                   float2  lightmapTexCoord2,
                   float2  lightmapTexCoord3,
                   HALF3   normal )
{
	HALF3 lightmapColor1 = tex2D( lightmapSampler, lightmapTexCoord1 );
	HALF3 lightmapColor2 = tex2D( lightmapSampler, lightmapTexCoord2 );
	HALF3 lightmapColor3 = tex2D( lightmapSampler, lightmapTexCoord3 );

	HALF3 diffuseLighting;
	diffuseLighting = saturate( dot( normal, bumpBasis[0] ) ) * lightmapColor1 +
					  saturate( dot( normal, bumpBasis[1] ) ) * lightmapColor2 +
					  saturate( dot( normal, bumpBasis[2] ) ) * lightmapColor3;

	return HALF4( diffuseLighting, LuminanceScaled( diffuseLighting ) );
}


HALF Fresnel( HALF3 normal,
              HALF3 eye,
              HALF2 scaleBias )
{
	HALF fresnel = HALF_CONSTANT(1.0f) - dot( normal, eye );
	fresnel = pow( fresnel, HALF_CONSTANT(5.0f) );

	return fresnel * scaleBias.x + scaleBias.y;
}

HALF4 GetNormal( sampler normalSampler,
                 float2 normalTexCoord )
{
	HALF4 normal = tex2D( normalSampler, normalTexCoord );
	normal.rgb = HALF_CONSTANT(2.0f) * normal.rgb - HALF_CONSTANT(1.0f);

	return normal;
}

#ifdef NV3X

HALF4 EnvReflect( sampler envmapSampler,
				 sampler normalizeSampler,
				 HALF3 normal,
				 float3 eye,
				 HALF2 fresnelScaleBias )
{
	HALF3 normEye = texCUBE( normalizeSampler, eye );
	HALF fresnel = Fresnel( normal, normEye, fresnelScaleBias );
	HALF3 reflect = CalcReflectionVectorUnnormalized( normal, eye );
	return texCUBE( envmapSampler, reflect );
}

#else // NV3X

HALF4 EnvReflect( sampler envmapSampler,
				 HALF3 normal,
				 HALF3 eye,
				 HALF2 fresnelScaleBias )
{
	eye = normalize( eye );
	
	HALF fresnel = Fresnel( normal, eye, fresnelScaleBias );
	HALF3 reflect = CalcReflectionVectorUnnormalized( normal, eye );
	
	return texCUBE( envmapSampler, reflect ) * fresnel;
}

#endif
