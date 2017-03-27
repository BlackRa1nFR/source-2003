//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_NAV_PROPERTY_H
#define AI_NAV_PROPERTY_H
#ifdef _WIN32
#pragma once
#endif

enum navproperties_t
{
	NAV_IGNORE = 0,
};

void NavPropertyAdd( CBaseEntity *pEntity, navproperties_t property );
void NavPropertyRemove( CBaseEntity *pEntity, navproperties_t property );
bool NavPropertyIsOnEntity( CBaseEntity *pEntity, navproperties_t property );


#endif // AI_NAV_PROPERTY_H
