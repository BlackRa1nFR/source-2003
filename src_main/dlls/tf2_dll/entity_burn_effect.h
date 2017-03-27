//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ENTITY_BURN_EFFECT_H
#define ENTITY_BURN_EFFECT_H
#ifdef _WIN32
#pragma once
#endif


#include "basenetworkable.h"
#include "server_class.h"


class CEntityBurnEffect : public CBaseNetworkable
{
public:
	
	DECLARE_CLASS( CEntityBurnEffect, CBaseNetworkable );
	DECLARE_SERVERCLASS();

	static CEntityBurnEffect* Create( CBaseEntity *pBurningEntity );


// Overrides.
public:

	virtual bool			ShouldTransmit( 
		const edict_t *recipient, 
		const void *pvs, 
		int clientArea
		);


private:
	CNetworkHandle( CBaseEntity, m_hBurningEntity );
};


#endif // ENTITY_BURN_EFFECT_H
