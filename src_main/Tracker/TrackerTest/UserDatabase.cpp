//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>

#include "UserDatabase.h"

CUserDatabase gUserDatabase(MAX_USERS);

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : maxUsers - maximum number of users that will be simulated
//-----------------------------------------------------------------------------
CUserDatabase::CUserDatabase(int maxUsers)
{
	m_iUsers = maxUsers;
	m_pUsers = new CUser[m_iUsers];
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CUserDatabase::~CUserDatabase()
{
	delete [] m_pUsers;
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to the user at the specified index
//-----------------------------------------------------------------------------
CUser *CUserDatabase::GetUser(int index)
{
	return &m_pUsers[index];
}

//-----------------------------------------------------------------------------
// Purpose: returns the next user after index.  
//			Updates index to be that of the user returned.  
//			Set index to -1 to get first user.
//-----------------------------------------------------------------------------
CUser *CUserDatabase::GetNextUser(int &index)
{
	index++;
	while (index < m_iUsers)
	{
		return &m_pUsers[index];
	}
	index = -1;
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: returns the next logged in user after index, updates index to be that of the user returned.
//-----------------------------------------------------------------------------
CUser *CUserDatabase::GetNextActiveUser(int &index)
{
	while (++index < m_iUsers)
	{
		if (m_pUsers[index].IsActive())
		{
			return &m_pUsers[index];
		}
	}

	index = -1;
	return NULL;
}
