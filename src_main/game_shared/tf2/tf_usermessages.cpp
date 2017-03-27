//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "usermessages.h"
#include "shake.h"
#include "voice_gamemgr.h"

void RegisterUserMessages( void )
{
	usermessages->Register( "Geiger", 1 );
	usermessages->Register( "Train", 1 );
	usermessages->Register( "HudText", -1 );
	usermessages->Register( "SayText", -1 );
	usermessages->Register( "TextMsg", -1 );
	usermessages->Register( "ResetHUD", 1 );		// called every respawn
	usermessages->Register( "InitHUD", 0 );		// called every time a new player joins the server
	usermessages->Register( "GameTitle", 1 );
	usermessages->Register( "DeathMsg", -1 );
	usermessages->Register( "GameMode", 1 );
	usermessages->Register( "MOTD", -1 );
	usermessages->Register( "ItemPickup", -1 );
	usermessages->Register( "ShowMenu", -1 );
	usermessages->Register( "Shake", -1 );
	usermessages->Register( "Fade", sizeof(ScreenFade_t) );
	usermessages->Register( "TeamChange", 1 );
	usermessages->Register( "ClearDecals", 1 );

	usermessages->Register( "VoiceMask", VOICE_MAX_PLAYERS_DW*4 * 2 + 1 );
	usermessages->Register( "RequestState", 0 );

	usermessages->Register( "TerrainMod", -1 );

	// TF User messages
	usermessages->Register( "Damage", 13 );
	usermessages->Register( "Accuracy", 2 );
	usermessages->Register( "ZoneState", 1 );
	usermessages->Register( "Technology", -1 );
	usermessages->Register( "ActBegin", -1 );
	usermessages->Register( "ActEnd", -1 );
	usermessages->Register( "MinimapPulse", -1 );
	usermessages->Register( "PickupRes", 1 );
}