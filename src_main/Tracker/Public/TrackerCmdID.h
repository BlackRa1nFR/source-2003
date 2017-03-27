//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TRACKERCMDID_H
#define TRACKERCMDID_H
#ifdef _WIN32
#pragma once
#endif

// list of all the cmd's used in i/o queues in the server
enum CmdID_t
{
	CMD_NONE = 0,

	// ITrackerUserDatabase cmds
	CMD_LOGIN,
	CMD_LOGOFF,
	CMD_VALIDATE,
	CMD_CREATE,
	CMD_FINDUSERS,
	CMD_GETSESSIONINFO,
	CMD_ISAUTHED,
	CMD_GETINFO,
	CMD_GETWATCHERS,
	CMD_GETFRIENDLIST,
	CMD_GETFRIENDSTATUS,
	CMD_GETMESSAGE,
	CMD_DELETEMESSAGE,
	CMD_GETFRIENDSGAMESTATUS,
	CMD_USERSAREONLINE,
	CMD_GETCOMPLETEUSERDATA,
	CMD_INSERTCOMPLETEUSERDATA,

	// ITrackerDistroDatabase cmds
	CMD_RESERVEUSERID,
	CMD_UPDATEUSERDISTRIBUTION,
};


#endif // TRACKERCMDID_H
