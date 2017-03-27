//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:		Shield used by the mortar synth 
//
// $NoKeywords: $
//=============================================================================

#ifndef SCANNER_SHIELD_H
#define SCANNER_SHIELD_H
#ifdef _WIN32
#pragma once
#endif

#include "util.h"
#include "baseanimating.h"

#define NUM_CAGE_BEAMS 2

class CBeam;

class CScannerShield : public CBaseAnimating
{
	DECLARE_CLASS( CScannerShield, CBaseAnimating );
public:

	void				Spawn( void );
	void				Precache( void );
	void				SetTarget( CBaseEntity *pTarget );
	bool				IsShieldOn( void );
	void				TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType);

private:
	CBeam*				m_pCageBeam[NUM_CAGE_BEAMS];
	void				FadeIn( void );
	void				FadeOut( void );
	void				Spark( void );

public:
	DECLARE_DATADESC();
};


#endif // SCANNER_SHIELD_H
