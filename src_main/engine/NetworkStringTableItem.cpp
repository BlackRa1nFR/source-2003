//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "networkstringtableitem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetworkStringTableItem::CNetworkStringTableItem( void )
{
	m_pUserData = NULL;
	m_nUserDataLength = 0;
	m_nTickCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetworkStringTableItem::~CNetworkStringTableItem( void )
{
	delete[] m_pUserData;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *string - 
//-----------------------------------------------------------------------------
bool CNetworkStringTableItem::SetUserData( int length, const void *userData )
{
	// no old or new data
	if ( !userData && !m_pUserData )
		return true;

	if ( m_pUserData && 
		length == m_nUserDataLength &&
		!memcmp( m_pUserData, userData, length ) )
	{
		return true;
	}

	delete[] m_pUserData;

	m_nUserDataLength = length;

	int len = max( 1, length );
	m_pUserData = new unsigned char[ len ];
	if ( userData )
	{
		memcpy( m_pUserData, userData, length );
	}
	else
	{
		memset( m_pUserData, 0, length );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : stringNumber - 
// Output : const void
//-----------------------------------------------------------------------------
const void *CNetworkStringTableItem::GetUserData( int *length )
{
	if ( length )
		*length = m_nUserDataLength;

	return ( const void * )m_pUserData;
}


int CNetworkStringTableItem::GetUserDataLength()
{
	return m_nUserDataLength;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : count - 
//-----------------------------------------------------------------------------
void CNetworkStringTableItem::SetTickCount( int count )
{
	m_nTickCount = count;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTableItem::GetTickCount( void )
{
	return m_nTickCount;
}
