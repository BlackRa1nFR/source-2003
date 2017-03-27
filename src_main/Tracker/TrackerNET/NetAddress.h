//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//=============================================================================

#ifndef NETADDRESS_H
#define NETADDRESS_H
#pragma once

#pragma warning(disable: 4514)	// warning C4514: unreferenced inline function has been removed

//-----------------------------------------------------------------------------
// Purpose: Defines a network address
//			It is fine for this to be bitwise-copied
//-----------------------------------------------------------------------------
class CNetAddress
{
private:
	unsigned char  m_ip[4];
	unsigned short m_port;	// port in normal machine order (little-endian)

public:
	CNetAddress()
	{
		m_ip[0] = m_ip[1] = m_ip[2] = m_ip[3] = 0;
		m_port = 0;
	}

	CNetAddress(unsigned int IP, short port)
	{
		SetIP(IP);
		SetPort(port);
	}

	const char *ToStaticString( void );	// returns a pointer to a static string containing the IP:port

	void SetIP( unsigned int ip ) { *((unsigned int *)m_ip) = ip; }
	void SetPort( short port ) { m_port = port; }

	inline unsigned short Port( void ) const { return m_port; }
	inline unsigned int IP( void ) const { return *((unsigned int*)m_ip); }

	// compares two IP addresses, including port numbers
	inline bool operator == (const CNetAddress &netAddress) const
	{ 
		return IP() == netAddress.IP() && Port() == netAddress.Port(); 
	}

	// comparison function, for sorting IP's
	inline bool operator < (const CNetAddress &netAddress) const
	{ 
		if (IP() < netAddress.IP())
			return true;
		if (IP() > netAddress.IP())
			return false;
		if (Port() < netAddress.Port())
			return true;

		return false;
	}
};


#endif // NETADDRESS_H
