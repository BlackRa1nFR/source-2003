//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "NetAddress.h"

#include <stdio.h>
#include <string.h>

//-----------------------------------------------------------------------------
// Purpose: Converts the IP to a static string
//-----------------------------------------------------------------------------
const char *CNetAddress::ToStaticString( void )
{
	static char buf[24];
	sprintf( buf, "%d.%d.%d.%d:%d", m_ip[0], m_ip[1], m_ip[2], m_ip[3], m_port );
	return buf;
}
