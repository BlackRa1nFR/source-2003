//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/isurface.h>
#include "clientmode_csnormal.h"
#include "cs_gamerules.h"
#include "hud_numericdisplay.h"


class CHudRoundTimer : public CHudElement, public CHudNumericDisplay
{
public:
	DECLARE_CLASS_SIMPLE( CHudRoundTimer, CHudNumericDisplay );

	CHudRoundTimer( const char *name );

	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void Init();
};


DECLARE_HUDELEMENT( CHudRoundTimer );


CHudRoundTimer::CHudRoundTimer( const char *pName ) :
	CHudNumericDisplay( NULL, pName ), CHudElement( pName )
{
}


void CHudRoundTimer::Init()
{
	SetLabelText(L"TIMER");
}


bool CHudRoundTimer::ShouldDraw()
{
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( pPlayer )
	{
		return !pPlayer->IsObserver();
	}
	else
	{
		return false;
	}
}


void CHudRoundTimer::Paint()
{
	// Update the time.
	C_CSGameRules *pRules = CSGameRules();
	if ( pRules )
	{
		SetDisplayValue( (int)ceil( CSGameRules()->TimeRemaining() ) );
		SetShouldDisplayValue( true );
		BaseClass::Paint();
	}
}

