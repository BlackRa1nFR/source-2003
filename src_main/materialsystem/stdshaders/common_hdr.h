#include "common_ps_fxc.h"

// We use this from textures unless the texture is a render target.
float3 DecompressHDRFromTexture( float4 input )
{
	return input.rgb * input.a * MAX_HDR_OVERBRIGHT;
}

// We only use this when building cube maps. . will have to disable transclucent rendering
// when building cubemaps if we use this.
float4 CompressHDRToTexture( float3 input )
{
	// FIXME: want to use min so that we clamp to white, but what happens if we 
	// have an albedo component that's less than 1/MAX_HDR_OVERBRIGHT?
	//	float fMax = max( max( color.r, color.g ), color.b );
	float4 output;
	float fMax = min( min( input.r, input.g ), input.b );
//	float fMax = max( max( input.r, input.g ), input.b );
	if( fMax > 1.0f )
	{
		float oofMax = 1.0f / fMax;
		output.rgb = oofMax * input.rgb;
		output.a = min( fMax / MAX_HDR_OVERBRIGHT, 1.0f );
	}
	else
	{
		output.rgb = input.rgb;
		output.a = 1.0f / MAX_HDR_OVERBRIGHT;
	}
	return output;
}

// This is used when reading from a render target (water, for instance.)
float3 DecompressHDRFromRenderTarget( float4 input )
{
	float lumRGB = Luminance( input.rgb );
	float lumA = input.a * MAX_HDR_OVERBRIGHT;
	// FIXME: tweak this value.
	if( lumA > lumRGB * 1.1f )
	{
		return input.rgb * ( lumA / lumRGB );
	}
	else
	{
		return input.rgb;
	}
}

// This is used for almost all rendering so that alpha blending will work properly.
float4 CompressHDRToRenderTarget( float3 input )
{
	float lum = Luminance( input ) * ( 1.0f / MAX_HDR_OVERBRIGHT );
	// go ahead and let the hardware clamp the value here!!!!
	return float4( input, lum );
}

