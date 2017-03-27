//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef NETWORKSTRINGTABLEITEM_H
#define NETWORKSTRINGTABLEITEM_H
#ifdef _WIN32
#pragma once
#endif

#include "utlsymbol.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNetworkStringTableItem
{
public:
	enum
	{
		MAX_USERDATA_BITS = 12,
		MAX_USERDATA_SIZE = (1 << MAX_USERDATA_BITS)
	};

					CNetworkStringTableItem( void );
					~CNetworkStringTableItem( void );

	bool			SetUserData( int length, const void *userdata );
	const void		*GetUserData( int *length=0 );
	int				GetUserDataLength();

	// Used by server only
	void			SetTickCount( int count );
	int				GetTickCount( void );

public:
	unsigned char	*m_pUserData;
	int				m_nUserDataLength;
	int				m_nTickCount;
};

#endif // NETWORKSTRINGTABLEITEM_H
