//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef RECIPIENTFILTER_H
#define RECIPIENTFILTER_H
#ifdef _WIN32
#pragma once
#endif

#include "irecipientfilter.h"
#include "const.h"
#include "player.h"

//-----------------------------------------------------------------------------
// Purpose: A generic filter for determining whom to send message/sounds etc. to and
//  providing a bit of additional state information
//-----------------------------------------------------------------------------
class CRecipientFilter : public IRecipientFilter
{
public:
	// Some messages can be sent to a client side entity
	enum
	{
		NORMAL = 0,
		ENTITY,
	};

					CRecipientFilter();
	virtual 		~CRecipientFilter();

	virtual bool	IsReliable( void ) const;

	virtual int		GetRecipientCount( void ) const;
	virtual int		GetRecipientIndex( int slot ) const;

	virtual bool	IsEntityMessage( void ) const;
	virtual int		GetEntityIndex( void ) const;
	virtual const char *GetEntityMessageName( void ) const;

	virtual bool	IsBroadcastMessage( void );

	virtual bool	IsInitMessage( void ) const;

public:

	void			Reset( void );

	void			MakeInitMessage( void );

	void			MakeReliable( void );
	void			SetEntityMessage( int entity, char const *msgname );
	
	void			AddAllPlayers( void );
	void			AddRecipientsByPVS( const Vector& origin );
	void			AddRecipientsByPAS( const Vector& origin );
	void			AddRecipient( CBasePlayer *player );
	void			RemoveAllRecipients( void );
	void			RemoveRecipient( CBasePlayer *player );
	void			AddRecipientsByTeam( CTeam *team );
	void			RemoveRecipientsByTeam( CTeam *team );

	void			UsePredictionRules( void );
	bool			IsUsingPredictionRules( void ) const;

	bool			IgnorePredictionCull( void ) const;
	void			SetIgnorePredictionCull( bool ignore );

private:
	void			EnsureTruePlayerCount( void );
	void			AddPlayersFromBitMask( unsigned int playerbits );

	bool				m_bReliable;
	bool				m_bInitMessage;
	CUtlVector< int >	m_Recipients;
	int					m_Type;
	int					m_nEntityIndex;
	char				*m_pEntityMessageName;
	// i.e., being sent to all players
	int					m_nTruePlayerCount;
	// If using prediction rules, the filter itself suppresses local player
	bool				m_bUsingPredictionRules;
	// If ignoring prediction cull, then external systems can determine
	//  whether this is a special case where culling should not occur
	bool				m_bIgnorePredictionCull;
};

//-----------------------------------------------------------------------------
// Purpose: Simple class to create a filter for a single player ( unreliable )
//-----------------------------------------------------------------------------
class CSingleUserRecipientFilter : public CRecipientFilter
{
public:
	CSingleUserRecipientFilter( CBasePlayer *player )
	{
		AddRecipient( player );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Simple class to create a filter for all players ( unreliable )
//-----------------------------------------------------------------------------
class CBroadcastRecipientFilter : public CRecipientFilter
{
public:
	CBroadcastRecipientFilter( void )
	{
		AddAllPlayers();
	}
};

//-----------------------------------------------------------------------------
// Purpose: Simple class to create a filter for all players ( reliable )
//-----------------------------------------------------------------------------
class CRelieableBroadcastRecipientFilter : public CBroadcastRecipientFilter
{
public:
	CRelieableBroadcastRecipientFilter( void )
	{
		MakeReliable();
	}
};

//-----------------------------------------------------------------------------
// Purpose: Send a message to a client side entity for all players (unreliable)
//-----------------------------------------------------------------------------
class CEntityMessageFilter : public CRecipientFilter
{
public:
	CEntityMessageFilter( CBaseEntity *entity, char const *msgname )
	{
		Assert( msgname );
		AddAllPlayers();
		SetEntityMessage( entity->entindex(), msgname );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Add players in PAS to recipient list (unreliable)
//-----------------------------------------------------------------------------
class CPASFilter : public CRecipientFilter
{
public:
	CPASFilter( void )
	{
	}

	CPASFilter( const Vector& origin )
	{
		AddRecipientsByPAS( origin );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Add players in PAS to list and if not in single player, use attenuation
//  to remove those that are too far away from source origin
// Source origin can be stated as an entity or just a passed in origin
// (unreliable)
//-----------------------------------------------------------------------------
class CPASAttenuationFilter : public CPASFilter
{
public:
	CPASAttenuationFilter( void )
	{
	}

	CPASAttenuationFilter( CBaseEntity *entity, soundlevel_t soundlevel ) :
		CPASFilter( entity->GetSoundEmissionOrigin() )
	{
		Filter( entity->GetSoundEmissionOrigin(), SNDLVL_TO_ATTN( soundlevel ) );
	}

	CPASAttenuationFilter( CBaseEntity *entity, float attenuation = ATTN_NORM ) :
		CPASFilter( entity->GetSoundEmissionOrigin() )
	{
		Filter( entity->GetSoundEmissionOrigin(), attenuation );
	}

	CPASAttenuationFilter( const Vector& origin, soundlevel_t soundlevel ) :
		CPASFilter( origin )
	{
		Filter( origin, SNDLVL_TO_ATTN( soundlevel ) );
	}

	CPASAttenuationFilter( const Vector& origin, float attenuation = ATTN_NORM ) :
		CPASFilter( origin )
	{
		Filter( origin, attenuation );
	}

	CPASAttenuationFilter( CBaseEntity *entity, const char *lookupSound ) :
		CPASFilter( entity->GetSoundEmissionOrigin() )
	{
		soundlevel_t level = CBaseEntity::LookupSoundLevel( lookupSound );
		float attenuation = SNDLVL_TO_ATTN( level );
		Filter( entity->GetSoundEmissionOrigin(), attenuation );
	}

	CPASAttenuationFilter( const Vector& origin, const char *lookupSound ) :
		CPASFilter( origin )
	{
		soundlevel_t level = CBaseEntity::LookupSoundLevel( lookupSound );
		float attenuation = SNDLVL_TO_ATTN( level );
		Filter( origin, attenuation );
	}

public:
	void Filter( const Vector& origin, float attenuation = ATTN_NORM );
};

//-----------------------------------------------------------------------------
// Purpose: Simple PVS based filter ( unreliable )
//-----------------------------------------------------------------------------
class CPVSFilter : public CRecipientFilter
{
public:
	CPVSFilter( const Vector& origin )
	{
		AddRecipientsByPVS( origin );
	}
};

#endif // RECIPIENTFILTER_H
