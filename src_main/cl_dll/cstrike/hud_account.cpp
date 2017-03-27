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


class CHudAccount : public CHudElement, public CHudNumericDisplay
{
public:
	DECLARE_CLASS_SIMPLE( CHudAccount, CHudNumericDisplay );

	CHudAccount( const char *name );

	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void Init();
};


DECLARE_HUDELEMENT( CHudAccount );


CHudAccount::CHudAccount( const char *pName ) :
	CHudNumericDisplay( NULL, pName ), CHudElement( pName )
{
}


void CHudAccount::Init()
{
	SetLabelText(L"$");
}


bool CHudAccount::ShouldDraw()
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


void CHudAccount::Paint()
{
	// Update the time.
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( pPlayer )
	{
		SetDisplayValue( (int)pPlayer->GetAccount() );
		SetShouldDisplayValue( true );
		BaseClass::Paint();
	}
}

