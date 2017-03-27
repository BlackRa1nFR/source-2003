//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: Contains entities for implementing/changing game rules dynamically within each BSP.
//
//=============================================================================

#include "cbase.h"
#include "datamap.h"
#include "gamerules.h"
#include "maprules.h"
#include "player.h"
#include "entitylist.h"
#include "ai_hull.h"
#include "entityoutput.h"

class CRuleEntity : public CBaseEntity
{
public:
	DECLARE_CLASS( CRuleEntity, CBaseEntity );

	void	Spawn( void );

	DECLARE_DATADESC();

	void	SetMaster( string_t iszMaster ) { m_iszMaster = iszMaster; }

protected:
	bool	CanFireForActivator( CBaseEntity *pActivator );

private:
	string_t	m_iszMaster;
};

BEGIN_DATADESC( CRuleEntity )

	DEFINE_KEYFIELD( CRuleEntity, m_iszMaster, FIELD_STRING, "master" ),

END_DATADESC()



void CRuleEntity::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	m_fEffects		= EF_NODRAW;
}


bool CRuleEntity::CanFireForActivator( CBaseEntity *pActivator )
{
	if ( m_iszMaster != NULL_STRING )
	{
		if ( UTIL_IsMasterTriggered( m_iszMaster, pActivator ) )
			return true;
		else
			return false;
	}
	
	return true;
}

// 
// CRulePointEntity -- base class for all rule "point" entities (not brushes)
//
class CRulePointEntity : public CRuleEntity
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CRulePointEntity, CRuleEntity );

	int		m_Score;
	void		Spawn( void );
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CRulePointEntity )

	DEFINE_FIELD( CRulePointEntity, m_Score,	FIELD_INTEGER ),

END_DATADESC()


void CRulePointEntity::Spawn( void )
{
	BaseClass::Spawn();
	SetModelName( NULL_STRING );
	m_Score = 0;
}

// 
// CRuleBrushEntity -- base class for all rule "brush" entities (not brushes)
// Default behavior is to set up like a trigger, invisible, but keep the model for volume testing
//
class CRuleBrushEntity : public CRuleEntity
{
public:
	DECLARE_CLASS( CRuleBrushEntity, CRuleEntity );

	void		Spawn( void );

private:
};

void CRuleBrushEntity::Spawn( void )
{
	SetModel( STRING( GetModelName() ) );
	BaseClass::Spawn();
}


// CGameScore / game_score	-- award points to player / team 
//	Points +/- total
//	Flag: Allow negative scores					SF_SCORE_NEGATIVE
//	Flag: Award points to team in teamplay		SF_SCORE_TEAM

#define SF_SCORE_NEGATIVE			0x0001
#define SF_SCORE_TEAM				0x0002

class CGameScore : public CRulePointEntity
{
public:
	DECLARE_CLASS( CGameScore, CRulePointEntity );

	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	bool	KeyValue( const char *szKeyName, const char *szValue );

	inline	int		Points( void ) { return m_Score; }
	inline	bool	AllowNegativeScore( void ) { return m_spawnflags & SF_SCORE_NEGATIVE; }
	inline	int		AwardToTeam( void ) { return (m_spawnflags & SF_SCORE_TEAM); }

	inline	void	SetPoints( int points ) { m_Score = points; }

private:
};

LINK_ENTITY_TO_CLASS( game_score, CGameScore );


void CGameScore::Spawn( void )
{
	BaseClass::Spawn();
}


bool CGameScore::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "points"))
	{
		SetPoints( atoi(szValue) );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}



void CGameScore::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	// Only players can use this
	if ( pActivator->IsPlayer() )
	{
		if ( AwardToTeam() )
		{
			pActivator->AddPointsToTeam( Points(), AllowNegativeScore() );
		}
		else
		{
			pActivator->AddPoints( Points(), AllowNegativeScore() );
		}
	}
}


// CGameEnd / game_end	-- Ends the game in MP

class CGameEnd : public CRulePointEntity
{
	DECLARE_CLASS( CGameEnd, CRulePointEntity );

public:
	DECLARE_DATADESC();

	void	InputGameEnd( inputdata_t &inputdata );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
private:
};

BEGIN_DATADESC( CGameEnd )

	// inputs
	DEFINE_INPUTFUNC( CGameEnd, FIELD_VOID, "EndGame", InputGameEnd ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( game_end, CGameEnd );


void CGameEnd::InputGameEnd( inputdata_t &inputdata )
{
	g_pGameRules->EndMultiplayerGame();
}

void CGameEnd::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	g_pGameRules->EndMultiplayerGame();
}


