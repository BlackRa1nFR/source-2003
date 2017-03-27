//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LANBROADCASTMSGHANDLER_H
#define LANBROADCASTMSGHANDLER_H
#ifdef _WIN32
#pragma once
#endif

#include "Socket.h"

//-----------------------------------------------------------------------------
// Purpose: Socket handler for lan broadcast pings
//-----------------------------------------------------------------------------
class CLanBroadcastMsgHandler : public CMsgHandler
{
public:
	CLanBroadcastMsgHandler( IGameList *baseobject, HANDLERTYPE type, void *typeinfo = NULL );
	virtual bool Process( netadr_t *from, CMsgBuffer *msg );
	void SetRequestTime(float flRequestTime);

private:
	IGameList *m_pGameList;
	float m_flRequestTime;
};

#endif // LANBROADCASTMSGHANDLER_H
