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


class CHudArmor : public CHudElement, public CHudNumericDisplay
{
public:
	DECLARE_CLASS_SIMPLE( CHudArmor, CHudNumericDisplay );

	CHudArmor( const char *name );

	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void Init();
};


DECLARE_HUDELEMENT( CHudArmor );


CHudArmor::CHudArmor( const char *pName ) :
	CHudNumericDisplay( NULL, pName ), CHudElement( pName )
{
}


void CHudArmor::Init()
{
	SetLabelText(L"Armor");
}


bool CHudArmor::ShouldDraw()
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


void CHudArmor::Paint()
{
	// Update the time.
	C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
	if ( pPlayer )
	{
		SetDisplayValue( (int)pPlayer->ArmorValue() );
		SetShouldDisplayValue( true );
		BaseClass::Paint();
	}
}