//
// CGameText / game_text	-- NON-Localized HUD Message (use env_message to display a titles.txt message)
//	Flag: All players					SF_ENVTEXT_ALLPLAYERS
//
#define SF_ENVTEXT_ALLPLAYERS			0x0001


class CGameText : public CRulePointEntity
{
public:
	DECLARE_CLASS( CGameText, CRulePointEntity );

	bool	KeyValue( const char *szKeyName, const char *szValue );

	DECLARE_DATADESC();

	inline	bool	MessageToAll( void ) { return (m_spawnflags & SF_ENVTEXT_ALLPLAYERS); }
	inline	void	MessageSet( const char *pMessage ) { m_iszMessage = AllocPooledString(pMessage); }
	inline	const char *MessageGet( void )	{ return STRING( m_iszMessage ); }

	void InputDisplay( inputdata_t &inputdata );
	void Display( CBaseEntity *pActivator );

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		Display( pActivator );
	}

private:

	string_t m_iszMessage;
	hudtextparms_t	m_textParms;
};

LINK_ENTITY_TO_CLASS( game_text, CGameText );

// Save parms as a block.  Will break save/restore if the structure changes, but this entity didn't ship with Half-Life, so
// it can't impact saved Half-Life games.
BEGIN_DATADESC( CGameText )

	DEFINE_KEYFIELD( CGameText, m_iszMessage, FIELD_STRING, "message" ),

	DEFINE_KEYFIELD( CGameText, m_textParms.channel, FIELD_INTEGER, "channel" ),
	DEFINE_KEYFIELD( CGameText, m_textParms.x, FIELD_FLOAT, "x" ),
	DEFINE_KEYFIELD( CGameText, m_textParms.y, FIELD_FLOAT, "y" ),
	DEFINE_KEYFIELD( CGameText, m_textParms.effect, FIELD_INTEGER, "effect" ),
	DEFINE_KEYFIELD( CGameText, m_textParms.fadeinTime, FIELD_FLOAT, "fadein" ),
	DEFINE_KEYFIELD( CGameText, m_textParms.fadeoutTime, FIELD_FLOAT, "fadeout" ),
	DEFINE_KEYFIELD( CGameText, m_textParms.holdTime, FIELD_FLOAT, "holdtime" ),
	DEFINE_KEYFIELD( CGameText, m_textParms.fxTime, FIELD_FLOAT, "fxtime" ),

	DEFINE_ARRAY( CGameText, m_textParms, FIELD_CHARACTER, sizeof(hudtextparms_t) ),

	// Inputs
	DEFINE_INPUTFUNC( CGameText, FIELD_VOID, "Display", InputDisplay ),

END_DATADESC()



bool CGameText::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "color"))
	{
		int color[4];
		UTIL_StringToIntArray( color, 4, szValue );
		m_textParms.r1 = color[0];
		m_textParms.g1 = color[1];
		m_textParms.b1 = color[2];
		m_textParms.a1 = color[3];
	}
	else if (FStrEq(szKeyName, "color2"))
	{
		int color[4];
		UTIL_StringToIntArray( color, 4, szValue );
		m_textParms.r2 = color[0];
		m_textParms.g2 = color[1];
		m_textParms.b2 = color[2];
		m_textParms.a2 = color[3];
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}


void CGameText::InputDisplay( inputdata_t &inputdata )
{
	Display( inputdata.pActivator );
}

void CGameText::Display( CBaseEntity *pActivator )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( MessageToAll() )
	{
		UTIL_HudMessageAll( m_textParms, MessageGet() );
	}
	else
	{
		// If we're in singleplayer, show the message to the player.
		if ( gpGlobals->maxClients == 1 )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
			UTIL_HudMessage( pPlayer, m_textParms, MessageGet() );
		}
		// Otherwise show the message to the player that triggered us.
		else if ( pActivator && pActivator->IsNetClient() )
		{
			UTIL_HudMessage( pActivator, m_textParms, MessageGet() );
		}
	}
}


