//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Holds all the protocol bits and defines used in tracker networking
//
// $NoKeywords: $
//=============================================================================

#ifndef TRACKERPROTOCOL_H
#define TRACKERPROTOCOL_H
#ifdef _WIN32
#pragma once
#endif

// failed return versions of the messages are the TMSG_FAIL_OFFSET + 10000
#define TMSG_FAIL_OFFSET	10000

//-----------------------------------------------------------------------------
// Purpose: List of all the tracker messages used
//			msgID's are 32bits big
//-----------------------------------------------------------------------------
enum TrackerMsgID_t
{
	// generic messages
	TMSG_NONE = 0,		// no message id
	TMSG_ACK = 1,		// packet acknowledgement

	// server -> Client messages
	TSVC_BASE = 1000,

	TSVC_CHALLENGE,
	TSVC_LOGINOK,
	TSVC_LOGINFAIL,
	TSVC_DISCONNECT,
	TSVC_FRIENDS,
	TSVC_FRIENDUPDATE,
	TSVC_HEARTBEAT,
	TSVC_PINGACK,		// acknowledgement of TCLS_PING packet
	TSVC_FRIENDINFO,
	TSVC_USERCREATED,
	TSVC_USERCREATEDENIED,
	TSVC_USERVALID,
	TSVC_FRIENDSFOUND,
	TSVC_NOFRIENDS,
	TSVC_MESSAGE,		// message passed through from another Client
	TSVC_GAMEINFO,		// information about a friends' game

	// Client -> server messages
	TCLS_BASE = 2000,
	
	TCLS_CREATEUSER,	// user creation
	TCLS_LOGIN,			// login message
	TCLS_RESPONSE,		// response to login challenge
	TCLS_PING,
	TCLS_FRIENDSEARCH,
	TCLS_HEARTBEAT,
	TCLS_VALIDATEUSER, 
	TCLS_AUTHUSER,
	TCLS_REQAUTH,
	TCLS_FRIENDINFO,	// friend info request
	TCLS_SETINFO,
	TCLS_MESSAGE,		// obselete: text message that is to be passed on to a friend
	TCLS_ROUTETOFRIEND, // generic reroute of a message to a friend

	// Client -> Client messages
	TCL_BASE = 3000,
	TCL_MESSAGE,		// chat text message
	TCL_USERBLOCK,		// soon to be obselete
	TCL_FRIENDUPDATE,	// OBSELETE
	TCL_REQGAMEINFO,	// OBSELETE
	TCL_GAMEINFO,		// OBSELETE
	TCL_ADDEDTOCHAT,
	TCL_CHATADDUSER,
	TCL_CHATUSERLEAVE,
	TCL_TYPINGMESSAGE,

	// server -> server messages
	TSV_BASE = 4000,

	TSV_WHOISPRIMARY,
	TSV_PRIMARYSRV,
	TSV_REQUESTINFO,
	TSV_SRVINFO,
	TSV_REQSRVINFO,
	TSV_SERVERPING,
	TSV_MONITORINFO,
	TSV_LOCKUSERRANGE,
	TSV_UNLOCKUSERRANGE,
	TSV_REDIRECTTOUSER,
	TSV_FORCEDISCONNECTUSER,
	TSV_USERCHECKMESSAGES,


	// game server -> Client
	TCLG_BASE = 5000,

	// common msg failed ID's
	TSVC_HEARTBEAT_FAIL = TSVC_HEARTBEAT + TMSG_FAIL_OFFSET,

	TCLS_HEARTBEAT_FAIL = TCLS_HEARTBEAT + TMSG_FAIL_OFFSET,

	TCL_MESSAGE_FAIL = TCL_MESSAGE + TMSG_FAIL_OFFSET,

};


#endif // TRACKERPROTOCOL_H
