/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef EXPLODE_H
#define EXPLODE_H

#define	SF_ENVEXPLOSION_NODAMAGE	0x00000001 // when set, ENV_EXPLOSION will not actually inflict damage
#define	SF_ENVEXPLOSION_REPEATABLE	0x00000002 // can this entity be refired?
#define SF_ENVEXPLOSION_NOFIREBALL	0x00000004 // don't draw the fireball
#define SF_ENVEXPLOSION_NOSMOKE		0x00000008 // don't draw the smoke
#define SF_ENVEXPLOSION_NODECAL		0x00000010 // don't make a scorch mark
#define SF_ENVEXPLOSION_NOSPARKS	0x00000020 // don't make sparks
#define SF_ENVEXPLOSION_NOSOUND		0x00000040 // don't play explosion sound.
#define SF_ENVEXPLOSION_RND_ORIENT	0x00000080	// randomly oriented sprites
// 0x100
#define SF_ENVEXPLOSION_NOPARTICLES 0x00000200
#define SF_ENVEXPLOSION_NODLIGHTS	0x00000400
#define SF_ENVEXPLOSION_NOCLAMPMIN	0x00000800 // don't clamp the minimum size of the fireball sprite
#define SF_ENVEXPLOSION_NOCLAMPMAX	0x00001000 // don't clamp the maximum size of the fireball sprite

extern short	g_sModelIndexFireball;
extern short	g_sModelIndexSmoke;


extern void ExplosionCreate( const Vector &center, const QAngle &angles, CBaseEntity *pOwner, int magnitude, int radius, bool doDamage );

#endif			//EXPLODE_H
