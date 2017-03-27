//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_DEBUG_H
#define AI_DEBUG_H

#if defined( _WIN32 )
#pragma once
#endif

// This gives an in game report with game_speeds
//#define MEASURE_AI 1

// This uses VPROF to profile
#define VPROF_AI 1

// This dumps a summary result on exit
//#define PROFILE_AI 1

#if defined(MEASURE_AI)
#include "measure_section.h"
#define AI_PROFILE_MEASURE_SCOPE( tag )	MEASURECODE( #tag )
#define AI_PROFILE_SCOPE( tag )			((void)0)
#elif defined(VPROF_AI)
#include "tier0/vprof.h"
#define AI_PROFILE_SCOPE( tag )			VPROF( #tag )
#define AI_PROFILE_MEASURE_SCOPE( tag )	VPROF( #tag )
#elif defined(PROFILE_AI)
#include "tier0/fasttimer.h"
#define AI_PROFILE_SCOPE( tag )			PROFILE_SCOPE( tag )
#define AI_PROFILE_MEASURE_SCOPE( tag )	PROFILE_SCOPE( tag )
#else
#define AI_PROFILE_MEASURE_SCOPE( tag )	((void)0)
#define AI_PROFILE_SCOPE( tag )			((void)0)
#endif


enum AIMsgFlags
{
	AIMF_IGNORE_SELECTED = 0x01
};

void Msg( CAI_BaseNPC *pAI, unsigned flags, const char *pszFormat, ... );
void Msg( CAI_BaseNPC *pAI, const char *pszFormat, ... );

//-----------------------------------------------------------------------------

#ifdef VPROF_AI
inline void AI_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 const IHandleEntity *ignore, int collisionGroup, trace_t *ptr )
{
	VPROF( "AI_TraceLine" );
	UTIL_TraceLine( vecAbsStart, vecAbsEnd, mask, ignore, collisionGroup, ptr );
}

inline void AI_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const IHandleEntity *ignore, 
					 int collisionGroup, trace_t *ptr )
{
	VPROF( "AI_TraceHull" );
	UTIL_TraceHull( vecAbsStart, vecAbsEnd, hullMin, hullMax, mask, ignore, collisionGroup, ptr );
}

inline void AI_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, unsigned int mask, trace_t *ptr )
{
	VPROF( "AI_TraceEntity" );
	UTIL_TraceEntity( pEntity, vecAbsStart, vecAbsEnd, mask, ptr );
}

#else
#define AI_TraceLine UTIL_TraceLine
#define AI_TraceHull UTIL_TraceHull
#define AI_TraceHull UTIL_TraceEntity
#endif



//-----------------------------------------------------------------------------

#ifdef DEBUG
extern bool g_fTestSteering;
#define TestingSteering() g_fTestSteering
#else
#define TestingSteering() false
#endif


#endif // AI_DEBUG_H
