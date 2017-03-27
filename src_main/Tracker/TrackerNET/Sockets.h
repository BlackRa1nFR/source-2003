//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#ifndef SOCKETS_H
#define SOCKETS_H
#pragma once

#include "NetAddress.h"
#include "Interface.h"

//-----------------------------------------------------------------------------
// Purpose: Basic internet UDP socket layer interface
//-----------------------------------------------------------------------------
class ISockets : public IBaseInterface
{
public:
	// initializes the sockets to send and receive in the portMin-portMax range
	virtual bool InitializeSockets( unsigned short portMin, unsigned short portMax ) = 0;

	// Extracts the IP address and port number from a string IP
	virtual bool StringToAddress( const char *s, int *ip, unsigned short *port ) = 0;

	// sends a block of data over the network to the target address
	virtual bool SendData( CNetAddress target, void *data, int dataSize ) = 0;

	// broadcasts data 
	virtual bool BroadcastData(int port, void *data, int dataSize) = 0;

	// reads data from the stream; returns the number of bytes read. sourceAddress is optional and may be NULL
	virtual int ReadData( void *buffer, int bufferSize, CNetAddress *sourceAddress ) = 0;

	// returns true if there is data waiting in the network stream
	virtual bool IsDataAvailable( void ) = 0;

	// returns the port number that sockets are listening on
	virtual unsigned short GetListenPort( void ) = 0;

	// Returns the local machine IP address
	virtual CNetAddress GetLocalAddress( void ) = 0;

	// virtual destructor
	virtual void deleteThis() = 0;
};

#define SOCKETS_INTERFACE_VERSION "Sockets003"

#endif // SOCKETS_H
