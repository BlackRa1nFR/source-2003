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
#ifndef CLIENT_H
#define CLIENT_H

extern void ClientPutInServer( edict_t *pEdict, const char *playername );
extern void ClientCommand( CBasePlayer *player );
extern void ClientPrecache( void );
// Game specific precaches
extern void ClientGamePrecache( void );
extern const char *GetGameDescription( void );
void ClientKill( edict_t *pEdict );
void Host_Say( edict_t *pEdict, int teamonly );

class CUserCmd;


#endif		// CLIENT_H
