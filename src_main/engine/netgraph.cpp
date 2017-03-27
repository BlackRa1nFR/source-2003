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
#include "quakedef.h"
#include "client.h"
#include "inetgraph.h"

class CNetGraph : public INetGraph
{
public:
	int	GetOutgoingSequence( void )
	{
		return cls.netchan.outgoing_sequence;
	}
	
	int GetIncomingSequence( void )
	{
		return cls.netchan.incoming_sequence;
	}

	int	GetUpdateWindowSize( void )
	{
		return CL_UPDATE_BACKUP;
	}

	int GetUpdateWindowMask( void )
	{
		return CL_UPDATE_MASK;
	}

	bool IsFrameValid( int frame_number )
	{
		frame_t *frame = &cl.frames[ frame_number & CL_UPDATE_MASK ];
		return !frame->invalid;
	}

	float GetFrameReceivedTime( int frame_number )
	{
		frame_t *frame = &cl.frames[ frame_number & CL_UPDATE_MASK ];
		return frame->receivedtime;

	}
	float GetFrameLatency( int frame_number )
	{
		frame_t *frame = &cl.frames[ frame_number & CL_UPDATE_MASK ];
		return frame->latency;
	}
	int GetFrameBytes( int frame_number, const char *fieldName )
	{
		frame_t *frame = &cl.frames[ frame_number & CL_UPDATE_MASK ];
		if ( !stricmp( fieldName, "client" ) )
		{
			return frame->clientbytes;
		}
		else if ( !stricmp( fieldName, "localplayer" ) )
		{
			return frame->localplayerinfobytes;
		}
		else if ( !stricmp( fieldName, "otherplayers" ) )
		{
			return frame->otherplayerinfobytes;
		}
		else if ( !stricmp( fieldName, "entities" ) )
		{
			return frame->packetentitybytes;
		}
		else if ( !stricmp( fieldName, "sounds" ) )
		{
			return frame->soundbytes;
		}
		else if ( !stricmp( fieldName, "events" ) )
		{
			return frame->eventbytes;
		}
		else if ( !stricmp( fieldName, "usermessages" ) )
		{
			return frame->usrbytes;
		}
		else if( !stricmp( fieldName, "voice" ) )
		{
			return frame->voicebytes;
		}
		else if ( !stricmp( fieldName, "total" ) )
		{
			return frame->msgbytes;
		}

		return 0;
	}

	void GetAverageDataFlow( float *incoming, float *outgoing )
	{
		if ( incoming )
		{
			*incoming = cls.netchan.flow[FLOW_INCOMING].avgkbytespersec;
		}
		if ( outgoing )
		{
			*outgoing = cls.netchan.flow[FLOW_OUTGOING].avgkbytespersec;
		}
	}

	float GetCommandInterpolationAmount( int command_number )
	{
		cmd_t *cmd = &cl.commands[ command_number & CL_UPDATE_MASK ];
		return cmd->frame_lerp;
	}

	bool GetCommandSent( int command_number )
	{
		cmd_t *cmd = &cl.commands[ command_number & CL_UPDATE_MASK ];
		return !cmd->heldback;

	}
	int GetCommandSize( int command_number )
	{
		cmd_t *cmd = &cl.commands[ command_number & CL_UPDATE_MASK ];
		return cmd->sendsize;
	}
};

EXPOSE_SINGLE_INTERFACE( CNetGraph, INetGraph, VNETGRAPH_INTERFACE_VERSION );