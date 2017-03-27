//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: MiniMap.cpp: implementation of the CMiniMap class.
//
// $NoKeywords: $
//=============================================================================

#include <vgui/isurface.h>
#include <vgui/ILocalize.h>
#include "filesystem.h"
#include <keyvalues.h>
#include <game_controls/mapoverview.h>
#include <mathlib.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


using namespace vgui;

CMapOverview::CMapOverview(vgui::Panel *parent, IGameEventManager * gameeventmanager ) : BaseClass( parent, "HLTVPanel" )
{
	SetBounds( 0,0, 256, 256 );
	SetBgColor( Color( 0,0,0,100 ) );
	SetPaintBackgroundEnabled( true );
	SetVisible( false );
	
	m_nMapTextureID = -1;
	m_MapKeyValues = NULL;

	m_MapOrigin = Vector( 0, 0, 0 );
	m_fMapScale = 1.0f;

	m_fZoom = 3.0f;
	m_ViewOrigin = Vector2D( 512, 512 );
	m_fViewAngle = 0;

	m_GameEventManager = gameeventmanager;

	GameEventsUpdated();
}

void CMapOverview::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings( scheme );

	SetBgColor( Color( 0,0,0,100 ) );
	SetPaintBackgroundEnabled( true );
}

CMapOverview::~CMapOverview()
{
	if ( m_MapKeyValues )
		m_MapKeyValues->deleteThis();

	//TODO release Textures ? clear lists
}

void CMapOverview::Paint()
{
	DrawMapTexture();
	
	BaseClass::Paint();
}

bool CMapOverview::IsVisible()
{
	return BaseClass::IsVisible();
}

void CMapOverview::GetBounds(int& x, int& y, int& wide, int& tall)
{
	BaseClass::GetBounds( x, y, wide, tall );
}

void CMapOverview::DrawMapTexture()
{
	// now draw a box around the outside of this panel
	int x0, y0, x1, y1;
	int wide, tall;

	GetSize(wide, tall);
	x0 = 0; y0 = 0; x1 = wide - 2; y1 = tall - 2 ;

	if ( m_nMapTextureID < 0 )
		return;
	
	Vertex_t pts[4] =
	{
		Vertex_t( MapToPanel ( Vector2D(0,0) ), Vector2D(0,0) ),
		Vertex_t( MapToPanel ( Vector2D(OVERVIEW_MAP_SIZE-1,0) ), Vector2D(1,0) ),
		Vertex_t( MapToPanel ( Vector2D(OVERVIEW_MAP_SIZE-1,OVERVIEW_MAP_SIZE-1) ), Vector2D(1,1) ),
		Vertex_t( MapToPanel ( Vector2D(0,OVERVIEW_MAP_SIZE-1) ), Vector2D(0,1) )
	};

	surface()->DrawSetTexture( m_nMapTextureID );
	surface()->DrawTexturedPolygon( 4, pts );

	// draw red center point
	surface()->DrawSetColor( 255,0,0,255 );
	surface()->DrawFilledRect( (wide/2)-2, (tall/2)-2, (wide/2)+2, (tall/2)+2);
	
}

// map coordinates are always [0,1023]x[0,1023]

Vector2D CMapOverview::WorldToMap( Vector &worldpos )
{
	Vector2D offset( worldpos.x - m_MapOrigin.x, worldpos.y - m_MapOrigin.y);

	offset.x /=  m_fMapScale;
	offset.y /= -m_fMapScale;

	return offset;
}


Vector2D CMapOverview::MapToPanel( const Vector2D &mappos )
{
	int pwidth, pheight; 
	Vector2D panelpos;

	GetSize(pwidth, pheight);

	Vector offset;
	offset.x = mappos.x - m_ViewOrigin.x;
	offset.y = mappos.y - m_ViewOrigin.y;
	offset.z = 0;

	VectorYawRotate( offset, m_fViewAngle - 90.0f, offset );

	offset.x *= m_fZoom/OVERVIEW_MAP_SIZE;
	offset.y *= m_fZoom/OVERVIEW_MAP_SIZE;

	panelpos.x = (pwidth * 0.5f) + (pwidth * offset.x);
	panelpos.y = (pheight * 0.5f) + (pheight * offset.y);

	return panelpos;
}

void CMapOverview::SetMap(const char * levelname)
{
	// load new KeyValues

	if ( m_MapKeyValues && Q_strcmp( levelname, m_MapKeyValues->GetName() ) == 0 )
	{
		return;	// map didn't change
	}

	if ( m_MapKeyValues )
		m_MapKeyValues->deleteThis();

	m_MapKeyValues = new KeyValues( levelname );

	char tempfile[MAX_PATH];
	Q_snprintf( tempfile, sizeof( tempfile ), "resource/overviews/%s.txt", levelname );
	
	if ( !m_MapKeyValues->LoadFromFile( filesystem(), tempfile, "GAME" ) )
	{
		DevMsg( 1, "CMapOverview::OnNewLevel: couldn't load file %s.\n", tempfile );
		m_nMapTextureID = -1;
		return;
	}

	// TODO release old texture ?

	m_nMapTextureID = surface()->CreateNewTextureID();

	//if we have not uploaded yet, lets go ahead and do so
	// surface()->DrawSetTextureFile(m_nMapTextureID,( const char * )"sprites/shopping_cart", false, false);
	surface()->DrawSetTextureFile( m_nMapTextureID, m_MapKeyValues->GetString("material"), true, false);

	m_MapOrigin.x = m_MapKeyValues->GetInt("pos_x");
	m_MapOrigin.y = m_MapKeyValues->GetInt("pos_y");
	m_fMapScale = m_MapKeyValues->GetFloat("scale");
}

void CMapOverview::OnMousePressed( MouseCode code )
{
	
}

void CMapOverview::FireGameEvent( KeyValues * event)
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "game_newmap") == 0 )
	{
		SetMap( event->GetString("mapname") );
	}
}

void CMapOverview::GameEventsUpdated()
{
	m_GameEventManager->AddListener( this );	// register for all events
}

void CMapOverview::SetCenter(Vector2D &mappos)
{
	m_ViewOrigin = mappos;
}

void CMapOverview::SetZoom( float zoom )
{
	m_fZoom = zoom;
}

void CMapOverview::SetAngle(float angle)
{
	m_fViewAngle = angle;
}

void CMapOverview::SetBounds(int x, int y, int wide, int tall)
{
	BaseClass::SetBounds(x, y, wide, tall);
}


void CMapOverview::SetVisible(bool state )
{
	BaseClass::SetVisible( state );
}

void CMapOverview::ShowPlayerNames(bool state)
{
	m_bShowPlayerNames = state;
}

void CMapOverview::ShowTracers(bool state)
{
	m_bShowTracers = state;
}

void CMapOverview::ShowExplosions(bool state)
{
	m_bShowExplosions = state;
}

void CMapOverview::ShowHealth(bool state)
{
	m_bShowHealth = state;
}

void CMapOverview::ShowHurts(bool state)
{
	m_bShowHurts = state;
}

void CMapOverview::ShowTrails(float seconds)
{
	m_bShowTrails = seconds;
}

bool CMapOverview::SetTeamColor(int team, Color color)
{
	if ( team < 0 || team>= MAX_TEAMS )
		return false;

	m_TeamColors[team] = color;

	return true;
}