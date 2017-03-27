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
#if !defined( C_TE_LEGACYTEMPENTS_H )
#define C_TE_LEGACYTEMPENTS_H
#ifdef _WIN32
#pragma once
#endif

class C_BaseEntity;
class C_LocalTempEntity;
struct model_t;

//-----------------------------------------------------------------------------
// Purpose: Interface for lecacy temp entities
//-----------------------------------------------------------------------------
class ITempEnts
{
public:
	virtual void				Init( void ) = 0;
	virtual void				Shutdown( void ) = 0;
	virtual void				LevelShutdown() = 0;

	virtual void				Update( void ) = 0;
	virtual void				Clear( void ) = 0;

	virtual void				BloodSprite( const Vector &org, int r, int g, int b, int a, int modelIndex, int modelIndex2, float size ) = 0;
	virtual void				RicochetSprite( const Vector &pos, model_t *pmodel, float duration, float scale ) = 0;
	virtual void				MuzzleFlash( const Vector &pos1, const QAngle &angles, int type, int entityIndex, bool firstPerson ) = 0;
	virtual void				EjectBrass( const Vector& pos1, const QAngle& angles, const QAngle& gunAngles, int type ) = 0;
	virtual void				SpawnTempModel( model_t *pModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, float flLifeTime, int iFlags ) = 0;
	virtual void				BreakModel( const Vector &pos, const Vector &size, const Vector &dir, float random, float life, int count, int modelIndex, char flags) = 0;
	virtual void				Bubbles( const Vector &mins, const Vector &maxs, float height, int modelIndex, int count, float speed ) = 0;
	virtual void				BubbleTrail( const Vector &start, const Vector &end, float height, int modelIndex, int count, float speed ) = 0;
	virtual void				Sprite_Explode( C_LocalTempEntity *pTemp, float scale, int flags ) = 0;
	virtual void				FizzEffect( C_BaseEntity *pent, int modelIndex, int density ) = 0;
	virtual C_LocalTempEntity	*DefaultSprite( const Vector &pos, int spriteIndex, float framerate ) = 0;
	virtual void				Sprite_Smoke( C_LocalTempEntity *pTemp, float scale ) = 0;
	virtual C_LocalTempEntity	*TempSprite( const Vector &pos, const Vector &dir, float scale, int modelIndex, int rendermode, int renderfx, float a, float life, int flags, const Vector &normal = vec3_origin ) = 0;
	virtual void				AttachTentToPlayer( int client, int modelIndex, float zoffset, float life ) = 0;
	virtual void				KillAttachedTents( int client ) = 0;
	virtual void				Sprite_Spray( const Vector &pos, const Vector &dir, int modelIndex, int count, int speed, int iRand ) = 0;
	virtual void				RocketFlare( const Vector& pos ) = 0;
	
	virtual void				PlaySound ( C_LocalTempEntity *pTemp, float damp ) = 0;
};

extern ITempEnts *tempents;


#endif // C_TE_LEGACYTEMPENTS_H
