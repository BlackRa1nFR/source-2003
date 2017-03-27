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
#include "cdll_int.h"
#include "iprediction.h"
#include "measure_section.h"
#include "cl_pred.h"
#include "demo.h"
#include "host.h"

static int g_FramneCountOfLastPrediction = -1;
//-----------------------------------------------------------------------------
// Purpose: Mark message sequence of frame we are predicting
//-----------------------------------------------------------------------------
void CL_StorePredictionFrame( void )
{
	g_FramneCountOfLastPrediction = host_tickcount;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CL_HasRunPredictionThisFrame( void )
{
	return g_FramneCountOfLastPrediction == host_tickcount;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : world_start_state - 
//			validframe - 
//			start_command - 
//			stop_command - 
//-----------------------------------------------------------------------------
void CL_Predict( int world_start_state, bool validframe, int start_command, int stop_command )
{
	MEASURECODE( "CL_Predict" );

	// Allow client .dll to do prediction.
	g_pClientSidePrediction->Update( 
		world_start_state, 
		validframe, 
		start_command, 
		stop_command );
}

//-----------------------------------------------------------------------------
// Purpose: Determine if we received a new message, which means we need to
//  redo prediction
//-----------------------------------------------------------------------------
void CL_RunPrediction( PREDICTION_REASON reason )
{
	if ( CL_HasRunPredictionThisFrame() )
	{
		return;
	}

	CL_StorePredictionFrame();

	if ( cls.state != ca_active )
		return;

	if ( !cl.validsequence ||
		 ( ( cls.netchan.outgoing_sequence + 1 - cls.netchan.incoming_acknowledged ) >= CL_UPDATE_MASK ) )
	{
		return;
	}

	// Don't do prediction if skipping ahead during demo playback
	if ( demo->IsPlayingBack() && 
		( demo->IsSkippingAhead() || demo->IsFastForwarding() ) )
	{
		return;
	}

	//Msg( "%i pred reason %s\n", host_framecount, reason == PREDICTION_SIMULATION_RESULTS_ARRIVING_ON_SEND_FRAME ?
	//	"same frame" : "normal" );
	frame_t *frame = &cl.frames[ cl.last_prediction_start_state & CL_UPDATE_MASK ];

	//Msg( "%i/%i CL_RunPrediction:  last ack %i most recent %i\n", 
	//	host_framecount, cl.tickcount,
	//	cl.last_command_ack, 
	//	cls.netchan.outgoing_sequence - 1 );

	CL_Predict( cl.last_prediction_start_state, 
		!frame->invalid ? true : false, 
		cl.last_command_ack, 
		cls.netchan.outgoing_sequence - 1 );
}
