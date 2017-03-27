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
#include "tier0/vprof.h"
#ifdef VPROF_ENABLED
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
#include "iviewrender.h"
#include "hud_element_helper.h"

#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMesh.h"
#include "materialsystem/IMaterial.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar vprof_graph	   ( "vprof_graph","0", 0, "Draw the vprof graph." );
static ConVar vprof_graphwidth ( "vprof_graphwidth", "192", FCVAR_ARCHIVE );
static ConVar vprof_graphheight( "vprof_graphheight", "64" );

#define	TIMINGS		1024	// Number of values to track (must be power of 2) b/c of masking
#define COMPONENTS	32		// Max components per timing.

#define GRAPH_RED	(0.9f * 255)
#define GRAPH_GREEN (0.9f * 255)
#define GRAPH_BLUE	(0.7f * 255)

#define LERP_HEIGHT 24


//-----------------------------------------------------------------------------
// Purpose: Displays the netgraph 
//-----------------------------------------------------------------------------
class CVProfGraphPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
private:

	vgui::HFont			m_hFont;

public:
						CVProfGraphPanel( vgui::VPANEL parent );
	virtual				~CVProfGraphPanel( void );

	virtual void		ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void		Paint();
	virtual void		OnTick( void );

	virtual bool		ShouldDraw( void );


	struct CLineSegment
	{
		int			x1, y1, x2, y2;
		byte		color[4];
	};

	CUtlVector< CLineSegment >	m_Rects;

	inline void			DrawLine( vrect_t *rect, unsigned char *color, unsigned char alpha );

	void				DrawLineSegments();

	void				GraphGetXY( vrect_t *rect, int width, int *x, int *y );

private:

	void				PaintLineArt( int x, int y, int w );

	CMaterialReference	m_WhiteMaterial;

	// VProf interface:
	float m_Samples[ TIMINGS ][ COMPONENTS ];
	CVProfNode*  m_Components[ COMPONENTS ];

	int   m_CurrentSample;

	void GetNextSample();

public:
	static CVProfNode*  m_CurrentNode;
};

CVProfNode* CVProfGraphPanel::m_CurrentNode = NULL;


void IN_VProfPrevSibling(void)
{
	CVProfNode* n = CVProfGraphPanel::m_CurrentNode->GetPrevSibling();
	if( n )
		CVProfGraphPanel::m_CurrentNode = n;
}

void IN_VProfNextSibling(void)
{
	CVProfNode* n = CVProfGraphPanel::m_CurrentNode->GetSibling();
	if( n )
		CVProfGraphPanel::m_CurrentNode = n;

}

void IN_VProfParent(void)
{
	CVProfNode* n = CVProfGraphPanel::m_CurrentNode->GetParent();
	if( n )
		CVProfGraphPanel::m_CurrentNode = n;

}

void IN_VProfChild(void)
{
	CVProfNode* n = CVProfGraphPanel::m_CurrentNode->GetChild();
	if( n )
	{
		// Find the largest child:
		CVProfGraphPanel::m_CurrentNode = n; 

		for( ; n; n = n->GetSibling() )
		{
			if( n->GetPrevTime() > CVProfGraphPanel::m_CurrentNode->GetPrevTime() )
				CVProfGraphPanel::m_CurrentNode = n;
		}
	}
}

static ConCommand vprof_siblingprev	("vprof_prevsibling", IN_VProfPrevSibling);
static ConCommand vprof_siblingnext	("vprof_nextsibling", IN_VProfNextSibling);
static ConCommand vprof_parent		("vprof_parent",	  IN_VProfParent);
static ConCommand vprof_child		("vprof_child",		  IN_VProfChild);

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CVProfGraphPanel::CVProfGraphPanel( vgui::VPANEL parent )
: BaseClass( NULL, "CVProfGraphPanel" )
{
	SetParent( parent ); 
	SetSize( ScreenWidth(), ScreenHeight() );
	SetPos( 0, 0 );
	SetVisible( false );
	SetCursor( null );

	m_hFont = 0;

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );

	memset( m_Samples, 0, sizeof( m_Samples ) );
	m_CurrentSample = 0;
	m_CurrentNode = g_VProfCurrentProfile.GetRoot();

	// Move down to an interesting node ( the render / sound / etc level)
	if( m_CurrentNode->GetChild() )
	{
		m_CurrentNode = m_CurrentNode->GetChild();

		if( m_CurrentNode->GetChild() )
		{
			m_CurrentNode = m_CurrentNode->GetChild();
		}
	}

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	m_WhiteMaterial.Init( "vgui/white" );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CVProfGraphPanel::~CVProfGraphPanel( void )
{
}

void CVProfGraphPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont( "DefaultVerySmall" );
	assert( m_hFont );
}


//-----------------------------------------------------------------------------
// Purpose: Figure out x and y position for graph based on vprof_graphpos
//   value.
// Input  : *rect - 
//			width - 
//			*x - 
//			*y - 
//-----------------------------------------------------------------------------
void CVProfGraphPanel::GraphGetXY( vrect_t *rect, int width, int *x, int *y )
{
	*x = rect->x + rect->width - 5 - width;
	*y = rect->y+rect->height - LERP_HEIGHT - 5;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVProfGraphPanel::OnTick( void )
{
	SetVisible( ShouldDraw() );
	
}

bool CVProfGraphPanel::ShouldDraw( void )
{
	return vprof_graph.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVProfGraphPanel::Paint() 
{
	int			x, y, w;
	vrect_t		vrect;

	if ( ( ShouldDraw() ) == false )
		return;
	
	// Get screen rectangle
	vrect.x		 = 0;
	vrect.y		 = 0;
	vrect.width	 = ScreenWidth();
	vrect.height = ScreenHeight();

	// Determine graph width
	w = min( (int)TIMINGS, vprof_graphwidth.GetInt() );
	if ( vrect.width < w + 10 )
	{
		w = vrect.width - 10;
	}

	// Get the graph's location:
	GraphGetXY( &vrect, w, &x, &y );

	PaintLineArt( x, y, w );

	// Draw the text overlays:

	// Print it out
	y -= vprof_graphheight.GetInt();

	double RootTime =  g_VProfCurrentProfile.GetRoot()->GetPrevTime();

	char sz[256];
	if ( ( gpGlobals->absoluteframetime) > 0.f )
	{
		Q_snprintf( sz, sizeof( sz ), "%s - %0.1f%%%%", m_CurrentNode->GetName(), ( m_CurrentNode->GetPrevTime() /  RootTime ) * 100.f);
		g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );
	}

	// Draw the legend:
	x += w / 2;
	for( int i = 0; i < COMPONENTS; i++ )
	{		
		// Don't draw components that are too small:
		if( ( !m_Components[i] && i) || m_Samples[m_CurrentSample][i] <= 0 )
			continue;

		byte color[3];

		color[0] = (i & 1) ? 255 : 127;
		color[1] = (i & 2) ? 255 : 127;
		color[2] = (i & 4) ? 255 : 127;


		Q_snprintf( sz, sizeof( sz ), "%03.1f%%%% (%s)", 
				( ( m_Samples[m_CurrentSample][i] * m_CurrentNode->GetPeakTime() ) / RootTime  ) * 100.f,	
				i ? m_Components[i]->GetName() : "unknown" );

		y-=10;
		g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, color[0], color[1], color[2], 180, sz );

	}
}


