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
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "inetgraphpanel.h"
#include "kbutton.h"
#include "inetgraph.h"
#include "input.h"
#include <vgui/IVgui.h>
#include "c_clientstats.h"
#include "VguiMatSurface/IMatSystemSurface.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include "tier0/vprof.h"
#include "iviewrender.h"

#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMesh.h"
#include "materialsystem/IMaterial.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar	net_graph			( "net_graph","0", 0, "Draw the network usage graph" );
static ConVar	net_graphwidth		( "net_graphwidth", "192", FCVAR_ARCHIVE );
static ConVar	net_scale			( "net_scale", "5", FCVAR_ARCHIVE );
static ConVar	net_graphpos		( "net_graphpos", "1", FCVAR_ARCHIVE );
static ConVar	net_graphsolid		( "net_graphsolid", "1", FCVAR_ARCHIVE );
static ConVar	net_graphheight		( "net_graphheight", "64" );

#define	TIMINGS	1024       // Number of values to track (must be power of 2) b/c of masking
#define LATENCY_AVG_FRAC 0.5
#define PACKETLOSS_AVG_FRAC 0.5
#define PACKETCHOKE_AVG_FRAC 0.5

#define NUM_LATENCY_SAMPLES 8

#define GRAPH_RED	(0.9 * 255)
#define GRAPH_GREEN (0.9 * 255)
#define GRAPH_BLUE	(0.7 * 255)

#define LERP_HEIGHT 24

#define COLOR_DROPPED	0
#define COLOR_INVALID	1
#define COLOR_SKIPPED	2
#define COLOR_CHOKED	3
#define COLOR_NORMAL	4

//-----------------------------------------------------------------------------
// Purpose: Displays the netgraph 
//-----------------------------------------------------------------------------
class CNetGraphPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
private:
	typedef struct
	{
		int latency;
		int	choked;
	} packet_latency_t;

	typedef struct
	{
		unsigned short client;
		unsigned short localplayer;
		unsigned short otherplayers;
		unsigned short entities;
		unsigned short sound;
		unsigned short event;
		unsigned short usr;
		unsigned short msgbytes;
		unsigned short voicebytes;

		int				sampleY;
		int				sampleHeight;

	} netbandwidthgraph_t;

	typedef struct
	{
		float		cmd_lerp;
		int			size;
		bool		sent;
	} cmdinfo_t;

	typedef struct
	{
		byte color[3];
		byte alpha;
	} netcolor_t;

	byte colors[ LERP_HEIGHT ][3];

	byte sendcolor[ 3 ];
	byte holdcolor[ 3 ];
	byte extrap_base_color[ 3 ];

	packet_latency_t	packet_latency[ TIMINGS ];
	cmdinfo_t			cmdinfo[ TIMINGS ];
	netbandwidthgraph_t	graph[ TIMINGS ];

	float framerate;
	float packet_loss;
	float packet_choke;

	netcolor_t netcolors[5];

	vgui::HFont			m_hFont;
	const ConVar		*cl_updaterate;
	const ConVar		*cl_cmdrate;
	const ConVar		*rate;

public:
						CNetGraphPanel( vgui::VPANEL parent );
	virtual				~CNetGraphPanel( void );

	virtual void		ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void		Paint();
	virtual void		OnTick( void );

	virtual bool		ShouldDraw( void );

	void				InitColors( void );
	int					GraphValue( void );

	struct CLineSegment
	{
		int			x1, y1, x2, y2;
		byte		color[4];
	};

	CUtlVector< CLineSegment >	m_Rects;

	inline void			DrawLine( vrect_t *rect, unsigned char *color, unsigned char alpha );

	void				ResetLineSegments();
	void				DrawLineSegments();

	int					DrawDataSegment( vrect_t *rcFill, int bytes, byte r, byte g, byte b, byte alpha );
	void				DrawUpdateRate( int x, int y );
	void				DrawHatches( int x, int y, int maxmsgbytes );
	void				DrawTimes( vrect_t vrect, cmdinfo_t *cmdinfo, int x, int w );
	void				DrawTextFields( int graphvalue, int x, int y, netbandwidthgraph_t *graph, cmdinfo_t *cmdinfo, 
							int count, float avg, float *framerate, int packet_loss, int packet_choke );
	void				GraphGetXY( vrect_t *rect, int width, int *x, int *y );
	void				GetCommandInfo( cmdinfo_t *cmdinfo );
	void				GetFrameData( packet_latency_t *packet_latency, netbandwidthgraph_t *graph,
							int *choke_count, int *loss_count, int *biggest_message, float *latency, int *latency_count, float *avg_message, float *f95thpercentile );
	void				ColorForHeight( packet_latency_t *packet, byte *color, int *ping, byte *alpha );
	void				GetColorValues( int color, byte *cv, byte *alpha );

