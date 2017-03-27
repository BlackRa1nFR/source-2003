//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Projectile shot from the MP5 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================


#ifndef	NPC_SNARK_H
#define	NPC_SNARK_H


#include "hl1_ai_basenpc.h"


class CSnark : public CHL1BaseNPC
{
	DECLARE_CLASS( CSnark, CHL1BaseNPC );
public:

	DECLARE_DATADESC();

	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void );
	void	Event_Killed( const CTakeDamageInfo &info );
	bool	Event_Gibbed( const CTakeDamageInfo &info );
	void	HuntThink( void );
	void	SuperBounceTouch( CBaseEntity *pOther );
/*
	int  BloodColor( void ) { return BLOOD_COLOR_YELLOW; }

	// CBaseEntity *m_pTarget;
*/
private:
	Class_T	m_iMyClass;
	float	m_flDie;
	Vector	m_vecTarget;
	float	m_flNextHunt;
	float	m_flNextHit;
	Vector	m_posPrev;
	float	m_flNextBounceSoundTime;
	EHANDLE m_hOwner;
};


#endif	// NPC_SNARK_H