// VProf interface:
void CVProfGraphPanel::GetNextSample()
{
	// Increment to the next sample:
	m_CurrentSample = ( m_CurrentSample + 1 ) % TIMINGS; 


	// The base node holds the time unaccounted for by the other nodes, this may
	// be different than vprof if vprof has more than 8 nodes on a branch, but 
	// this will still be as accurate.
	m_Samples[m_CurrentSample][0] = m_CurrentNode->GetPrevTime();

	// Iterate through my children, determining the most important components
	CVProfNode* c = m_CurrentNode->GetChild();
	for( int i = 1; i < COMPONENTS; i++ )
	{
		m_Components[i] = c;
		m_Samples[m_CurrentSample][i] = 0;
		if( !c ) 
			continue;


		float NodeTime = c->GetPrevTime();
		
		// Only acknowledge this node if it's displayable, otherwise leave the time in default.
		m_Samples[m_CurrentSample][0] -= NodeTime;
		m_Samples[ m_CurrentSample ][i] = NodeTime / m_CurrentNode->GetPeakTime();

		c = c->GetSibling();

	}

	m_Samples[m_CurrentSample][0] /= m_CurrentNode->GetPeakTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVProfGraphPanel::PaintLineArt( int x, int y, int w ) 
{
	m_Rects.RemoveAll();

	int h, a;

	vrect_t	rcFill = {0,0,0,0};

	// Update the sample graph:
	GetNextSample();
	int sample = m_CurrentSample;

	for (a=w; a >= 0; a-- )
	{
		byte color[3];
	
		int baseY = y;

		for( int i = 0; i < COMPONENTS; i++ )
		{
			if( m_Samples[sample][i] <= 0 )
				continue;

			color[0] = (i & 1) ? 255 : 127;
			color[1] = (i & 2) ? 255 : 127;
			color[2] = (i & 4) ? 255 : 127;
			
			
			h = m_Samples[sample][i] * ( vprof_graphheight.GetFloat() - LERP_HEIGHT - 2 );

			// Clamp the height: (though it shouldn't need it)
			if (h > ( vprof_graphheight.GetFloat() -  LERP_HEIGHT - 2 ) )
			{
				h = vprof_graphheight.GetFloat() - LERP_HEIGHT - 2;
			}

			// Draw the height:
			rcFill.x		= x + w - a -1;
			rcFill.y		= baseY - h;
			rcFill.width	= 1;
			rcFill.height	= h ;

			DrawLine(&rcFill, color, 255 );		
			baseY -= h;
		}

		// Draw the baseline:
		rcFill.y		= y;
		rcFill.height	= 1;

		color[0] = 90;
		color[1] = 90;
		color[2] = 90;

		DrawLine( &rcFill, color, 128 );

		// Draw the limit:
		rcFill.y = y - ( vprof_graphheight.GetFloat() - LERP_HEIGHT - 2 );
		DrawLine( &rcFill, color, 128 );


		// Move on to the next sample:
		sample--;
		if( sample < 0 ) sample = TIMINGS - 1;

	}

	// Build the mesh and draw the graph:
	DrawLineSegments();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVProfGraphPanel::DrawLineSegments()
{
	int c = m_Rects.Count();

	if ( c <= 0 )
		return;

	IMesh* m_pMesh = materials->GetDynamicMesh( true, NULL, NULL, m_WhiteMaterial );
	CMeshBuilder		meshBuilder;
	meshBuilder.Begin( m_pMesh, MATERIAL_LINES, c );

	for ( int i = 0 ; i < c; i++ )
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
void CVProfGraphPanel::DrawLine( vrect_t *rect, unsigned char *color, unsigned char alpha )
{
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

class CVProfGraphPanelInterface : public INetGraphPanel
{
private:
	CVProfGraphPanel *vprofGraphPanel;
public:
	CVProfGraphPanelInterface( void )
	{
		vprofGraphPanel = NULL;
	}
	void Create( vgui::VPANEL parent )
	{
		vprofGraphPanel = new CVProfGraphPanel( parent );
	}
	void Destroy( void )
	{
		if ( vprofGraphPanel )
		{
			vprofGraphPanel->SetParent( (vgui::Panel *)NULL );
			delete vprofGraphPanel;
		}
	}
};

static CVProfGraphPanelInterface g_VProfGraphPanel;
INetGraphPanel *vprofgraphpanel = ( INetGraphPanel * )&g_VProfGraphPanel;
#endif // VPROF_ENABLED

