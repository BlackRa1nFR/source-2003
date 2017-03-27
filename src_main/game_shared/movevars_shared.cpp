//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "movevars_shared.h"

// some cvars used by player movement system
ConVar	sv_gravity		( "sv_gravity","800", FCVAR_SERVER | FCVAR_REPLICATED, "World gravity." );
ConVar	sv_stopspeed	( "sv_stopspeed","100", FCVAR_SERVER | FCVAR_REPLICATED, "Minimum stopping speed when on ground." );
ConVar	sv_noclipaccelerate( "sv_noclipaccelerate", "5", FCVAR_SERVER | FCVAR_ARCHIVE | FCVAR_REPLICATED);
ConVar	sv_noclipspeed( "sv_noclipspeed", "2000", FCVAR_ARCHIVE | FCVAR_SERVER | FCVAR_REPLICATED);
ConVar	sv_maxspeed( "sv_maxspeed", "320", FCVAR_SERVER | FCVAR_REPLICATED);
ConVar	sv_accelerate ( "sv_accelerate", "10", FCVAR_SERVER | FCVAR_REPLICATED);
ConVar	sv_airaccelerate(  "sv_airaccelerate", "10", FCVAR_SERVER | FCVAR_REPLICATED);    
ConVar	sv_wateraccelerate(  "sv_wateraccelerate", "10", FCVAR_SERVER | FCVAR_REPLICATED);     
ConVar	sv_waterfriction(  "sv_waterfriction", "1", FCVAR_SERVER | FCVAR_REPLICATED);      
ConVar	sv_edgefriction ( "sv_edgefriction", "2", FCVAR_SERVER | FCVAR_REPLICATED);
ConVar	sv_wateramp ( "sv_wateramp","0", FCVAR_SERVER | FCVAR_REPLICATED);
ConVar	sv_footsteps( "sv_footsteps", "1", FCVAR_SERVER | FCVAR_REPLICATED, "Play footstep sound for players" );
ConVar	sv_rollspeed( "sv_rollspeed", "200", FCVAR_SERVER | FCVAR_REPLICATED);
ConVar	sv_rollangle( "sv_rollangle", "2", FCVAR_SERVER | FCVAR_REPLICATED);
ConVar	sv_friction		( "sv_friction","4", FCVAR_SERVER | FCVAR_REPLICATED, "World friction." );
ConVar	sv_bounce		( "sv_bounce","1", FCVAR_SERVER | FCVAR_REPLICATED, "Bounce multiplier for when physically simulated objects collide with other objects." );
ConVar	sv_maxvelocity	( "sv_maxvelocity","3500", FCVAR_REPLICATED, "Maximum speed any ballistically moving object is allowed to attain per axis." );
ConVar	sv_stepsize( "sv_stepsize","18", FCVAR_SERVER | FCVAR_REPLICATED );
ConVar	sv_skyname( "sv_skyname", "sky_urb01", FCVAR_ARCHIVE | FCVAR_REPLICATED, "Current name of the skybox texture" );
ConVar	sv_backspeed( "sv_backspeed", "0.6", FCVAR_ARCHIVE | FCVAR_REPLICATED, "How much to slow down backwards motion" );
ConVar  sv_waterdist( "sv_waterdist","12", FCVAR_REPLICATED, "Vertical view fixup when eyes are near water plane." );