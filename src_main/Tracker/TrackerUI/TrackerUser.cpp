//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "ITrackerUser.h"

#include "TrackerDoc.h"
#include "OnlineStatus.h"
#include "ServerSession.h"
#include "Buddy.h"

//-----------------------------------------------------------------------------
// Purpose: Implementation of ITrackerUser
//			Provides access to information about tracker users
//-----------------------------------------------------------------------------
class CTrackerUser : public ITrackerUser
{
public:
	virtual bool IsValid()
	{
		return (GetDoc() != NULL);
	}

	// returns the tracker userID of the current user
	virtual int GetTrackerID()
	{
		if (GetDoc())
		{
			return GetDoc()->GetUserID();
		}
		return 0;
	}

	// returns information about a user
	virtual const char *GetUserName(int userID)
	{
		if (GetDoc())
		{
			CBuddy *buddy = GetDoc()->GetBuddy(userID);
			if (buddy)
			{
				return buddy->DisplayName();
			}
		}
		return "";
	}

	virtual const char *GetFirstName(int userID)
	{
		if (GetDoc())
		{
			CBuddy *buddy = GetDoc()->GetBuddy(userID);
			if (buddy)
			{
				return buddy->Data()->GetString("FirstName");
			}
		}
		return "";
	}

	virtual const char *GetLastName(int userID)
	{
		if (GetDoc())
		{
			CBuddy *buddy = GetDoc()->GetBuddy(userID);
			if (buddy)
			{
				return buddy->Data()->GetString("LastName");
			}
		}
		return "";
	}

	virtual const char *GetEmail(int userID)
	{
		if (GetDoc())
		{
			CBuddy *buddy = GetDoc()->GetBuddy(userID);
			if (buddy)
			{
				return buddy->Data()->GetString("Email");
			}
		}
		return "";
	}

	// returns true if friendID is a friend of the current user 
	// ie. the current is authorized to see when the friend is online
	virtual bool IsFriend(int friendID)
	{
		if (GetDoc())
		{
			CBuddy *buddy = GetDoc()->GetBuddy(friendID);
			if (buddy)
			{
				if (buddy->Data()->GetInt("Status") >= COnlineStatus::AWAITINGAUTHORIZATION)
				{
					return true;
				}
			}
		}

		return false;
	}

	// requests authorization from a user
	virtual void RequestAuthorizationFromUser(int potentialFriendID)
	{
		ServerSession().RequestAuthorizationFromUser(potentialFriendID, "");
	}

	// returns the status of the user, > 0 is online, 4 is ingame
	virtual int GetUserStatus(int friendID)
	{
		if (GetDoc())
		{
			CBuddy *buddy = GetDoc()->GetBuddy(friendID);
			if (buddy)
			{
				return buddy->Data()->GetInt("Status", 0);
			}
		}

		return 0;
	}

	// gets the IP address of the server the user is on, returns false if couldn't get
	virtual bool GetUserGameAddress(int friendID, int *ip, int *port)
	{
		if (GetDoc())
		{
			CBuddy *buddy = GetDoc()->GetBuddy(friendID);
			if (buddy)
			{
				*ip = buddy->Data()->GetInt("GameIP");
				*port = buddy->Data()->GetInt("GamePort");
				if (*ip && *port)
				{
					return true;
				}
			}
		}

		return false;
	}

	// returns the number of friends, so we can iterate
	virtual int GetNumberOfFriends()
	{
		return GetDoc()->GetNumberOfBuddies();				
	}

	// returns the userID of a friend - friendIndex is valid in the range [0, GetNumberOfFriends)
	virtual int GetFriendTrackerID(int friendIndex)
	{
		return GetDoc()->GetBuddyByIndex(friendIndex);
	}

	// sets whether or not the user can receive messages at this time
	// messages will be queued until this is set to true
	virtual void SetCanReceiveMessages(bool state)
	{


		
	}
};

// export interface
static CTrackerUser g_TrackerUser;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CTrackerUser, ITrackerUser, TRACKERUSER_INTERFACE_VERSION, g_TrackerUser);
