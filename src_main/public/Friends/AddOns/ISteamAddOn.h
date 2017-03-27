//=========== (C) Copyright 1999-2003 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface to a Steam Add On
//
//=============================================================================

#ifndef ISTEAMADDON_H
#define ISTEAMADDON_H
#pragma once

#include "interface.h"
#include "AddOnTypes.h"
class CUtlMsgBuffer;

class ISteamAddOn : public IBaseInterface
{
public:
	// allows SteamAddOn to link to the core vgui factories
    virtual bool Initialize(CreateInterfaceFn *vguiFactories, int factoryCount) = 0;

	// allows SteamAddOns to link to all other modules
	virtual bool PostInitialize(CreateInterfaceFn *modules, int factoryCount) = 0;

	// when Friends closes down - all SteamAddOns are notified
    virtual void Deactivate() = 0;

	// notifies the SteamAddOn of the user's ID and username. 
	// Note: username can be set mulitple times due to changing of name
    virtual void SetUserID(unsigned int userID) = 0;
    virtual void SetUserName(const char *userName) = 0;

    // A request to start/join this AddOn with this user ID/name. addOnSessionID will be zero if it's a new session request
    virtual void InviteUser(SessionInt64 addOnSessionID, unsigned int targetUserID, const char *username) = 0;

	// Query if there are any 'open' sessions - open meaning allowing new users to join the sessions
    virtual int QueryOpenSessionCount() = 0;

	// will be valid right after a call to QueryOpenInviteCount will set the addOnSessionID and hostname for 
	// any open sessions for this addOn. Return true if it's a valid index
    virtual bool QueryOpenSessionInfo(int nOpenGameIndex, SessionInt64 &addOnSessionID, char *pszHostName) = 0;

	// returns true if this userID is involved in an addOnSession with this ID
    virtual bool IsUserInvolved(SessionInt64 addOnSessionID, unsigned int userID) = 0;

    // if session doesn't exist, then the SteamAddOn body should deal with it
    virtual bool OnReceiveMsg(SessionInt64 addOnSessionID, CUtlMsgBuffer *msgBuffer) = 0;

    // Let's the SteamAddOn know when when any friend's status has changed
    virtual void OnFriendStatusChanged() = 0;
};

#define STEAMADDON_INTERFACE_VERSION "SteamAddOn003"

#endif // ISTEAMADDON_H

