//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#if !defined( FX_H )
#define FX_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"
#include "particles_simple.h"

class Vector;
class CGameTrace;
typedef CGameTrace trace_t;
//struct trace_t;

void FX_RicochetSound( const Vector& pos );

void FX_AntlionImpact( const Vector &pos, trace_t *tr );
void FX_DebrisFlecks( Vector& origin, trace_t *trace, char materialType, int iScale );
void FX_Tracer( Vector& start, Vector& end, int velocity, bool makeWhiz = true );
void FX_GunshipTracer( Vector& start, Vector& end, int velocity, bool makeWhiz = true );
void FX_StriderTracer( Vector& start, Vector& end, int velocity, bool makeWhiz = true );
void FX_PlayerTracer( Vector& start, Vector& end );
void FX_BulletPass( Vector& start, Vector& end );
void FX_MetalSpark( const Vector &position, const Vector &normal, int iScale = 1 );
void FX_MetalScrape( Vector &position, Vector &normal );
void FX_Sparks( const Vector &pos, int nMagnitude, int nTrailLength, const Vector &vecDir, float flWidth, float flMinSpeed, float flMaxSpeed, char *pSparkMaterial = NULL );
void FX_ElectricSpark( const Vector &pos, int nMagnitude, int nTrailLength, const Vector *vecDir );
void FX_BugBlood( Vector &pos, Vector &dir );
void FX_Blood( Vector &pos, Vector &dir, float r, float g, float b, float a );
void FX_CreateImpactDust( Vector &origin, Vector &normal );
void FX_EnergySplash( const Vector &pos, const Vector &normal, bool bExplosive );
void FX_MicroExplosion( Vector &position, Vector &normal );
void FX_Explosion( Vector& origin, Vector& normal, char materialType );
void FX_ConcussiveExplosion( Vector& origin, Vector& normal ); 
void FX_DustImpact( const Vector &origin, trace_t *tr, int iScale );
void FX_MuzzleEffect( const Vector &origin, const QAngle &angles, float scale, int entityIndex, unsigned char *pFlashColor = NULL );
void FX_GunshipMuzzleEffect( const Vector &origin, const QAngle &angles, float scale, int entityIndex, unsigned char *pFlashColor = NULL );
CSmartPtr<CSimpleEmitter> FX_Smoke( const Vector &origin, const Vector &velocity, float scale, int numParticles, float flDietime, unsigned char *pColor, int iAlpha, const char *pMaterial, float flRoll, float flRollDelta );
void FX_Smoke( const Vector &origin, const QAngle &angles, float scale, int numParticles, unsigned char *pColor = NULL, int iAlpha = -1 );
void FX_Dust( const Vector &vecOrigin, const Vector &vecDirection, float flSize, float flSpeed );
void FX_CreateGaussExplosion( const Vector &pos, const Vector &dir, int type );
void FX_GaussTracer( Vector& start, Vector& end, int velocity, bool makeWhiz = true );

//Water effects
void FX_GunshotSplash( const Vector &origin, const Vector &normal, float scale );
void FX_WaterRipple( const Vector &origin, float scale );

// Useful function for testing whether to draw noZ effects
bool EffectOccluded( const Vector &pos );

#endif // FX_H