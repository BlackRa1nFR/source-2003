//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ONLINESTATUS_H
#define ONLINESTATUS_H
#pragma once

//-----------------------------------------------------------------------------
// Purpose: Contains the different online status modes
//			Static class
//-----------------------------------------------------------------------------
class COnlineStatus
{
public:
	enum Status_e
	{
		// user is only in for a chat
		CHATUSER = -7,

		// user is being removed from the list
		REMOVED = -6,

		// don't know who the user is
		UNKNOWNUSER = -5,

		// user is awaiting the buddy authorizing them
		AWAITINGAUTHORIZATION = -4,

		// buddy is waiting for an authorization response
		REQUESTINGAUTHORIZATION = -3,

		// negative status is offline but with a special known status
		RECENTLYONLINE = -2,

		// offline, but trying to connect
		CONNECTING = -1, 

		// 0 is basic offline, status unknown
		OFFLINE = 0,

		// positive status means the user is ONLINE and has an available IP
		ONLINE = 1,
		BUSY = 2,
		AWAY = 3,
		INGAME = 4,
		SNOOZE = 5,
	};

	enum
	{
		// keep the heartbeat rate as low as possible, since it is the major server load
		HEARTBEAT_TIME = (5 * 60 * 1000),	// 5 minute heartbeat rate (millis)
		
		RECONNECT_TIME = (30 * 1000),		// 30 second reconnect time (if unable to connect to server)

		SERVERCONNECT_TIMEOUT = (8 * 1000), // 8 second server connection timeout
	};

	static const char *IDToString(int statusID);
	static int StringToID(const char *statusString);
};


#endif // ONLINESTATUS_H
