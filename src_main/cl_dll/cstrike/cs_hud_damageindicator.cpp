//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "text_message.h"
#include "hud_macros.h"
#include "parsemsg.h"
#include "iclientmode.h"
#include "view.h"
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include "vguimatsurface/IMatSystemSurface.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMesh.h"
#include "materialsystem/imaterialvar.h"
#include "ieffects.h"
#include "hudelement.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: HDU Damage indication
//-----------------------------------------------------------------------------
class CHudDamageIndicator : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDamageIndicator, vgui::Panel );

public:
	CHudDamageIndicator( const char *pElementName );
	void Reset( void );
	virtual bool ShouldDraw( void );

	// Handler for our message
	void MsgFunc_Damage( const char *pszName, int iSize, void *pbuf );

private:
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	CPanelAnimationVarAliasType( float, m_flDmgX, "dmg_xpos", "10", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDmgY, "dmg_ypos", "80", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDmgWide, "dmg_wide", "30", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDmgTall1, "dmg_tall1", "300", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDmgTall2, "dmg_tall2", "240", "proportional_float" );

	CPanelAnimationVar( Color, m_DmgColorLeft, "DmgColorLeft", "255 0 0 0" );
	CPanelAnimationVar( Color, m_DmgColorRight, "DmgColorRight", "255 0 0 0" );

	void DrawDamageIndicator(int side);
	void GetDamagePosition( const Vector &vecDelta, float *flRotation );
};

DECLARE_HUDELEMENT( CHudDamageIndicator );
// DECLARE_HUD_MESSAGE( CHudDamageIndicator, Damage );

void HudDamageIndicator_MsgFunc_Damage( const char *pszName, int iSize, void *pbuf )
{
	((CHudDamageIndicator *)GET_HUDELEMENT( CHudDamageIndicator ))->MsgFunc_Damage( pszName, iSize, pbuf );
}


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudDamageIndicator::CHudDamageIndicator( const char *pElementName ) : CHudElement( pElementName ), BaseClass(NULL, "HudDamageIndicator")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDamageIndicator::Reset( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudDamageIndicator::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	if ( !m_DmgColorLeft[3] && !m_DmgColorRight[3] )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Draws a damage quad
//-----------------------------------------------------------------------------
void CHudDamageIndicator::DrawDamageIndicator(int side)
{
	IMaterial *pMat = materials->FindMaterial( "vgui/white_additive", 0, 0 );
	IMesh *pMesh = materials->GetDynamicMesh( true, NULL, NULL, pMat );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	int insetY = (m_flDmgTall1 - m_flDmgTall2) / 2;

	int x1 = m_flDmgX;
	int x2 = m_flDmgX + m_flDmgWide;
	int y[4] = { m_flDmgY, m_flDmgY + insetY, m_flDmgY + m_flDmgTall1 - insetY, m_flDmgY + m_flDmgTall1 };

	if (side == 1)
	{
		int r = m_DmgColorRight[0], g = m_DmgColorRight[1], b = m_DmgColorRight[2], a = m_DmgColorRight[3];

		// realign x coords
		x1 = GetWide() - x1;
		x2 = GetWide() - x2;

		meshBuilder.Color4ub( r, g, b, 0 );
		meshBuilder.TexCoord2f( 0,0,0 );
		meshBuilder.Position3f( x1, y[0], 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ub( r, g, b, 0 );
		meshBuilder.TexCoord2f( 0,0,1 );
		meshBuilder.Position3f( x1, y[3], 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ub( r, g, b, a );
		meshBuilder.TexCoord2f( 0,1,1 );
		meshBuilder.Position3f( x2, y[2], 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ub( r, g, b, a );
		meshBuilder.TexCoord2f( 0,1,0 );
		meshBuilder.Position3f( x2, y[1], 0 );
		meshBuilder.AdvanceVertex();
	}
	else
	{
		int r = m_DmgColorLeft[0], g = m_DmgColorLeft[1], b = m_DmgColorLeft[2], a = m_DmgColorLeft[3];

		meshBuilder.Color4ub( r, g, b, 0 );
		meshBuilder.TexCoord2f( 0,0,0 );
		meshBuilder.Position3f( x1, y[0], 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ub( r, g, b, a );
		meshBuilder.TexCoord2f( 0,1,0 );
		meshBuilder.Position3f( x2, y[1], 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ub( r, g, b, a );
		meshBuilder.TexCoord2f( 0,1,1 );
		meshBuilder.Position3f( x2, y[2], 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ub( r, g, b, 0 );
		meshBuilder.TexCoord2f( 0,0,1 );
		meshBuilder.Position3f( x1, y[3], 0 );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: Paints the damage display
//-----------------------------------------------------------------------------
void CHudDamageIndicator::Paint()
{
	// draw damage indicators	
	DrawDamageIndicator(0);
	DrawDamageIndicator(1);
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for Damage message
//-----------------------------------------------------------------------------
void CHudDamageIndicator::MsgFunc_Damage( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int armor = READ_BYTE();	// armor
	int damageTaken = READ_BYTE();	// health
	long bitsDamage = READ_LONG(); // damage bits
	bitsDamage; // variable still sent but not used

	Vector vecFrom;

	vecFrom.x = READ_FLOAT();
	vecFrom.y = READ_FLOAT();
	vecFrom.z = READ_FLOAT();

	if ( vecFrom == vec3_origin )
	{
		// vecFrom = MainViewOrigin();
		return;
	}

	Vector vecDelta = (vecFrom - MainViewOrigin());
	VectorNormalize( vecDelta );

	if ( damageTaken > 0 || armor > 0 )
	{
		// see which quandrant the effect is in
		float angle;
		GetDamagePosition( vecDelta, &angle );

		if ( angle < 45.0f || angle > 315.0f )
		{
			// front
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudTakeDamageFront" );
		}
		else if ( angle >= 45.0f && angle <= 135.0f )
		{
			// left
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudTakeDamageLeft" );
		}
		else if ( angle <= 315.0f && angle >= 225.0f )
		{
			// right
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudTakeDamageRight" );
		}
		else
		{
			// behind
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudTakeDamageBehind" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Convert a damage position in world units to the screen's units
//-----------------------------------------------------------------------------
void CHudDamageIndicator::GetDamagePosition( const Vector &vecDelta, float *flRotation )
{
	float flRadius = 360.0f;

	// Player Data
	Vector playerPosition = MainViewOrigin();
	QAngle playerAngles = MainViewAngles();

	Vector forward, right, up(0,0,1);
	AngleVectors (playerAngles, &forward, NULL, NULL );
	forward.z = 0;
	VectorNormalize(forward);
	CrossProduct( up, forward, right );
	float front = DotProduct(vecDelta, forward);
	float side = DotProduct(vecDelta, right);
	float xpos = flRadius * -side;
	float ypos = flRadius * -front;

	// Get the rotation (yaw)
	*flRotation = atan2(xpos, ypos) + M_PI;
	*flRotation *= 180 / M_PI;

	float yawRadians = -(*flRotation) * M_PI / 180.0f;
	float ca = cos( yawRadians );
	float sa = sin( yawRadians );
				 
	// Rotate it around the circle
	xpos = (int)((ScreenWidth() / 2) + (flRadius * sa));
	ypos = (int)((ScreenHeight() / 2) - (flRadius * ca));
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudDamageIndicator::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	SetSize(wide, tall);
}
