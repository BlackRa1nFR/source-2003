//
// cdll_exp.c
//
// 4-23-98  JOHN
//  this file implements the functions exported the the client-side (HUD-drawing) DLL
//

#include "quakedef.h"
#include "cdll_int.h"

int Draw_MessageFontInfo( short *pWidth );

int hudServerCmd( char *pszCmdString, bool bReliable )
{
	char buf[255];
	// just like the client typed "cmd xxxxx" at the console
	
	strcpy( buf, "cmd " );
	strncat( buf, pszCmdString, 250 );

	Cmd_TokenizeString( buf );
	Cmd_ForwardToServer( bReliable );

	return TRUE;
}

int hudClientCmd(char *pszCmdString)
{
	if (!pszCmdString)
		return 0;

	Cbuf_AddText(pszCmdString);
	Cbuf_AddText("\n");
	return 1;
}

// player info handler
void hudGetPlayerInfo( int ent_num, hud_player_info_t *pinfo )
{
	ent_num -= 1; // player list if offset by 1 from ents

	if ( ent_num >= cl.maxclients || 
		 ent_num < 0 || 
		 !cl.players[ ent_num ].name[ 0 ] )
	{
		pinfo->name = NULL;
		pinfo->model = NULL;
		pinfo->thisplayer = false;
		pinfo->logo = 0;
		return;
	}

	pinfo->thisplayer = ( ent_num == cl.playernum ) ? true : false;

	pinfo->name			= cl.players[ ent_num ].name;
	pinfo->model		= cl.players[ ent_num ].model;
	pinfo->logo			= (int)cl.players[ ent_num ].logo;
}