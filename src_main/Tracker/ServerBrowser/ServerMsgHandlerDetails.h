//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SERVERMSGHANDLERDETAILS_H
#define SERVERMSGHANDLERDETAILS_H
#ifdef _WIN32
#pragma once
#endif

#include "Socket.h"

class CServerList;

//-----------------------------------------------------------------------------
// Purpose: Socket handler for pinging internet servers
//-----------------------------------------------------------------------------
class CServerMsgHandlerDetails : public CMsgHandler
{
public:
	CServerMsgHandlerDetails(CServerList *baseobject, HANDLERTYPE type, void *typeinfo = NULL);

	virtual bool Process(netadr_t *from, CMsgBuffer *msg);

private:
	CServerList *m_pServerList;
};




#endif // SERVERMSGHANDLERDETAILS_H