/* DISABLE:  Will be replaced by activator filter entities
//
// CGameTeamMaster / game_team_master	-- "Masters" like multisource, but based on the team of the activator
// Only allows mastered entity to fire if the team matches my team
//
// team index (pulled from server team list "mp_teamlist"
// Flag: Remove on Fire
// Flag: Any team until set?		-- Any team can use this until the team is set (otherwise no teams can use it)
//

#define SF_TEAMMASTER_FIREONCE			0x0001
#define SF_TEAMMASTER_ANYTEAM			0x0002

class CGameTeamMaster : public CRulePointEntity
{
public:
	DECLARE_CLASS( CGameTeamMaster, CRulePointEntity );

	bool		KeyValue( const char *szKeyName, const char *szValue );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int			ObjectCaps( void ) { return BaseClass:: ObjectCaps() | FCAP_MASTER; }

	bool		IsTriggered( CBaseEntity *pActivator );
	const char	*TeamID( void );
	inline bool RemoveOnFire( void ) { return (m_spawnflags & SF_TEAMMASTER_FIREONCE) ? true : false; }
	inline bool AnyTeam( void ) { return (m_spawnflags & SF_TEAMMASTER_ANYTEAM) ? true : false; }

private:
	bool		TeamMatch( CBaseEntity *pActivator );

	int			m_teamIndex;
	USE_TYPE	triggerType;

	COutputEvent m_OnUse;
};

LINK_ENTITY_TO_CLASS( game_team_master, CGameTeamMaster );

bool CGameTeamMaster::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "teamindex"))
	{
		m_teamIndex = atoi( szValue );
	}
	else if (FStrEq(szKeyName, "triggerstate"))
	{
		int type = atoi( szValue );
		switch( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}


void CGameTeamMaster::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( useType == USE_SET )
	{
		if ( value < 0 )
		{
			m_teamIndex = -1;
		}
		else
		{
			m_teamIndex = g_pGameRules->GetTeamIndex( pActivator->TeamID() );
		}
		return;
	}

	if ( TeamMatch( pActivator ) )
	{
		SUB_UseTargets( pActivator, triggerType, value );
		m_OnUse.FireOutput(pActivator, this); // dvsents2: what is the correct name for this output?

		if ( RemoveOnFire() )
		{
			UTIL_Remove( this );
		}
	}
}


bool CGameTeamMaster::IsTriggered( CBaseEntity *pActivator )
{
	return TeamMatch( pActivator );
}


const char *CGameTeamMaster::TeamID( void )
{
	if ( m_teamIndex < 0 )		// Currently set to "no team"
		return "";

	return g_pGameRules->GetIndexedTeamName( m_teamIndex );		// UNDONE: Fill this in with the team from the "teamlist"
}


bool CGameTeamMaster::TeamMatch( CBaseEntity *pActivator )
{
	if ( m_teamIndex < 0 && AnyTeam() )
		return true;

	if ( !pActivator )
		return false;

	return UTIL_TeamsMatch( pActivator->TeamID(), TeamID() );
}


//
// CGameTeamSet / game_team_set	-- Changes the team of the entity it targets to the activator's team
// Flag: Fire once
// Flag: Clear team				-- Sets the team to "NONE" instead of activator

#define SF_TEAMSET_FIREONCE			0x0001
#define SF_TEAMSET_CLEARTEAM		0x0002

class CGameTeamSet : public CRulePointEntity
{
public:
	DECLARE_CLASS( CGameTeamSet, CRulePointEntity );

	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline bool RemoveOnFire( void ) { return (m_spawnflags & SF_TEAMSET_FIREONCE) ? true : false; }
	inline bool ShouldClearTeam( void ) { return (m_spawnflags & SF_TEAMSET_CLEARTEAM) ? true : false; }

private:
	COutputEvent m_OnUse;
};

LINK_ENTITY_TO_CLASS( game_team_set, CGameTeamSet );


void CGameTeamSet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( ShouldClearTeam() )
	{
		SUB_UseTargets( pActivator, USE_SET, -1 );
	}
	else
	{
		SUB_UseTargets( pActivator, USE_SET, 0 );
	}

	m_OnUse.FireOutput(pActivator, this); // dvsents2: handle data here, -1 vs. 0

	if ( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}
*/

//
// CGamePlayerZone / game_player_zone -- players in the zone fire my target when I'm fired
//
// Needs master?
class CGamePlayerZone : public CRuleBrushEntity
{
public:
	DECLARE_CLASS( CGamePlayerZone, CRuleBrushEntity );

	bool		KeyValue( const char *szKeyName, const char *szValue );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	DECLARE_DATADESC();

private:
	string_t	m_iszInTarget;
	string_t	m_iszOutTarget;
	string_t	m_iszInCount;
	string_t	m_iszOutCount;

	COutputEvent m_OnPlayerInZone;
	COutputEvent m_OnPlayerOutZone;

	COutputInt m_PlayersInCount;
	COutputInt m_PlayersOutCount;
};

