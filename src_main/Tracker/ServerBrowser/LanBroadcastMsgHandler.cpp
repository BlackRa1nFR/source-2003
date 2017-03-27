//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "LanBroadcastMsgHandler.h"

#include "IGameList.h"
//#include "Info.h"
#include "info_key.h"
#include "ServerBrowserDialog.h"

#include <vgui/IVGui.h>

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CLanBroadcastMsgHandler::CLanBroadcastMsgHandler( IGameList *baseobject, HANDLERTYPE type, void *typeinfo /*= NULL*/ ) 
	: CMsgHandler( type, typeinfo )
{
	m_pGameList = baseobject;
}

//-----------------------------------------------------------------------------
// Purpose: Process cracked message
//-----------------------------------------------------------------------------
bool CLanBroadcastMsgHandler::Process( netadr_t *from, CMsgBuffer *msg )
{
	// Skip the control character
	msg->ReadByte();

	// get response name
	const char *str = msg->ReadString();
	if ( !str || !str[0] )
		return false;

	// get infostring
	str = msg->ReadString();
	if ( !str || !str[0] )
		return false;

	char info[ 2048 ];
	strncpy( info, str, 2047 );
	info[2047] = 0;

	serveritem_t server;
	memset(&server, 0, sizeof(server));
	for (int i = 0; i < 4; i++)
	{
		server.ip[i] = from->ip[i];
	}
	server.port = (from->port & 0xff) << 8 | (from->port & 0xff00) >> 8;
	server.doNotRefresh = false;
	server.hadSuccessfulResponse = true;

	v_strncpy(server.name, Info_ValueForKey(info, "hostname"), sizeof(server.name));
	v_strncpy(server.map, Info_ValueForKey(info, "map"), sizeof(server.map));
	v_strncpy(server.gameDir, Info_ValueForKey(info, "gamedir"), sizeof(server.gameDir));
	strlwr(server.gameDir);
	v_strncpy(server.gameDescription, Info_ValueForKey(info, "description"), sizeof(server.gameDescription));
	server.players = atoi(Info_ValueForKey(info, "players"));
	server.maxPlayers = atoi(Info_ValueForKey(info, "max"));
	server.proxy = *Info_ValueForKey(info, "type") == 'p';
	server.ping = (int)((CSocket::GetClock() - m_flRequestTime) * 1000.0f);

	m_pGameList->AddNewServer(server);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLanBroadcastMsgHandler::SetRequestTime(float flRequestTime)
{
	m_flRequestTime = flRequestTime;
}
