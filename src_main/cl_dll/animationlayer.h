//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef ANIMATIONLAYER_H
#define ANIMATIONLAYER_H
#ifdef _WIN32
#pragma once
#endif

struct AnimationLayer_t
{
	int		nSequence;
	float	flCycle;
	float	flPlaybackrate;
	float	flWeight;
	int		nOrder;

	float	flAnimtime;
	float	flFadeOuttime;
};

template <>
inline AnimationLayer_t LoopingLerp( float flPercent, AnimationLayer_t from, AnimationLayer_t to )
{
	AnimationLayer_t output;

	output.nSequence = to.nSequence;
	output.flCycle = LoopingLerp( flPercent, from.flCycle, to.flCycle );
	output.flPlaybackrate = to.flPlaybackrate;
	output.flWeight = Lerp( flPercent, from.flWeight, to.flWeight );
	output.nOrder = to.nOrder;

	output.flAnimtime = to.flAnimtime;
	output.flFadeOuttime = to.flFadeOuttime;
	return output;
}

template<> 
inline AnimationLayer_t Lerp<AnimationLayer_t>( float flPercent, const AnimationLayer_t& from, const AnimationLayer_t& to )
{
	AnimationLayer_t output;

	output.nSequence = to.nSequence;
	output.flCycle = Lerp( flPercent, from.flCycle, to.flCycle );
	output.flPlaybackrate = to.flPlaybackrate;
	output.flWeight = Lerp( flPercent, from.flWeight, to.flWeight );
	output.nOrder = to.nOrder;

	output.flAnimtime = to.flAnimtime;
	output.flFadeOuttime = to.flFadeOuttime;
	return output;
}

template <>
inline AnimationLayer_t LoopingLerp_Hermite( float flPercent, AnimationLayer_t prev, AnimationLayer_t from, AnimationLayer_t to )
{
	AnimationLayer_t output;

	output.nSequence = to.nSequence;
	output.flCycle = LoopingLerp_Hermite( flPercent, prev.flCycle, from.flCycle, to.flCycle );
	output.flPlaybackrate = to.flPlaybackrate;
	output.flWeight = Lerp( flPercent, from.flWeight, to.flWeight );
	output.nOrder = to.nOrder;

	output.flAnimtime = to.flAnimtime;
	output.flFadeOuttime = to.flFadeOuttime;
	return output;
}

// YWB:  Specialization for interpolating euler angles via quaternions...
template<> 
inline AnimationLayer_t Lerp_Hermite<AnimationLayer_t>( float flPercent, const AnimationLayer_t& prev, const AnimationLayer_t& from, const AnimationLayer_t& to )
{
	AnimationLayer_t output;

	output.nSequence = to.nSequence;
	output.flCycle = Lerp_Hermite( flPercent, prev.flCycle, from.flCycle, to.flCycle );
	output.flPlaybackrate = to.flPlaybackrate;
	output.flWeight = Lerp( flPercent, from.flWeight, to.flWeight );
	output.nOrder = to.nOrder;

	output.flAnimtime = to.flAnimtime;
	output.flFadeOuttime = to.flFadeOuttime;
	return output;
}

#endif // ANIMATIONLAYER_H
