//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: VProf engine integration
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================
#include "sys.h"
#include "vprof_engine.h"
#include "iengine.h"

#include "basetypes.h"

#include "convar.h"
#include "cmd.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef VPROF_ENABLED
static ConVar vprof_dump_spikes( "vprof_dump_spikes","0", 0, "Framerate at which vprof will begin to dump spikes to the console. 0 = disabled." );

static void (*g_pfnDeferredOp)();

static void ExecuteDeferredOp() 
{
	if ( g_pfnDeferredOp )
	{
		(*g_pfnDeferredOp)();
		g_pfnDeferredOp = NULL;
	}
}
	
const double MAX_SPIKE_REPORT = 1.5;
static double LastSpikeTime = 0;

void PreUpdateProfile()
{
	ExecuteDeferredOp();

	if( vprof_dump_spikes.GetFloat() && ( eng->GetFrameTime() > ( 1.f / vprof_dump_spikes.GetFloat() ) ) )
	{
		if( Sys_FloatTime() - LastSpikeTime > MAX_SPIKE_REPORT )
		{
			g_VProfCurrentProfile.Pause();
			g_VProfCurrentProfile.OutputReport();
			g_VProfCurrentProfile.Resume();

			LastSpikeTime = Sys_FloatTime();
		}
	}
}

void PostUpdateProfile()
{
	if ( g_VProfCurrentProfile.IsEnabled() )
	{
		g_VProfCurrentProfile.MarkFrame();
	}
}



static bool g_fVprofOnByUI;

#define DEFERRED_CON_COMMAND( cmd, help ) static void cmd##_Impl(); CON_COMMAND(cmd, help) { g_pfnDeferredOp = cmd##_Impl; } static void cmd##_Impl()

CON_COMMAND(spike,"generates a fake spike")
{
	Sys_Sleep(1000);
}

DEFERRED_CON_COMMAND(vprof, "Toggle VProf profiler")
{
	if ( !g_fVprofOnByUI )
	{
		Msg("VProf enabled.\n");
		g_VProfCurrentProfile.Start();
		g_fVprofOnByUI = true;
	}
	else
	{
		Msg("VProf disabled.\n");
		g_VProfCurrentProfile.Stop();
		g_fVprofOnByUI = false;
	}
}
DEFERRED_CON_COMMAND(vprof_on, "Turn on VProf profiler")
{
	if ( !g_fVprofOnByUI )
	{
		Msg("VProf enabled.\n");
		g_VProfCurrentProfile.Start();
		g_fVprofOnByUI = true;
	}
}

void VProfOn( void )
{
	vprof_on();
}

DEFERRED_CON_COMMAND(vprof_off, "Turn off VProf profiler")
{
	if ( g_fVprofOnByUI )
	{
		Msg("VProf disabled.\n");
		g_VProfCurrentProfile.Stop();
		g_fVprofOnByUI = false;
	}
}

DEFERRED_CON_COMMAND(vprof_reset, "Reset the stats in VProf profiler")
{
	Msg("VProf reset.\n");
	g_VProfCurrentProfile.Reset();
}

DEFERRED_CON_COMMAND(vprof_reset_peaks, "Reset just the peak time in VProf profiler")
{
	Msg("VProf peaks reset.\n");
	g_VProfCurrentProfile.ResetPeaks();
}

DEFERRED_CON_COMMAND(vprof_generate_report, "Generate a report to the console.")
{
	g_VProfCurrentProfile.Pause();
	g_VProfCurrentProfile.OutputReport();
	g_VProfCurrentProfile.Resume();
}

DEFERRED_CON_COMMAND(vprof_generate_report_AI, "Generate a report to the console.")
{
	// This is an unfortunate artifact of deferred commands not supporting arguments
	g_VProfCurrentProfile.Pause();
	g_VProfCurrentProfile.OutputReport( VPRT_FULL, "NPCs" );
	g_VProfCurrentProfile.Resume();
}

DEFERRED_CON_COMMAND(vprof_generate_report_AI_only, "Generate a report to the console.")
{
	// This is an unfortunate artifact of deferred commands not supporting arguments
	g_VProfCurrentProfile.Pause();
	g_VProfCurrentProfile.OutputReport( VPRT_FULL, "NPCs", g_VProfCurrentProfile.BudgetGroupNameToBudgetGroupID( VPROF_BUDGETGROUP_NPCS ) );
	g_VProfCurrentProfile.Resume();
}

#ifdef MOVE_BACK_TO_PANEL
	DEFERRED_CON_COMMAND(vprof_expand_all, "Expand the whole vprof tree")
	{
		Msg("VProf expand all.\n");
		CVProfPanel *pVProfPanel = ( CVProfPanel * )GET_HUDELEMENT( CVProfPanel );
		pVProfPanel->ExpandAll();
	}
#endif

#endif

