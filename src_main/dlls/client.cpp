/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include "cbase.h"
#include "player.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "physics.h"
#include "entitylist.h"
#include "shake.h"
#include "globalstate.h"
#include "event_tempentity_tester.h"
#include "ndebugoverlay.h"
#include "engine/IEngineSound.h"
#include <ctype.h>
#include "vstdlib/strtools.h"
#include "te_effect_dispatch.h"
#include "globals.h"


extern int giPrecacheGrunt;
extern int g_iWeaponCheat;

// For not just using one big ai net
extern CBaseEntity*	FindPickerEntity( CBasePlayer* pPlayer );
/*
============
ClientKill

Player entered the suicide command

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
============
*/
void ClientKill( edict_t *pEdict )
{
	CBasePlayer *pl = (CBasePlayer*) GetContainingEntity( pEdict );
	pl->CommitSuicide();
}


//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
void Host_Say( edict_t *pEdict, int teamonly )
{
	CBasePlayer *client;
	int		j;
	char	*p;
	char	text[128];
	char    szTemp[256];
	const char *cpSay = "say";
	const char *cpSayTeam = "say_team";
	const char *pcmd = engine->Cmd_Argv(0);

	// We can get a raw string now, without the "say " prepended
	if ( engine->Cmd_Argc() == 0 )
		return;

	if ( !stricmp( pcmd, cpSay) || !stricmp( pcmd, cpSayTeam ) )
	{
		if ( engine->Cmd_Argc() >= 2 )
		{
			p = (char *)engine->Cmd_Args();
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else  // Raw text, need to prepend argv[0]
	{
		if ( engine->Cmd_Argc() >= 2 )
		{
			Q_snprintf( szTemp,sizeof(szTemp), "%s %s", ( char * )pcmd, (char *)engine->Cmd_Args() );
		}
		else
		{
			// Just a one word command, use the first word...sigh
			Q_snprintf( szTemp,sizeof(szTemp), "%s", ( char * )pcmd );
		}
		p = szTemp;
	}

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

// make sure the text has content
	char *pc;
	for ( pc = p; pc != NULL && *pc != 0; pc++ )
	{
		if ( isprint( *pc ) && !isspace( *pc ) )
		{
			pc = NULL;	// we've found an alphanumeric character,  so text is valid
			break;
		}
	}
	if ( pc != NULL )
		return;  // no character found, so say nothing

	CBasePlayer *pPlayer = ((CBasePlayer *)CBaseEntity::Instance( pEdict ));

	Assert( pPlayer );

	if ( !pPlayer->CanSpeak() )
		return;

	Assert( STRING( pPlayer->pl.netname ) );

// turn on color set 2  (color on,  no sound)
	if ( teamonly )
		Q_snprintf( text,sizeof(text), "%c(TEAM) %s: ", 2, STRING( pPlayer->pl.netname ) );
	else
		Q_snprintf( text,sizeof(text), "%c%s: ", 2, STRING( pPlayer->pl.netname ) );

	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if ( (int)strlen(p) > j )
		p[j] = 0;

	strcat( text, p );
	strcat( text, "\n" );

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	client = NULL;
	
	// UNDONE: This should use UTIL_PlayerByIndex and not NextEntByClass (does dynamic_cast!)
	while ( ((client = gEntList.NextEntByClass( (CBasePlayer*)client )) != NULL) && (!FNullEnt(client->edict()) ) )
	{
		if ( !client->edict() )
			continue;
		
		if ( client->edict() == pEdict )
			continue;

		if ( !(client->IsNetClient()) )	// Not a client ? (should never be true)
			continue;

		if ( teamonly && g_pGameRules->PlayerCanHearChat( client, pPlayer ) != GR_TEAMMATE )
			continue;

		CSingleUserRecipientFilter user( client );
		user.MakeReliable();
		UserMessageBegin( user, "SayText" );
			WRITE_BYTE( ENTINDEX(client->edict()) );
			WRITE_STRING( text );
		MessageEnd();

	}

	// print to the sending client
	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();
	UserMessageBegin( user, "SayText" );
		WRITE_BYTE( ENTINDEX(pEdict) );
		WRITE_STRING( text );
	MessageEnd();

	// echo to server console
	Msg( text );

	Assert( p );
	if ( teamonly )
		UTIL_LogPrintf( "\"%s\" say_team \"%s\"\n", STRING( pPlayer->pl.netname ), p );
	else
		UTIL_LogPrintf( "\"%s\" say \"%s\"\n", STRING( pPlayer->pl.netname ), p );
}


void ClientPrecache( void )
{
	// Precache cable textures.
	engine->PrecacheModel( "cable/cable.vmt" );	
	engine->PrecacheModel( "cable/cable_lit.vmt" );	
	engine->PrecacheModel( "cable/chain.vmt" );	
	engine->PrecacheModel( "cable/rope.vmt" );	
	
	ClientGamePrecache();
}

//-----------------------------------------------------------------------------
// Purpose: called each time a player uses a "cmd" command
// Input  : pPlayer - the player who issued the command
//			Use engine->Cmd_Argv,  engine->Cmd_Argv, and engine->Cmd_Argc to get 
//			pointers the character string command.
//-----------------------------------------------------------------------------
void SetDebugBits( CBasePlayer* pPlayer, char *name, int bit )
{

	// If no name was given set bits based on the picked
	if (FStrEq(name,"")) 
	{
		CBaseEntity *pEntity = FindPickerEntity( pPlayer );
		if ( pEntity )
		{
			if (pEntity->m_debugOverlays & bit)
				pEntity->m_debugOverlays &= ~bit;
			else
				pEntity->m_debugOverlays |= bit;
		}
	}
	// Otherwise set bits based on name or classname
	else 
	{
		CBaseEntity *ent = NULL;
		while ( (ent = gEntList.NextEnt(ent)) != NULL )
		{
			if (  (ent->GetEntityName() != NULL_STRING	&& FStrEq(name, STRING(ent->GetEntityName())))	|| 
				  (ent->m_iClassname != NULL_STRING	&& FStrEq(name, STRING(ent->m_iClassname))) ||
				  (ent->GetClassname() !=NULL && FStrEq(name, ent->GetClassname())))
			{
				if (ent->m_debugOverlays & bit)
					ent->m_debugOverlays &= ~bit;
				else
					ent->m_debugOverlays |= bit;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pKillTargetName - 
//-----------------------------------------------------------------------------
void KillTargets( const char *pKillTargetName )
{
	CBaseEntity *pentKillTarget = NULL;

	DevMsg( 2, "KillTarget: %s\n", pKillTargetName );
	pentKillTarget = gEntList.FindEntityByName( NULL, pKillTargetName, NULL );
	while ( pentKillTarget )
	{
		UTIL_Remove( pentKillTarget );

		DevMsg( 2, "killing %s\n", STRING( pentKillTarget->m_iClassname ) );
		pentKillTarget = gEntList.FindEntityByName( pentKillTarget, pKillTargetName, NULL );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void ConsoleKillTarget( CBasePlayer *pPlayer, char *name)
{
	// If no name was given use the picker
	if (FStrEq(name,"")) 
	{
		CBaseEntity *pEntity = FindPickerEntity( pPlayer );
		if ( pEntity )
		{
			UTIL_Remove( pEntity );
			Msg( "killing %s\n", pEntity->GetDebugName() );
			return;
		}
	}
	// Otherwise use name or classname
	KillTargets( name );
}

//------------------------------------------------------------------------------
// Purpose : Draw a line betwen two points.  White if no world collisions, red if collisions
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_DrawLine( void )
{
	Vector startPos;
	Vector endPos;

	startPos.x = atof(engine->Cmd_Argv(1));
	startPos.y = atof(engine->Cmd_Argv(2));
	startPos.z = atof(engine->Cmd_Argv(3));
	endPos.x = atof(engine->Cmd_Argv(4));
	endPos.y = atof(engine->Cmd_Argv(5));
	endPos.z = atof(engine->Cmd_Argv(6));

	NDebugOverlay::AddDebugLine(startPos,endPos,true,true);
}
static ConCommand drawline("drawline", CC_DrawLine, "Draws line between two 3D Points.\n\tGreen if no collision\n\tRed is collides with something\n\tArguments: x1 y1 z1 x2 y2 z2", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : Draw a cross at a points.  
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_DrawCross( void )
{
	Vector vPosition;

	vPosition.x = atof(engine->Cmd_Argv(1));
	vPosition.y = atof(engine->Cmd_Argv(2));
	vPosition.z = atof(engine->Cmd_Argv(3));

	// Offset since min and max z in not about center
	Vector mins = Vector(-5,-5,-5);
	Vector maxs = Vector(5,5,5);

	Vector start = mins + vPosition;
	Vector end   = maxs + vPosition;
	NDebugOverlay::AddDebugLine(start,end,true,true);

	start.x += (maxs.x - mins.x);
	end.x	-= (maxs.x - mins.x);
	NDebugOverlay::AddDebugLine(start,end,true,true);

	start.y += (maxs.y - mins.y);
	end.y	-= (maxs.y - mins.y);
	NDebugOverlay::AddDebugLine(start,end,true,true);

	start.x -= (maxs.x - mins.x);
	end.x	+= (maxs.x - mins.x);
	NDebugOverlay::AddDebugLine(start,end,true,true);
}
static ConCommand drawcross("drawcross", CC_DrawCross, "Draws a cross at the given location\n\tArguments: x y z", FCVAR_CHEAT);


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_Kill( void )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (pPlayer)
	{
		if ( engine->Cmd_Argc() > 1)
		{
			// Find the matching netname
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex(i) );
				if ( pPlayer )
				{
					if ( Q_strstr(STRING(pPlayer->pl.netname), engine->Cmd_Argv(1)) )
					{
						ClientKill( pPlayer->pev );
					}
				}
			}
		}
		else
		{
			ClientKill( pPlayer->pev );
		}
	}
}
static ConCommand kill("kill", CC_Player_Kill, "kills the player");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_Buddha( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if (pPlayer)
	{
		if (pPlayer->m_debugOverlays & OVERLAY_BUDDHA_MODE)
		{
			pPlayer->m_debugOverlays &= ~OVERLAY_BUDDHA_MODE;
			Msg("Buddha Mode off...\n");
		}
		else
		{
			pPlayer->m_debugOverlays |= OVERLAY_BUDDHA_MODE;
			Msg("Buddha Mode on...\n");
		}
	}
}
static ConCommand buddha("buddha", CC_Player_Buddha, "Toggle.  Player takes damage but won't die. (Shows red cross when health is zero)", FCVAR_CHEAT);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_Say( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if (pPlayer)
	{
		Host_Say( pPlayer->pev, 0 );
	}
}
static ConCommand say("say", CC_Player_Say, "Display player message");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_SayTeam( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if (pPlayer)
	{
		Host_Say( pPlayer->pev, 1 );
	}
}
static ConCommand say_team("say_team", CC_Player_SayTeam, "Display player message to team");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_Give( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( pPlayer && g_iWeaponCheat != 0 && engine->Cmd_Argc()>=2 )
	{
		char item_to_give[ 256 ];
		Q_strncpy( item_to_give, engine->Cmd_Argv(1), sizeof( item_to_give ) );
		Q_strlower( item_to_give );

		// Dirty hack to avoid suit playing it's pickup sound
		if ( !stricmp( item_to_give, "item_suit" ) )
		{
			pPlayer->EquipSuit();
			return;
		}

		string_t iszItem = AllocPooledString( item_to_give );	// Make a copy of the classname
		pPlayer->GiveNamedItem( STRING(iszItem) );
	}
}
static ConCommand give("give", CC_Player_Give, "Give item to player.\n\tArguments: <item_name>", FCVAR_CHEAT);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_FOV( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( pPlayer && g_iWeaponCheat != 0)
	{
		if ( engine->Cmd_Argc() > 1)
		{
			int FOV = atoi( engine->Cmd_Argv(1) );

			pPlayer->SetFOV( FOV );
		}
		else
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs( "\"fov\" is \"%d\"\n", (int)pPlayer->m_Local.m_iFOV ) );
		}
	}
}
static ConCommand fov("fov", CC_Player_FOV, "Change players FOV");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_SetModel( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( pPlayer && engine->Cmd_Argc() == 2)
	{
		static char szName[256];
		Q_strncpy( szName, "models/" ,sizeof(szName));
		strcat( szName, engine->Cmd_Argv(1) );
		strcat( szName, ".mdl" );

		pPlayer->SetModel( szName );
		UTIL_SetSize(pPlayer, VEC_HULL_MIN, VEC_HULL_MAX);
		UTIL_Relink( pPlayer );
	}
}
static ConCommand setmodel("setmodel", CC_Player_SetModel, "Changes's player's model");

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_LastInv( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( pPlayer)
	{
		pPlayer->SelectLastItem();
	}
}
//static ConCommand lastinv("lastinv", CC_Player_LastInv);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CC_Player_TestDispatchEffect( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( !pPlayer)
		return;
	
	if ( engine->Cmd_Argc() < 2 )
	{
		Msg(" Usage: test_dispatcheffect <effect name> <distance away> <flags> <magnitude> <scale>\n " );
		Msg("		 defaults are: <distance 1024> <flags 0> <magnitude 0> <scale 0>\n" );
		return;
	}

	// Optional distance
	float flDistance = 1024;
	if ( engine->Cmd_Argc() >= 3 )
	{
		flDistance = atoi( engine->Cmd_Argv( 2 ) );
	}

	// Optional flags
	float flags = 0;
	if ( engine->Cmd_Argc() >= 4 )
	{
		flags = atoi( engine->Cmd_Argv( 3 ) );
	}

	// Optional magnitude
	float magnitude = 0;
	if ( engine->Cmd_Argc() >= 5 )
	{
		magnitude = atof( engine->Cmd_Argv( 4 ) );
	}

	// Optional scale
	float scale = 0;
	if ( engine->Cmd_Argc() >= 6 )
	{
		scale = atof( engine->Cmd_Argv( 5 ) );
	}

	Vector vecForward;
	QAngle vecAngles = pPlayer->EyeAngles();
	AngleVectors( vecAngles, &vecForward );

	// Trace forward
	trace_t tr;
	Vector vecSrc = pPlayer->EyePosition();
	Vector vecEnd = vecSrc + (vecForward * flDistance);
	UTIL_TraceLine( vecSrc, vecEnd, MASK_ALL, pPlayer, COLLISION_GROUP_NONE, &tr );

	// Fill out the generic data
	CEffectData data;
	// If we hit something, use that data
	if ( tr.fraction < 1.0 )
	{
		data.m_vOrigin = tr.endpos;
		VectorAngles( tr.plane.normal, data.m_vAngles );
		data.m_vNormal = tr.plane.normal;
	}
	else
	{
		data.m_vOrigin = vecEnd;
		data.m_vAngles = vecAngles;
		AngleVectors( vecAngles, &data.m_vNormal );
	}
	data.m_nEntIndex = pPlayer->entindex();
	data.m_fFlags = flags;
	data.m_flMagnitude = magnitude;
	data.m_flScale = scale;
	DispatchEffect( (char *)engine->Cmd_Argv(1), data );
}

static ConCommand test_dispatcheffect("test_dispatcheffect", CC_Player_TestDispatchEffect, "Test a clientside dispatch effect.\n\tUsage: test_dispatcheffect <effect name> <distance away> <flags> <magnitude> <scale>\n\tDefaults are: <distance 1024> <flags 0> <magnitude 0> <scale 0>\n", FCVAR_CHEAT);

//-----------------------------------------------------------------------------
// Purpose: Quickly switch to the physics cannon, or back to previous item
//-----------------------------------------------------------------------------
void CC_Player_PhysSwap( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	
	if ( pPlayer )
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();

		if ( pWeapon )
		{
			const char *strWeaponName = pWeapon->GetName();

			if ( !Q_stricmp( strWeaponName, "weapon_physcannon" ) )
			{
				pPlayer->SelectLastItem();
			}
			else
			{
				pPlayer->SelectItem( "weapon_physcannon" );
			}
		}
	}
}
static ConCommand physswap("phys_swap", CC_Player_PhysSwap);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_Player_Use( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( pPlayer)
	{
		pPlayer->SelectItem((char *)engine->Cmd_Argv(1));
	}
}
static ConCommand use("use", CC_Player_Use, "Use a particular weapon\t\nArguments: <weapon_name>");


//------------------------------------------------------------------------------
// A small wrapper around SV_Move that never clips against the supplied entity.
//------------------------------------------------------------------------------
static bool TestEntityPosition ( CBasePlayer *pPlayer )
{	
	trace_t	trace;
	UTIL_TraceEntity( pPlayer, pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), MASK_PLAYERSOLID, &trace );
	return (trace.startsolid == 0);
}


//------------------------------------------------------------------------------
// Searches along the direction ray in steps of "step" to see if 
// the entity position is passible.
// Used for putting the player in valid space when toggling off noclip mode.
//------------------------------------------------------------------------------
static int FindPassableSpace( CBasePlayer *pPlayer, const Vector& direction, float step, Vector& oldorigin )
{
	int i;
	for ( i = 0; i < 100; i++ )
	{
		Vector origin = pPlayer->GetAbsOrigin();
		VectorMA( origin, step, direction, origin );
		pPlayer->SetAbsOrigin( origin );
		if ( TestEntityPosition( pPlayer ) )
		{
			VectorCopy( pPlayer->GetAbsOrigin(), oldorigin );
			return 1;
		}
	}
	return 0;
}


//------------------------------------------------------------------------------
// Noclip
//------------------------------------------------------------------------------
void CC_Player_NoClip( void )
{
	ConVar const *cheats = cvar->FindVar( "sv_cheats" );
	if ( !cheats || !cheats->GetInt() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	CPlayerState *pl = pPlayer->PlayerData();
	Assert( pl );

	if (pPlayer->GetMoveType() != MOVETYPE_NOCLIP)
	{
		// Disengage from hierarchy
		pPlayer->SetParent( NULL );
		pPlayer->SetMoveType( MOVETYPE_NOCLIP );
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "noclip ON\n");
		pPlayer->AddEFlags( EFL_NOCLIP_ACTIVE );
		return;
	}

	pPlayer->RemoveEFlags( EFL_NOCLIP_ACTIVE );
	pPlayer->SetMoveType( MOVETYPE_WALK );

	Vector oldorigin = pPlayer->GetAbsOrigin();
	ClientPrint( pPlayer, HUD_PRINTCONSOLE, "noclip OFF\n");
	if ( !TestEntityPosition( pPlayer ) )
	{
		Vector forward, right, up;

		AngleVectors ( pl->v_angle, &forward, &right, &up);
		
		// Try to move into the world
		if ( !FindPassableSpace( pPlayer, forward, 1, oldorigin ) )
		{
			if ( !FindPassableSpace( pPlayer, right, 1, oldorigin ) )
			{
				if ( !FindPassableSpace( pPlayer, right, -1, oldorigin ) )		// left
				{
					if ( !FindPassableSpace( pPlayer, up, 1, oldorigin ) )	// up
					{
						if ( !FindPassableSpace( pPlayer, up, -1, oldorigin ) )	// down
						{
							if ( !FindPassableSpace( pPlayer, forward, -1, oldorigin ) )	// back
							{
								Msg( "Can't find the world\n" );
							}
						}
					}
				}
			}
		}

		pPlayer->SetAbsOrigin( oldorigin );
	}
}

static ConCommand noclip("noclip", CC_Player_NoClip, "Toggle. Player becomes non-solid and flies.", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Sets client to godmode
//------------------------------------------------------------------------------
void CC_God_f (void)
{
	ConVar const *cheats = cvar->FindVar( "sv_cheats" );
	if ( !cheats || !cheats->GetInt() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	if ( gpGlobals->deathmatch )
		return;

	pPlayer->ToggleFlag( FL_GODMODE );
	if (!(pPlayer->GetFlags() & FL_GODMODE ) )
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "godmode OFF\n");
	else
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "godmode ON\n");
}

static ConCommand god("god", CC_God_f, "Toggle. Player becomes invulnerable.", FCVAR_CHEAT );


//------------------------------------------------------------------------------
// Sets client to godmode
//------------------------------------------------------------------------------
void CC_setpos_f (void)
{
	ConVar const *cheats = cvar->FindVar( "sv_cheats" );
	if ( !cheats || !cheats->GetInt() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	if ( engine->Cmd_Argc() < 3 )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:  setpos x y <z optional>\n");
		return;
	}

	Vector oldorigin = pPlayer->GetAbsOrigin();

	Vector newpos;
	newpos.x = atof( engine->Cmd_Argv(1) );
	newpos.y = atof( engine->Cmd_Argv(2) );
	newpos.z = engine->Cmd_Argc() == 4 ? atof( engine->Cmd_Argv(3) ) : oldorigin.z;

	pPlayer->SetAbsOrigin( newpos );

	if ( !TestEntityPosition( pPlayer ) )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "setpos into world, use noclip to unstick yourself!\n");
	}
}

