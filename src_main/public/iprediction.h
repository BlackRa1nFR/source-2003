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
#if !defined( IPREDICTION_H )
#define IPREDICTION_H
#ifdef _WIN32
#pragma once
#endif


#include "interface.h"
#include "vector.h" // Solely to get at define for QAngle


class IMoveHelper;

//-----------------------------------------------------------------------------
// Purpose: Engine interface into client side prediction system
//-----------------------------------------------------------------------------
class IPrediction
{
public:
	virtual			~IPrediction( void ) {};

	virtual void	Init( void ) = 0;
	virtual void	Shutdown( void ) = 0;

	// Run prediction
	virtual void	Update
					( 
						int startframe,				// World update ( un-modded ) most recently received
						bool validframe,			// Is frame data valid
						int incoming_acknowledged,	// Last command acknowledged to have been run by server (un-modded)
						int outgoing_command		// Last command (most recent) sent to server (un-modded)
					) = 0;

	// We are about to get a network update from the server.  We know the update #, so we can pull any
	//  data purely predicted on the client side and transfer it to the new from data state.
	virtual void	PreEntityPacketReceived( int commands_acknowledged, int current_world_update_packet ) = 0;
	virtual void	PostEntityPacketReceived( void ) = 0;
	virtual void	PostNetworkDataReceived( int commands_acknowledged ) = 0;

	virtual void	OnReceivedUncompressedPacket( void ) = 0;

	// The engine needs to be able to access a few predicted values
	virtual int		GetWaterLevel( void ) = 0;
	virtual void	GetViewOrigin( Vector& org ) = 0;
	virtual void	SetViewOrigin( Vector& org ) = 0;
	virtual void	GetViewAngles( QAngle& ang ) = 0;
	virtual void	SetViewAngles( QAngle& ang ) = 0;
	virtual void	GetLocalViewAngles( QAngle& ang ) = 0;
	virtual void	SetLocalViewAngles( QAngle& ang ) = 0;

	virtual void	DemoReportInterpolatedPlayerOrigin( const Vector& eyePosition ) = 0;

};

extern IPrediction *g_pClientSidePrediction;

#define VCLIENT_PREDICTION_INTERFACE_VERSION	"VClientPrediction001"

#endif // IPREDICTION_H