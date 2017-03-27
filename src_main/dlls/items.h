//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ITEMS_H
#define ITEMS_H
#pragma once

#include "entityoutput.h"


#define MAX_NORMAL_BATTERY	100


class CItem : public CBaseAnimating
{
public:
	DECLARE_CLASS( CItem, CBaseAnimating );

	void	Spawn( void );
	CBaseEntity*	Respawn( void );
	void	ItemTouch( CBaseEntity *pOther );
	void	Materialize( void );
	virtual bool MyTouch( CBasePlayer *pPlayer ) { return false; };

private:

	COutputEvent m_OnPlayerTouch;

	DECLARE_DATADESC();
};

#endif // ITEMS_H
