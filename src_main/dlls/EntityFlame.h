//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef ENTITYFLAME_H
#define ENTITYFLAME_H
#ifdef _WIN32
#pragma once
#endif

class CEntityFlame : public CBaseEntity 
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_CLASS( CEntityFlame, CBaseEntity );

	CEntityFlame( void );

	static CEntityFlame	*Create( CBaseEntity *pTarget );

	void	AttachToEntity( CBaseEntity *pTarget );
	void	SetLifetime( float lifetime );

	DECLARE_DATADESC();

protected:

	void InputIgnite( inputdata_t &inputdata );

	void	FlameThink( void );

	CNetworkHandle( CBaseEntity, m_hEntAttached );		// The entity that we are burning (attached to).

	CNetworkVar( float, m_flSize );
	float	m_flLifetime;
};

#endif // ENTITYFLAME_H
