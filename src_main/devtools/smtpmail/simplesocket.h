//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: simple TCP socket API for communicating as a TCP client over a TEXT
//			connection
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef SIMPLESOCKET_H
#define SIMPLESOCKET_H
#pragma once

// opaque socket type
typedef struct socket_s socket_t, *HSOCKET;

// Socket reporting function
typedef void ( *REPORTFUNCTION )( HSOCKET socket, const char *pString );

extern HSOCKET		SocketOpen( const char *pServerName, int port );
extern void			SocketClose( HSOCKET socket );
extern void			SocketSendString( HSOCKET socket, const char *pString );

// This should probably return the data so we can handle errors and receive data
extern void			SocketWait( HSOCKET socket, const char *pString );

// sets the reporting function
extern void			SocketReport( REPORTFUNCTION pReportFunction );
extern void			SocketInit( void );
extern void			SocketExit( void );




#endif // SIMPLESOCKET_H
