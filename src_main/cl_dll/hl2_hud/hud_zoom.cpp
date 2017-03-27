//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "parsemsg.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "vguimatsurface/IMatSystemSurface.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMesh.h"
#include "materialsystem/imaterialvar.h"

#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>

//-----------------------------------------------------------------------------
// Purpose: Draws the zoom screen
//-----------------------------------------------------------------------------
class CHudZoom : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudZoom, vgui::Panel );

public:
	CHudZoom( const char *pElementName );
	void Init( void );

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void Paint();

private:
	void DrawDarkenedEdge(int x0, int y0, int x1, int y1, float a1, float a2, float a3, float a4);

	float m_flPreviousFOV;
	float m_flLowestFOV;
	bool m_bFovIncreasing;

	CPanelAnimationVarAliasType( float, m_flBorderThickness, "BorderThickness", "88", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flCircle1Radius, "Circle1Radius", "66", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flCircle2Radius, "Circle2Radius", "74", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDashGap, "DashGap", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDashHeight, "DashHeight", "4", "proportional_float" );

};

DECLARE_HUDELEMENT( CHudZoom );

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudZoom::CHudZoom( const char *pElementName ) : CHudElement(pElementName), BaseClass(NULL, "HudZoom")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_flPreviousFOV = 360.0f;
	m_flLowestFOV = 360.0f;
	m_bFovIncreasing = false;
}

//-----------------------------------------------------------------------------
// Purpose: standard hud element init function
//-----------------------------------------------------------------------------
void CHudZoom::Init()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets scheme colors
//-----------------------------------------------------------------------------
void CHudZoom::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
	SetFgColor(scheme->GetColor("ZoomReticleColor", GetFgColor()));

	int screenWide, screenTall;
	surface()->GetScreenSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}

//-----------------------------------------------------------------------------
// Purpose: draws the zoom effect
//-----------------------------------------------------------------------------
void CHudZoom::Paint()
{
	// see if we're zoomed any
	C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>(C_BasePlayer::GetLocalPlayer());
	if (!pPlayer)
		return;

	// see if the field of view has changed
	float fov = pPlayer->CurrentFOV() + pPlayer->GetZoom();
	float normalFov = pPlayer->CurrentFOV();
	if (fov >= normalFov)
	{
		m_flPreviousFOV = normalFov;
		return;
	}
	if (fov < 0.1f)
		return;

	// see if we're zooming in or out
	if (m_flPreviousFOV < fov)
	{
		m_bFovIncreasing = true;
	}
	else if (m_flPreviousFOV > fov)
	{
		m_bFovIncreasing = false;
	}
	m_flPreviousFOV = fov;

	// record our lowest fov
	if (fov < m_flLowestFOV)
	{
		m_flLowestFOV = fov;
	}

	// draw the appropriately scaled zoom animation
	float scale = normalFov / fov;
	Color col = GetFgColor();
	if (m_bFovIncreasing)
	{
		// fade out
		col[3] *= ((normalFov - fov) / m_flLowestFOV);
	}
	surface()->DrawSetColor(col);
	
	// draw zoom circles
	int wide, tall;
	GetSize(wide, tall);
	surface()->DrawOutlinedCircle(wide / 2, tall / 2, m_flCircle1Radius * scale, 48);
	surface()->DrawOutlinedCircle(wide / 2, tall / 2, m_flCircle2Radius * scale, 64);

	// draw dashed lines
	int dashCount = 1;
	int ypos = (tall - m_flDashHeight) / 2;
	int xpos = (int)((wide / 2) + (m_flDashGap * ++dashCount * scale));
	while (xpos < wide && xpos > 0)
	{
		surface()->DrawFilledRect(xpos, ypos, xpos + 1, ypos + m_flDashHeight);
		surface()->DrawFilledRect(wide - xpos, ypos, wide - xpos + 1, ypos + m_flDashHeight);
		xpos = (int)((wide / 2) + (m_flDashGap * ++dashCount * scale));
	}

	// draw border darkening
	float alpha = min(0.8f, (1.0f - (1.0f / (normalFov / fov))) * 2);
	if (alpha < 0.01)
		return;
	// vertical
	DrawDarkenedEdge(0, 0, m_flBorderThickness, tall, alpha, 0.0, 0.0, alpha);
	DrawDarkenedEdge(wide - m_flBorderThickness, 0, wide, tall, 0.0, alpha, alpha, 0.0);
	// horizontal
	DrawDarkenedEdge(0, 0, wide, m_flBorderThickness, alpha, alpha, 0.0, 0.0);
	DrawDarkenedEdge(0, tall - m_flBorderThickness, wide, tall, 0.0, 0.0, alpha, alpha);
}

//-----------------------------------------------------------------------------
// Purpose: draws a single darkened screen edge
//-----------------------------------------------------------------------------
void CHudZoom::DrawDarkenedEdge(int x0, int y0, int x1, int y1, float a1, float a2, float a3, float a4)
{
	IMesh *pMesh = materials->GetDynamicMesh( true, NULL, NULL, NULL );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4f( 0.0, 0.0, 0.0, a1 );
	meshBuilder.Position3f( x0,y0,0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4f( 0.0, 0.0, 0.0, a2 );
	meshBuilder.Position3f( x1, y0, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4f( 0.0, 0.0, 0.0, a3 );
	meshBuilder.Position3f( x1, y1, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4f( 0.0, 0.0, 0.0, a4 );
	meshBuilder.Position3f( x0, y1, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}