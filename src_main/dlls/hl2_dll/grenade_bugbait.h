//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef GRENADE_BUGBAIT_H
#define GRENADE_BUGBAIT_H
#ifdef _WIN32
#pragma once
#endif

#include "smoke_trail.h"
#include "basegrenade_shared.h"

//Radius of the bugbait's effect on other creatures
extern ConVar bugbait_radius;
extern ConVar bugbait_hear_radius;
extern ConVar bugbait_distract_time;

//
// Bug Bait Grenade
//

class CGrenadeBugBait : public CBaseGrenade
{
	DECLARE_CLASS( CGrenadeBugBait, CBaseGrenade );
public:
	void	Spawn( void );
	void	Precache( void );

	void	BugBaitTouch( CBaseEntity *pOther );

	DECLARE_DATADESC();

protected:
	void	CreateTarget( const Vector &position, CBaseEntity *pOther );
	
	SporeTrail *m_pSporeTrail;
};

extern CBaseGrenade *BugBaitGrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const QAngle &angVelocity, CBaseEntity *owner );

#endif // GRENADE_BUGBAIT_H
