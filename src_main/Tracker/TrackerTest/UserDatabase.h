//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef USERDATABASE_H
#define USERDATABASE_H
#ifdef _WIN32
#pragma once
#endif

#include "User.h"

#define MAX_USERS	2048

//-----------------------------------------------------------------------------
// Purpose: Holds the set of users
//-----------------------------------------------------------------------------
class CUserDatabase
{
public:
	CUserDatabase(int maxUsers);
	~CUserDatabase();

	// returns a pointer to the user at the specified index
	CUser *GetUser(int index);

	// returns the next user after index.  Updates index to be that of the user returned.  Set index to -1 to get first user.
	CUser *GetNextUser(int &index);

	// returns the next logged in user after index, updates index to be that of the user returned.
	CUser *GetNextActiveUser(int &index);

private:
	int m_iUsers;
	CUser *m_pUsers;
};

extern CUserDatabase gUserDatabase;


#endif // USERDATABASE_H