static ConCommand setpos("setpos", CC_setpos_f, "Move player to specified origin (must have sv_cheats).", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Sets client to godmode
//------------------------------------------------------------------------------
void CC_setang_f (void)
{
	ConVar const *cheats = cvar->FindVar( "sv_cheats" );
	if ( !cheats || !cheats->GetInt() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	if ( engine->Cmd_Argc() < 3 )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:  setang pitch yaw <roll optional>\n");
		return;
	}

	QAngle oldang = pPlayer->GetAbsAngles();

	QAngle newang;
	newang.x = atof( engine->Cmd_Argv(1) );
	newang.y = atof( engine->Cmd_Argv(2) );
	newang.z = engine->Cmd_Argc() == 4 ? atof( engine->Cmd_Argv(3) ) : oldang.z;

	pPlayer->SnapEyeAngles( newang );
}

static ConCommand setang("setang", CC_setang_f, "Snap player eyes to specified pitch yaw <roll:optional> (must have sv_cheats).", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Sets client to notarget mode.
//------------------------------------------------------------------------------
void CC_Notarget_f (void)
{
	ConVar const *cheats = cvar->FindVar( "sv_cheats" );
	if ( !cheats || !cheats->GetInt() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	if ( gpGlobals->deathmatch )
		return;

	pPlayer->ToggleFlag( FL_NOTARGET );
	if ( !(pPlayer->GetFlags() & FL_NOTARGET ) )
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "notarget OFF\n");
	else
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "notarget ON\n");
}

static ConCommand notarget("notarget", CC_Notarget_f, "Toggle. Player becomes hidden to NPCs.", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Damage the client the specified amount
//------------------------------------------------------------------------------
void CC_HurtMe_f(void)
{
	ConVar const *cheats = cvar->FindVar( "sv_cheats" );
	if ( !cheats || !cheats->GetInt() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() ); 
	if ( !pPlayer )
		return;

	int iDamage = 10;
	if ( engine->Cmd_Argc() >= 2 )
	{
		iDamage = atoi( engine->Cmd_Argv( 1 ) );
	}

	pPlayer->TakeDamage( CTakeDamageInfo( pPlayer, pPlayer, iDamage, DMG_GENERIC ) );
}

static ConCommand hurtme("hurtme", CC_HurtMe_f, "Hurts the player.\n\tArguments: <health to lose>", FCVAR_CHEAT);


//-----------------------------------------------------------------------------
// Purpose: called each time a player uses a "cmd" command
// Input  : *pEdict - the player who issued the command
//			Use engine->Cmd_Argv,  engine->Cmd_Argv, and engine->Cmd_Argc to get 
//			pointers the character string command.
//-----------------------------------------------------------------------------
void ClientCommand( CBasePlayer *pPlayer )
{
	const char *pcmd = engine->Cmd_Argv(0);

	// Is the client spawned yet?
	if ( !pPlayer )
		return;

	/*
	const char *pstr;

	if (((pstr = strstr(pcmd, "weapon_")) != NULL)  && (pstr == pcmd))
	{
		// Subtype may be specified
		if ( engine->Cmd_Argc() == 2 )
		{
			pPlayer->SelectItem( pcmd, atoi( engine->Cmd_Argv( 1 ) ) );
		}
		else
		{
			pPlayer->SelectItem(pcmd);
		}
	}
	*/
	
	if ( FStrEq( pcmd, "killtarget" ) )
	{
		if ( g_pDeveloper->GetInt() )
		{
			ConsoleKillTarget(pPlayer, engine->Cmd_Argv(1));
		}
	}
	else if ( FStrEq( pcmd, "fade" ) )
	{
		color32 black = {32,63,100,200};
		UTIL_ScreenFade( pPlayer, black, 3, 3, FFADE_OUT  );
	} 
	else if ( FStrEq( pcmd, "te" ) )
	{
		if ( FStrEq( engine->Cmd_Argv(1), "stop" ) )
		{
			// Destroy it
			//
			CBaseEntity *ent = gEntList.FindEntityByClassname( NULL, "te_tester" );
			while ( ent )
			{
				CBaseEntity *next = gEntList.FindEntityByClassname( ent, "te_tester" );
				UTIL_Remove( ent );
				ent = next;
			}
		}
		else
		{
			CTempEntTester::Create( pPlayer->WorldSpaceCenter(), pPlayer->EyeAngles(), engine->Cmd_Argv(1), engine->Cmd_Argv(2) );
		}
	}
	else 
	{
		if (!g_pGameRules->ClientCommand( pcmd, pPlayer ))
		{
			// tell the user they entered an unknown command
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs( "Unknown command: %s\n", pcmd ) );
		}
	}
}