private:

	void				PaintLineArt( int x, int y, int w, int graphtype, int maxmsgbytes );
	void				DrawLargePacketSizes( int x, int w, int graphtype, float warning_threshold );

	CMaterialReference	m_WhiteMaterial;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CNetGraphPanel::CNetGraphPanel( vgui::VPANEL parent )
: BaseClass( NULL, "CNetGraphPanel" )
{
	SetParent( parent );
	SetSize( ScreenWidth(), ScreenHeight() );
	SetPos( 0, 0 );
	SetVisible( false );
	SetCursor( null );

	m_hFont = 0;

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );

	InitColors();

	cl_updaterate = cvar->FindVar( "cl_updaterate" );
	cl_cmdrate = cvar->FindVar( "cl_cmdrate" );
	rate = cvar->FindVar( "rate" );
	assert( cl_updaterate && cl_cmdrate && rate );

	memset( sendcolor, 0, 3 );
	memset( holdcolor, 0, 3 );
	memset( extrap_base_color, 255, 3 );

	memset( packet_latency, 0, TIMINGS * sizeof( packet_latency_t ) );
	memset( cmdinfo, 0, TIMINGS * sizeof( cmdinfo_t ) );
	memset( graph, 0, TIMINGS * sizeof( netbandwidthgraph_t ) );

	framerate = 0.0f;
	packet_loss = 0.0f;
	packet_choke = 0.0f;

	netcolors[0].color[0] = 255;
	netcolors[0].color[1] = 0;
	netcolors[0].color[2] = 0;
	netcolors[0].alpha = 255;
	netcolors[1].color[0] = 0;
	netcolors[1].color[1] = 0;
	netcolors[1].color[2] = 255;
	netcolors[1].alpha = 255;
	netcolors[2].color[0] = 240;
	netcolors[2].color[1] = 127;
	netcolors[2].color[2] = 63;
	netcolors[2].alpha = 255;
	netcolors[3].color[0] = 255;
	netcolors[3].color[1] = 255;
	netcolors[3].color[2] = 0;
	netcolors[3].alpha = 255;
	netcolors[4].color[0] = 63;
	netcolors[4].color[1] = 255;
	netcolors[4].color[2] = 63;
	netcolors[4].alpha = 150;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	m_WhiteMaterial.Init( "vgui/white" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetGraphPanel::~CNetGraphPanel( void )
{
}

void CNetGraphPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont( "DefaultVerySmall" );
	assert( m_hFont );
}

//-----------------------------------------------------------------------------
// Purpose: Copies data from netcolor_t array into fields passed in
// Input  : color - 
//			*cv - 
//			*alpha - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GetColorValues( int color, byte *cv, byte *alpha )
{
	int i;
	netcolor_t *pc = &netcolors[ color ];
	for ( i = 0; i < 3; i++ )
	{
		cv[ i ] = pc->color[ i ];
	}
	*alpha = pc->alpha;
}

