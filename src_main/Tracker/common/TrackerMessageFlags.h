//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TRACKERMESSAGEFLAGS_H
#define TRACKERMESSAGEFLAGS_H
#pragma once

//-----------------------------------------------------------------------------
// Purpose: the set of flags that can modify a message
//-----------------------------------------------------------------------------
enum TrackerMessageFlags_e
{
	MESSAGEFLAG_REQUESTAUTH = 1,	// message is requesting authorization from user
	MESSAGEFLAG_AUTHED = 2,			// user has authorized themselves to you
	MESSAGEFLAG_SYSTEM = 4,			// message is from the tracker system
	
};


#endif // TRACKERMESSAGEFLAGS_H