LINK_ENTITY_TO_CLASS( game_zone_player, CGamePlayerZone );
BEGIN_DATADESC( CGamePlayerZone )

	DEFINE_FIELD( CGamePlayerZone, m_iszInTarget, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszOutTarget, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszInCount, FIELD_STRING ),
	DEFINE_FIELD( CGamePlayerZone, m_iszOutCount, FIELD_STRING ),

	// Outputs
	DEFINE_OUTPUT(CGamePlayerZone, m_OnPlayerInZone, "OnPlayerInZone"),
	DEFINE_OUTPUT(CGamePlayerZone, m_OnPlayerOutZone, "OnPlayerOutZone"),
	DEFINE_OUTPUT(CGamePlayerZone, m_PlayersInCount, "PlayersInCount"),
	DEFINE_OUTPUT(CGamePlayerZone, m_PlayersOutCount, "PlayersOutCount"),

END_DATADESC()


bool CGamePlayerZone::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "intarget"))
	{
		m_iszInTarget = AllocPooledString( szValue );
	}
	else if (FStrEq(szKeyName, "outtarget"))
	{
		m_iszOutTarget = AllocPooledString( szValue );
	}
	else if (FStrEq(szKeyName, "incount"))
	{
		m_iszInCount = AllocPooledString( szValue );
	}
	else if (FStrEq(szKeyName, "outcount"))
	{
		m_iszOutCount = AllocPooledString( szValue );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

void CGamePlayerZone::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int playersInCount = 0;
	int playersOutCount = 0;

	if ( !CanFireForActivator( pActivator ) )
		return;

	CBaseEntity *pPlayer = NULL;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			trace_t		trace;
			Hull_t		hullType;

			hullType = HULL_HUMAN;
			if ( pPlayer->GetFlags() & FL_DUCKING )
			{
				hullType = HULL_SMALL_CENTERED;
			}

			UTIL_TraceModel( pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), NAI_Hull::Mins(hullType), 
				NAI_Hull::Maxs(hullType), this, COLLISION_GROUP_NONE, &trace );

			if ( trace.startsolid )
			{
				playersInCount++;
				m_OnPlayerInZone.FireOutput(pPlayer, this);

				if ( m_iszInTarget != NULL_STRING )
				{
					FireTargets( STRING(m_iszInTarget), pPlayer, pActivator, useType, value ); // dvsents2: done, this fires once per person in the GameZone. Purpose?
				}
			}
			else
			{
				playersOutCount++;
				m_OnPlayerOutZone.FireOutput(pPlayer, this);

				if ( m_iszOutTarget != NULL_STRING )
				{
					FireTargets( STRING(m_iszOutTarget), pPlayer, pActivator, useType, value );
				}
			}
		}
	}

	m_PlayersInCount.Set(playersInCount, pActivator, this);
	m_PlayersOutCount.Set(playersOutCount, pActivator, this);

	if ( m_iszInCount != NULL_STRING )
	{
		FireTargets( STRING(m_iszInCount), pActivator, this, USE_SET, playersInCount );
	}

	if ( m_iszOutCount != NULL_STRING )
	{
		FireTargets( STRING(m_iszOutCount), pActivator, this, USE_SET, playersOutCount );
	}
}

/*
// Disable.  Eventually will be replace by new activator filter entities.  (LHL)
//
// CGamePlayerHurt / game_player_hurt	-- Damages the player who fires it
// Flag: Fire once

#define SF_PKILL_FIREONCE			0x0001
class CGamePlayerHurt : public CRulePointEntity
{
public:
	DECLARE_CLASS( CGamePlayerHurt, CRulePointEntity );

	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	inline bool RemoveOnFire( void ) { return (m_spawnflags & SF_PKILL_FIREONCE) ? true : false; }

	DECLARE_DATADESC();

private:
	
	float m_flDamage;		// Damage to inflict, negative values give health.

	COutputEvent m_OnUse;
};

LINK_ENTITY_TO_CLASS( game_player_hurt, CGamePlayerHurt );


BEGIN_DATADESC( CGamePlayerHurt )

	DEFINE_KEYFIELD( CGamePlayerHurt, m_flDamage, FIELD_FLOAT, "dmg" ),

END_DATADESC()



void CGamePlayerHurt::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( pActivator->IsPlayer() )
	{
		if ( m_flDamage < 0 )
		{
			pActivator->TakeHealth( -m_flDamage, DMG_GENERIC );
		}
		else
		{
			pActivator->TakeDamage( this, this, m_flDamage, DMG_GENERIC );
		}
	}
	
	SUB_UseTargets( pActivator, useType, value );
	m_OnUse.FireOutput(pActivator, this); // dvsents2: handle useType and value here - they are passed through

	if ( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}
*/

