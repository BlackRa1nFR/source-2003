//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Projectile shot by bullsquid 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	GRENADESPIT_H
#define	GRENADESPIT_H

#include "basegrenade_shared.h"

enum SpitSize_e
{
	SPIT_SMALL,
	SPIT_MEDIUM,
	SPIT_LARGE,
};

#define SPIT_GRAVITY 0.5

class CGrenadeSpit : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenadeSpit, CBaseGrenade );

	void		Spawn( void );
	void		Precache( void );
	void		SpitThink( void );
	void 		GrenadeSpitTouch( CBaseEntity *pOther );
	void		Event_Killed( const CTakeDamageInfo &info );
	void		SetSpitSize(int nSize);

	int			m_nSquidSpitSprite;
	float		m_fSpitDeathTime;		// If non-zero won't detonate

	void EXPORT				Detonate(void);
	CGrenadeSpit(void);

	DECLARE_DATADESC();
};

#endif	//GRENADESPIT_H
