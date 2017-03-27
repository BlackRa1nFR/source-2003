//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( INETGRAPH_H )
#define INETGRAPH_H
#ifdef _WIN32
#pragma once
#endif

class INetGraph
{
public:
	virtual int			GetOutgoingSequence( void ) = 0;
	virtual int			GetIncomingSequence( void ) = 0;
	virtual int			GetUpdateWindowSize( void ) = 0;
	virtual int			GetUpdateWindowMask( void ) = 0;
	virtual bool		IsFrameValid( int frame_number ) = 0;
	virtual float		GetFrameReceivedTime( int frame_number ) = 0;
	virtual float		GetFrameLatency( int frame_number ) = 0;
	virtual int			GetFrameBytes( int frame_number, const char *fieldName ) = 0;
	virtual void		GetAverageDataFlow( float *incoming, float *outgoing ) = 0;

	virtual float		GetCommandInterpolationAmount( int command_number ) = 0;
	virtual bool		GetCommandSent( int command_number ) = 0;
	virtual int			GetCommandSize( int command_number ) = 0;
};

extern INetGraph *netgraph;

#define VNETGRAPH_INTERFACE_VERSION "VNETGRAPH001"

#endif // INETGRAPH_H