//
// CGamePlayerEquip / game_playerequip	-- Sets the default player equipment
// Flag: USE Only

#define SF_PLAYEREQUIP_USEONLY			0x0001
#define MAX_EQUIP		32

class CGamePlayerEquip : public CRulePointEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CGamePlayerEquip, CRulePointEntity );

	bool		KeyValue( const char *szKeyName, const char *szValue );
	void		Touch( CBaseEntity *pOther );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	inline bool	UseOnly( void ) { return (m_spawnflags & SF_PLAYEREQUIP_USEONLY) ? true : false; }

private:

	void		EquipPlayer( CBaseEntity *pPlayer );

	string_t	m_weaponNames[MAX_EQUIP];
	int			m_weaponCount[MAX_EQUIP];
};

LINK_ENTITY_TO_CLASS( game_player_equip, CGamePlayerEquip );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CGamePlayerEquip )

	DEFINE_AUTO_ARRAY( CGamePlayerEquip, m_weaponNames,		FIELD_STRING ),
	DEFINE_AUTO_ARRAY( CGamePlayerEquip, m_weaponCount,		FIELD_INTEGER ),

END_DATADESC()




bool CGamePlayerEquip::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( !BaseClass::KeyValue( szKeyName, szValue ) )
	{
		for ( int i = 0; i < MAX_EQUIP; i++ )
		{
			if ( !m_weaponNames[i] )
			{
				char tmp[128];

				UTIL_StripToken( szKeyName, tmp );

				m_weaponNames[i] = AllocPooledString(tmp);
				m_weaponCount[i] = atoi(szValue);
				m_weaponCount[i] = max(1,m_weaponCount[i]);
				return true;
			}
		}
	}

	return false;
}


void CGamePlayerEquip::Touch( CBaseEntity *pOther )
{
	if ( !CanFireForActivator( pOther ) )
		return;

	if ( UseOnly() )
		return;

	EquipPlayer( pOther );
}

void CGamePlayerEquip::EquipPlayer( CBaseEntity *pEntity )
{
	CBasePlayer *pPlayer = NULL;

	if ( pEntity->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)pEntity;
	}

	if ( !pPlayer )
		return;

	for ( int i = 0; i < MAX_EQUIP; i++ )
	{
		if ( !m_weaponNames[i] )
			break;
		for ( int j = 0; j < m_weaponCount[i]; j++ )
		{
 			pPlayer->GiveNamedItem( STRING(m_weaponNames[i]) );
		}
	}
}


void CGamePlayerEquip::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	EquipPlayer( pActivator );
}


//
// CGamePlayerTeam / game_player_team	-- Changes the team of the player who fired it
// Flag: Fire once
// Flag: Kill Player
// Flag: Gib Player

#define SF_PTEAM_FIREONCE			0x0001
#define SF_PTEAM_KILL    			0x0002
#define SF_PTEAM_GIB     			0x0004

class CGamePlayerTeam : public CRulePointEntity
{
public:
	DECLARE_CLASS( CGamePlayerTeam, CRulePointEntity );

	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:

	inline bool RemoveOnFire( void ) { return (m_spawnflags & SF_PTEAM_FIREONCE) ? true : false; }
	inline bool ShouldKillPlayer( void ) { return (m_spawnflags & SF_PTEAM_KILL) ? true : false; }
	inline bool ShouldGibPlayer( void ) { return (m_spawnflags & SF_PTEAM_GIB) ? true : false; }
	
	const char *TargetTeamName( const char *pszTargetName, CBaseEntity *pActivator );
};

LINK_ENTITY_TO_CLASS( game_player_team, CGamePlayerTeam );


const char *CGamePlayerTeam::TargetTeamName( const char *pszTargetName, CBaseEntity *pActivator )
{
	CBaseEntity *pTeamEntity = NULL;

	while ((pTeamEntity = gEntList.FindEntityByName( pTeamEntity, pszTargetName, pActivator )) != NULL)
	{
		if ( FClassnameIs( pTeamEntity, "game_team_master" ) )
			return pTeamEntity->TeamID();
	}

	return NULL;
}


void CGamePlayerTeam::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	if ( pActivator->IsPlayer() )
	{
		const char *pszTargetTeam = TargetTeamName( STRING(m_target), pActivator );
		if ( pszTargetTeam )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
			g_pGameRules->ChangePlayerTeam( pPlayer, pszTargetTeam, ShouldKillPlayer(), ShouldGibPlayer() );
		}
	}
	
	if ( RemoveOnFire() )
	{
		UTIL_Remove( this );
	}
}


