//=========== (C) Copyright 1999-2003 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
//=============================================================================

#ifndef FRIENDSNET_INTERFACE_H
#define FRIENDSNET_INTERFACE_H
#pragma once

class CUtlMsgBuffer;
class CServerSession;

#include "interface.h"
#include "Friends/AddOns/AddOnTypes.h"

class IFriendsNET : public IBaseInterface
{
public:
	// check if we have network information for this user
    virtual bool CheckUserRegistered(unsigned int userID) = 0;

	// update a user's network information
    virtual void UpdateUserNetInfo(unsigned int userID, unsigned int netSessionID, int serverID, int IP, int port) = 0;

	// set whether or not we send directly to user or through the server
    virtual void SetUserSendViaServer(unsigned int userID, bool bSendViaServer) = 0;

	// Gets a blob of data that represents this user's information
    virtual bool GetUserNetInfoBlob(unsigned int userID, unsigned int dataBlob[8]) = 0;
	// Sets a user's information using the same blob of data type
    virtual bool SetUserNetInfoBlob(unsigned int userID, const unsigned int dataBlob[8]) = 0;

    // send binary data to user, marked with game/sessionID
    virtual void SendAddOnPacket(const char *pszGameID, SessionInt64 addOnSessionID, unsigned int userID, const CUtlMsgBuffer& buffer) = 0;
};

#define FRIENDSNET_INTERFACE_VERSION "FriendsNET003"

#endif // FRIENDSNET_INTERFACE_H