//-----------------------------------------------------------------------------
// Purpose: Sets appropriate color values
// Input  : *packet - 
//			*color - 
//			*ping - 
//			*alpha - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::ColorForHeight( packet_latency_t *packet, byte *color, int *ping, byte *alpha )
{
	int h = packet->latency;
	*ping = 0;
	switch ( h )
	{
	case 9999:
		GetColorValues( COLOR_DROPPED, color, alpha );
		break;
	case 9998:
		GetColorValues( COLOR_INVALID, color, alpha );
		break;
	case 9997:
		GetColorValues( COLOR_SKIPPED, color, alpha );
		break;
	default:
		*ping = 1;
		GetColorValues( packet->choked ? COLOR_CHOKED : COLOR_NORMAL, color, alpha );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set up blend colors for comman/client-frame/interpolation graph
//-----------------------------------------------------------------------------
void CNetGraphPanel::InitColors( void )
{
	int i, j;
	byte mincolor[2][3];
	byte maxcolor[2][3];
	float	dc[2][3];
	int		hfrac;	
	float	f;

	mincolor[0][0] = 63;
	mincolor[0][1] = 0;
	mincolor[0][2] = 100;

	maxcolor[0][0] = 0;
	maxcolor[0][1] = 63;
	maxcolor[0][2] = 255;

	mincolor[1][0] = 255;
	mincolor[1][1] = 127;
	mincolor[1][2] = 0;

	maxcolor[1][0] = 250;
	maxcolor[1][1] = 0;
	maxcolor[1][2] = 0;

	for ( i = 0; i < 3; i++ )
	{
		dc[0][i] = (float)(maxcolor[0][i] - mincolor[0][i]);
		dc[1][i] = (float)(maxcolor[1][i] - mincolor[1][i]);
	}

	hfrac = LERP_HEIGHT / 3;

	for ( i = 0; i < LERP_HEIGHT; i++ )
	{
		if ( i < hfrac )
		{
			f = (float)i / (float)hfrac;
			for ( j = 0; j < 3; j++ )
			{
				colors[ i ][j] = mincolor[0][j] + f * dc[0][j];
			}
		}
		else
		{
			f = (float)(i-hfrac) / (float)(LERP_HEIGHT - hfrac );
			for ( j = 0; j < 3; j++ )
			{
				colors[ i ][j] = mincolor[1][j] + f * dc[1][j];
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw client framerate / usercommand graph
// Input  : vrect - 
//			*cmdinfo - 
//			x - 
//			w - 
//-----------------------------------------------------------------------------

void CNetGraphPanel::DrawTimes( vrect_t vrect, cmdinfo_t *cmdinfo, int x, int w )
{
	int i;
	int j;
	int	extrap_point;
	int a, h;
	vrect_t  rcFill;
	int ptx, pty;

	// Draw cmd_rate value
	ptx = max( x + w - 1 - 25, 1 );
	pty = max( vrect.y + vrect.height - 4 - LERP_HEIGHT + 1, 1 );
	
	extrap_point = LERP_HEIGHT / 3;

	for (a=0 ; a<w ; a++)
	{
		i = ( netgraph->GetOutgoingSequence() - a ) & ( TIMINGS - 1 );
		h = min( ( cmdinfo[i].cmd_lerp / 3.0 ) * LERP_HEIGHT, LERP_HEIGHT );

		rcFill.x		= x + w -a - 1;
		rcFill.width	= 1;
		rcFill.height	= 1;

		rcFill.y = vrect.y + vrect.height - 4;
		
		if ( h >= extrap_point )
		{
			int start = 0;

			h -= extrap_point;
			rcFill.y -= extrap_point;

			if ( !net_graphsolid.GetInt() )
			{
				rcFill.y -= (h - 1);
				start = (h - 1);
			}

			for ( j = start; j < h; j++ )
			{
				DrawLine(&rcFill, colors[j + extrap_point], 255 );	
				rcFill.y--;
			}
		}
		else
		{
			int oldh;
			oldh = h;
			rcFill.y -= h;
			h = extrap_point - h;

			if ( !net_graphsolid.GetInt() )
			{
				h = 1;
			}

			for ( j = 0; j < h; j++ )
			{
				DrawLine(&rcFill, colors[j + oldh], 255 );	
				rcFill.y--;
			}
		}

		rcFill.y = vrect.y + vrect.height - 4 - extrap_point;

		DrawLine( &rcFill, extrap_base_color, 255 );

		rcFill.y = vrect.y + vrect.height - 3;

		if ( cmdinfo[ i ].sent )
		{
			DrawLine( &rcFill, sendcolor, 255 );
		}
		else
		{
			DrawLine( &rcFill, holdcolor, 200 );
		}
	}

	g_pMatSystemSurface->DrawColoredText( m_hFont, ptx, pty, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%i/s", cl_cmdrate->GetInt() );

}

//-----------------------------------------------------------------------------
// Purpose: Compute frame database for rendering netgraph computes choked, and lost packets, too.
//  Also computes latency data and sets max packet size
// Input  : *packet_latency - 
//			*graph - 
//			*choke_count - 
//			*loss_count - 
//			*biggest_message - 
//			1 - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GetFrameData( packet_latency_t *packet_latency, netbandwidthgraph_t *graph,
	int *choke_count, int *loss_count, int *biggest_message,
	float *latency, int *latency_count,
	float *avg_message, float *f95thpercentile )
{
	int		i;
	int		frm;
	//frame_t *frame;
	float	frame_received_time;
	float	frame_latency;

	*choke_count		= 0;
	*loss_count			= 0;
	*biggest_message	= 0;
	*latency			= 0.0;
	*latency_count		= 0;
	*avg_message		= 0.0f;
	*f95thpercentile	= 0.0f;

	int msg_count = 0;

	// Fill in frame data
	for (i=netgraph->GetIncomingSequence() - netgraph->GetUpdateWindowSize() + 1
		; i <= netgraph->GetIncomingSequence()
		; i++)
	{
		frm = i;
		
		// frame = &cl.frames[ frm &netgraph->GetUpdateWindowMask()];

		/*
		packet_latency[ frm & ( TIMINGS - 1 ) ].choked = frame->choked;
		if ( frame->choked )
		{
			(*choke_count)++;
		}
		*/

		frame_received_time = netgraph->GetFrameReceivedTime( frm );

		if ( !netgraph->IsFrameValid( frm ) )
		{
			packet_latency[ frm & ( TIMINGS - 1 ) ].latency = 9998;	// invalid delta
		}
		else if ( frame_received_time == -1 )
		{
			packet_latency[ frm & ( TIMINGS - 1 ) ].latency = 9999;	// dropped
			(*loss_count)++;
		}
		else if ( frame_received_time == -3 )
		{
			packet_latency[ frm & ( TIMINGS - 1 ) ].latency = 9997;	// skipped b/c we are updating too fast
		}
		else
		{
			// Cap to 1 second latency in graph
			frame_latency = min( 1.0, netgraph->GetFrameLatency( frm ) );

			packet_latency[ frm & ( TIMINGS - 1 ) ].latency = ( ( frame_latency + 0.1 ) / 1.1 ) * ( net_graphheight.GetFloat() - LERP_HEIGHT - 2 );

			// Only use last few samples
			if ( i > ( netgraph->GetIncomingSequence() - NUM_LATENCY_SAMPLES ) )
			{
				*latency += 1000.0 * netgraph->GetFrameLatency( frm );
				(*latency_count)++;
			}
		}

		graph[ frm & ( TIMINGS - 1 )].client		= netgraph->GetFrameBytes( frm, "client" );
		graph[ frm & ( TIMINGS - 1 )].localplayer   = netgraph->GetFrameBytes( frm, "localplayer" );
		graph[ frm & ( TIMINGS - 1 )].otherplayers  = netgraph->GetFrameBytes( frm, "otherplayers" );
		graph[ frm & ( TIMINGS - 1 )].entities		= netgraph->GetFrameBytes( frm, "entities" );
		graph[ frm & ( TIMINGS - 1 )].sound			= netgraph->GetFrameBytes( frm, "sounds" );
		graph[ frm & ( TIMINGS - 1 )].event			= netgraph->GetFrameBytes( frm, "events" );
		graph[ frm & ( TIMINGS - 1 )].usr			= netgraph->GetFrameBytes( frm, "usermessages" );
		graph[ frm & ( TIMINGS - 1 )].voicebytes	= netgraph->GetFrameBytes( frm, "voice" );
		graph[ frm & ( TIMINGS - 1 )].msgbytes		= netgraph->GetFrameBytes( frm, "total" );

		if ( graph[ frm & ( TIMINGS - 1 )].msgbytes > *biggest_message )
		{
			*biggest_message = graph[ frm & ( TIMINGS - 1 )].msgbytes;
		}

		*avg_message += (float)( graph[ frm & ( TIMINGS - 1 )].msgbytes );
		msg_count++;
	}

	if ( *biggest_message > 1000 )
	{
		*biggest_message = 1000;
	}

	if ( msg_count >= 1 )
	{
		*avg_message /= msg_count;

		int deviationsquared = 0;

		// Compute std deviation
		// Fill in frame data
		for (i=netgraph->GetIncomingSequence() - netgraph->GetUpdateWindowSize() + 1
			; i <= netgraph->GetIncomingSequence()
			; i++)
		{
			frm = i;
			
			int bytes = graph[ frm & ( TIMINGS - 1 )].msgbytes;

			deviationsquared += ( bytes * bytes );
		}

		float var = ( float )( deviationsquared ) / (float)( msg_count - 1 );
		float stddev = sqrt( var );

		*f95thpercentile = *avg_message + 2.0f * stddev;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fills in command interpolation/holdback & message size data
// Input  : *cmdinfo - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GetCommandInfo( cmdinfo_t *cmdinfo )
{
	int		i;

	for ( i = netgraph->GetOutgoingSequence() - netgraph->GetUpdateWindowSize() + 1
		; i <= netgraph->GetOutgoingSequence()
		; i++)
	{
		// Also set up the lerp point.
		cmdinfo[ i & ( TIMINGS - 1 ) ].cmd_lerp = netgraph->GetCommandInterpolationAmount( i );
		cmdinfo[ i & ( TIMINGS - 1 ) ].sent = netgraph->GetCommandSent( i );
		cmdinfo[ i & ( TIMINGS - 1 ) ].size = netgraph->GetCommandSize( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws overlay text fields showing framerate, latency, bandwidth breakdowns, 
//  and, optionally, packet loss and choked packet percentages
// Input  : graphvalue - 
//			x - 
//			y - 
//			*graph - 
//			*cmdinfo - 
//			count - 
//			avg - 
//			*framerate - 
//			0.0 - 
//			avg - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawTextFields( int graphvalue, int x, int y, netbandwidthgraph_t *graph, cmdinfo_t *cmdinfo, 
	int count, float avg, float *framerate,
	int packet_loss, int packet_choke )
{
	static int lastout;

	char sz[ 256 ];
	int out;
	int localplayer;
	int otherplayers;

	if ( count > 0 )
	{
		avg /= (float)count;
		
		// Remove CPU influences ( average )
		avg -= gpGlobals->absoluteframetime / 2.0;
		avg -= cl_updaterate->GetFloat();

		// Can't be below zero
		avg = max( 0.0, avg );
	}
	else
	{
		avg = 0.0;
	}

	// Move rolling average
	*framerate   = LATENCY_AVG_FRAC * (*framerate) + ( 1.0 - LATENCY_AVG_FRAC ) * gpGlobals->absoluteframetime;

	// Print it out
	y -= net_graphheight.GetInt();

	if ( (*framerate) > 0.0 )
	{
		Q_snprintf( sz, sizeof( sz ), "%.1f fps", 1.0 / (*framerate) );

		g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );

		if ( avg > 1.0 )
		{
			Q_snprintf( sz, sizeof( sz ), "%i ms ", (int)avg );
			g_pMatSystemSurface->DrawColoredText( m_hFont, x + 75, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );
		}
		else if ( render->IsPlayingDemo() )
		{
			Q_strcpy( sz, "demo" );
			g_pMatSystemSurface->DrawColoredText( m_hFont, x + 75, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );
		}

		y += 15;

		out = cmdinfo[ ( ( netgraph->GetOutgoingSequence() - 1 ) & ( TIMINGS - 1 ) ) ].size;
		if ( !out )
		{
			out = lastout;
		}
		else
		{
			lastout = out;
		}

		localplayer = graph[ ( netgraph->GetIncomingSequence() & ( TIMINGS - 1 ) ) ].localplayer;
		otherplayers = graph[ ( netgraph->GetIncomingSequence() & ( TIMINGS - 1 ) ) ].otherplayers;
		
		float incoming, outgoing;
		netgraph->GetAverageDataFlow( &incoming, &outgoing );

		Q_snprintf( sz, sizeof( sz ), "in :  %4i %.2f k/s (lp%i:op%i)", graph[ ( netgraph->GetIncomingSequence() & ( TIMINGS - 1 ) ) ].msgbytes, incoming, localplayer, otherplayers );

		g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );
		y += 15;

		Q_snprintf( sz, sizeof( sz ), "out:  %4i %.2f k/s", out, outgoing );

		g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );
		y += 15;

		if ( graphvalue > 2 )
		{
			Q_snprintf( sz, sizeof( sz ), "loss: %i choke: %i", (int)(packet_loss + 0.49 ), (int)( packet_choke + 0.49 ) );
			g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );
			y += 15;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determine type of graph to show, or if +graph key is held down, use detailed graph
// Output : int
//-----------------------------------------------------------------------------
int CNetGraphPanel::GraphValue( void )
{
	int graphtype;

	graphtype = net_graph.GetInt();
	
	if ( !graphtype && !( in_graph.state & 1 ) )
		return 0;

	// With +graph key, use max area
	if ( !graphtype )
	{
		graphtype = 2;
	}

	return graphtype;
}

//-----------------------------------------------------------------------------
// Purpose: Figure out x and y position for graph based on net_graphpos
//   value.
// Input  : *rect - 
//			width - 
//			*x - 
//			*y - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GraphGetXY( vrect_t *rect, int width, int *x, int *y )
{
	*x = rect->x + 5;

	switch ( net_graphpos.GetInt() )
	{
	case 0:
		break;
	case 1:
		*x = rect->x + rect->width - 5 - width;
		break;
	case 2:
		*x = rect->x + ( rect->width - 10 - width ) / 2;
		break;
	default:
		*x = rect->x + clamp( XRES( net_graphpos.GetInt() ), 5, rect->width - width - 5 );
	}

	*y = rect->y+rect->height - LERP_HEIGHT - 5;
}

//-----------------------------------------------------------------------------
// Purpose: If showing bandwidth data, draw hatches big enough for largest message
// Input  : x - 
//			y - 
//			maxmsgbytes - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawHatches( int x, int y, int maxmsgbytes )
{
	int starty;
	int ystep;
	vrect_t rcHatch;

	byte colorminor[3];
	byte color[3];

	ystep = (int)( 10.0 / net_scale.GetFloat() );
	ystep = max( ystep, 1 );

	rcHatch.y		= y;
	rcHatch.height	= 1;
	rcHatch.x		= x;
	rcHatch.width	= 4;

	color[0] = 0;
	color[1] = 200;
	color[2] = 0;

	colorminor[0] = 63;
	colorminor[1] = 63;
	colorminor[2] = 0;

	for ( starty = rcHatch.y; rcHatch.y > 0 && ((starty - rcHatch.y)*net_scale.GetFloat() < ( maxmsgbytes + 50 ) ); rcHatch.y -= ystep )
	{
		if ( !((int)((starty - rcHatch.y)*net_scale.GetFloat() ) % 50 ) )
		{
			DrawLine( &rcHatch, color, 255 );
		}
		else if ( ystep > 5 )
		{
			DrawLine( &rcHatch, colorminor, 200 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: State how many updates a second are being requested
// Input  : x - 
//			y - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawUpdateRate( int x, int y )
{
	// Last one
	g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%i/s", cl_updaterate->GetInt() );
}

//-----------------------------------------------------------------------------
// Purpose: Draws bandwidth breakdown data
// Input  : *rcFill - 
//			bytes - 
//			r - 
//			g - 
//			b - 
//			alpha - 
// Output : int
//-----------------------------------------------------------------------------
int CNetGraphPanel::DrawDataSegment( vrect_t *rcFill, int bytes, byte r, byte g, byte b, byte alpha )
{
	int h;
	byte color[3];

	h = bytes / net_scale.GetFloat();
	
	color[0] = r;
	color[1] = g;
	color[2] = b;

	rcFill->height = h;
	rcFill->y -= h;

	if ( rcFill->y < 2 )
		return 0;

	DrawLine( rcFill, color, alpha );

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::OnTick( void )
{
	SetVisible( ShouldDraw() );
}

bool CNetGraphPanel::ShouldDraw( void )
{
	if ( GraphValue() != 0 )
		return true;

	return false;
}

void CNetGraphPanel::DrawLargePacketSizes( int x, int w, int graphtype, float warning_threshold )
{
	byte		color[3];
	byte		alpha;
	vrect_t		rcFill = {0,0,0,0};
	int a, i;
	int			ping;

	if ( graphtype >= 2 )
	{
		for (a=0 ; a<w ; a++)
		{
			i = (netgraph->GetIncomingSequence()-a) & ( TIMINGS - 1 );
			
			ColorForHeight( &packet_latency[i], color, &ping, &alpha );

			rcFill.x			= x + w -a -1;
			rcFill.width		= 1;
			rcFill.y			= graph[i].sampleY;
			rcFill.height		= graph[i].sampleHeight;

			if ( graph[i].msgbytes > 300 &&
				warning_threshold != 0.0f &&
				graph[i].msgbytes >= warning_threshold && 
				( rcFill.y >= 12 ) )
			{
				char sz[ 32 ];
				Q_snprintf( sz, sizeof( sz ), "%i", graph[i].msgbytes );

				int len = g_pMatSystemSurface->DrawTextLen( m_hFont, sz );

				int textx, texty;

				textx = rcFill.x - len / 2;
				texty = rcFill.y - 11;

				g_pMatSystemSurface->DrawColoredText( m_hFont, textx, texty, 255, 255, 255, 255, sz );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::Paint() 
{
	VPROF( "CNetGraphPanel::Paint" );
	ClientStats().IncrementCountedStat( CS_DRAW_NETGRAPH_COUNT, 1 );

	int			graphtype;

	int			x, y;
	int			w;
	vrect_t		vrect;

	int			maxmsgbytes = 0;

	float		loss = 0.0;
	float		choke = 0.0;
	int			loss_count = 0;
	int			choke_count = 0;
	float		avg_ping = 0.0;
	int			ping_count = 0;
	float		avg_message = 0.0f;
	float		warning_threshold = 0.0f;

	if ( ( graphtype = GraphValue() ) == 0 )
		return;
	
	// Since we divide by scale, make sure it's sensible
	if ( net_scale.GetFloat() <= 0 )
	{
		net_scale.SetValue( 0.1f );
	}

	// Get screen rectangle
	vrect.x			= 0;
	vrect.y			= 0;
	vrect.width		= ScreenWidth();
	vrect.height	= ScreenHeight();

	// Determine graph width
	w = min( (int)TIMINGS, net_graphwidth.GetInt() );
	if ( vrect.width < w + 10 )
	{
		w = vrect.width - 10;
	}

	// Grab frame data
	GetFrameData( packet_latency, graph, &choke_count, &loss_count, &maxmsgbytes, &avg_ping, &ping_count, &avg_message, &warning_threshold );

	// Packet loss
	loss = 100.0 * (float)loss_count / (float)netgraph->GetUpdateWindowSize();
	packet_loss = PACKETLOSS_AVG_FRAC * packet_loss + ( 1.0 - PACKETLOSS_AVG_FRAC ) * loss;

	// Packet choking
	choke = 100.0 * (float)choke_count / (float)netgraph->GetUpdateWindowSize();
	packet_choke = PACKETCHOKE_AVG_FRAC * packet_choke + ( 1.0 - PACKETCHOKE_AVG_FRAC ) * choke;

	// Grab cmd data
	GetCommandInfo( cmdinfo );

	GraphGetXY( &vrect, w, &x, &y );

	if ( graphtype < 3 )
	{
		PaintLineArt( x, y, w, graphtype, maxmsgbytes );

		DrawLargePacketSizes( x, w, graphtype, warning_threshold );

		// Draw client frame timing info
		DrawTimes( vrect, cmdinfo, x, w );

		// Draw update rate
		DrawUpdateRate( max( 1, x + w - 25 ), max( 1, y - net_graphheight.GetFloat() - 1 ) );
	}

	DrawTextFields( graphtype, x, y, graph, cmdinfo, ping_count, avg_ping, &framerate, packet_loss, packet_choke );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::PaintLineArt( int x, int y, int w, int graphtype, int maxmsgbytes ) 
{
	VPROF( "CNetGraphPanel::PaintLineArt" );

	ResetLineSegments();

	int i;
	int h;
	int a;
	int lastvalidh = 0;

	byte		color[3];
	int			ping;
	byte		alpha;
	vrect_t		rcFill = {0,0,0,0};

	for (a=0 ; a<w ; a++)
	{
		i = (netgraph->GetIncomingSequence()-a) & ( TIMINGS - 1 );
		h = packet_latency[i].latency;
		
		ColorForHeight( &packet_latency[i], color, &ping, &alpha );

		// Skipped
		if ( !ping ) 
		{
			// Re-use the last latency
			h = lastvalidh;  
		}
		else
		{
			lastvalidh = h;
		}

		if (h > ( net_graphheight.GetFloat() -  LERP_HEIGHT - 2 ) )
		{
			h = net_graphheight.GetFloat() - LERP_HEIGHT - 2;
		}

		rcFill.x		= x + w -a -1;
		rcFill.y		= y - h;
		rcFill.width	= 1;
		rcFill.height	= ping ? 1 : h;

		DrawLine(&rcFill, color, alpha );		

		rcFill.y		= y;
		rcFill.height	= 1;

		color[0] = 0;
		color[1] = 255;
		color[2] = 0;

		DrawLine( &rcFill, color, 160 );

		if ( graphtype < 2 )
			continue;

		// Draw a separator.
		rcFill.y = y - net_graphheight.GetFloat() - 1;
		rcFill.height = 1;

		color[0] = 255;
		color[1] = 255;
		color[2] = 255;

		DrawLine(&rcFill, color, 255 );		

		// Move up for begining of data
		rcFill.y -= 1;

		// Packet didn't have any real data...
		if ( packet_latency[i].latency > 9995 )
			continue;

		if ( !DrawDataSegment( &rcFill, graph[ i ].client, 255, 0, 0, 128 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, graph[ i ].localplayer, 100, 180, 250, 128 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, graph[ i ].otherplayers, 255, 255, 0, 128 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, graph[ i ].entities, 255, 0, 255, 128 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, graph[ i ].sound, 0, 255, 0, 128 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, graph[ i ].event, 0, 255, 255, 128 ) )
			continue;
		
		if ( !DrawDataSegment( &rcFill, graph[ i ].usr, 200, 200, 200, 128 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, graph[ i ].voicebytes, 255, 255, 255, 255 ) )
			continue;

		// Final data chunk is total size, don't use solid line routine for this
		h = graph[i].msgbytes / net_scale.GetFloat();

		color[ 0 ] = color[ 1 ] = color[ 2 ] = 240;

		rcFill.height = 1;
		rcFill.y = y - net_graphheight.GetFloat() - 1 - h;

		if ( rcFill.y < 2 )
			continue;

		DrawLine(&rcFill, color, 128 );		

		// Cache off height
		graph[i].sampleY = rcFill.y;
		graph[i].sampleHeight = rcFill.height;
	}

	if ( graphtype >= 2 )
	{
		// Draw hatches for first one:
		// on the far right side
		DrawHatches( x, y - net_graphheight.GetFloat() - 1, maxmsgbytes );
	}

	DrawLineSegments();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::ResetLineSegments()
{
	m_Rects.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawLineSegments()
{
	int c = m_Rects.Count();
	if ( c <= 0 )
		return;

	IMesh* m_pMesh = materials->GetDynamicMesh( true, NULL, NULL, m_WhiteMaterial );
	CMeshBuilder		meshBuilder;
	meshBuilder.Begin( m_pMesh, MATERIAL_LINES, c );

	int i;
	for ( i = 0 ; i < c; i++ )
	{
		CLineSegment *seg = &m_Rects[ i ];

		meshBuilder.Color4ubv( seg->color );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Position3f( seg->x1, seg->y1, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ubv( seg->color );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Position3f( seg->x2, seg->y2, 0 );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();

	m_pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: Draws a colored, filled rectangle
// Input  : *rect - 
//			*color - 
//			alpha - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawLine( vrect_t *rect, unsigned char *color, unsigned char alpha )
{
	VPROF( "CNetGraphPanel::DrawLine" );

	int idx = m_Rects.AddToTail();
	CLineSegment *seg = &m_Rects[ idx ];

	seg->color[0] = color[0];
	seg->color[1] = color[1];
	seg->color[2] = color[2];
	seg->color[3] = alpha;

	if ( rect->width == 1 )
	{
		seg->x1 = rect->x;
		seg->y1 = rect->y;
		seg->x2 = rect->x;
		seg->y2 = rect->y + rect->height;
	}
	else if ( rect->height == 1 )
	{
		seg->x1 = rect->x;
		seg->y1 = rect->y;
		seg->x2 = rect->x + rect->width;
		seg->y2 = rect->y;
	}
	else
	{
		Assert( 0 );
		m_Rects.Remove( idx );
	}
}

class CNetGraphPanelInterface : public INetGraphPanel
{
private:
	CNetGraphPanel *netGraphPanel;
public:
	CNetGraphPanelInterface( void )
	{
		netGraphPanel = NULL;
	}
	void Create( vgui::VPANEL parent )
	{
		netGraphPanel = new CNetGraphPanel( parent );
	}
	void Destroy( void )
	{
		if ( netGraphPanel )
		{
			netGraphPanel->SetParent( (vgui::Panel *)NULL );
			delete netGraphPanel;
		}
	}
};

static CNetGraphPanelInterface g_NetGraphPanel;
INetGraphPanel *netgraphpanel = ( INetGraphPanel * )&g_NetGraphPanel;
