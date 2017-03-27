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
#include "iloadingdisc.h"
#include "vgui_BasePanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Displays the loading plaque
//-----------------------------------------------------------------------------
class CLoadingDiscPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
public:
					CLoadingDiscPanel( vgui::VPANEL parent );
	virtual			~CLoadingDiscPanel( void );

	virtual void	Paint();

private:
	int				m_nTextureID;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CLoadingDiscPanel::CLoadingDiscPanel( vgui::VPANEL parent ) : 
	BaseClass( NULL, "CLoadingDiscPanel" )
{
	SetParent( parent );
	SetVisible( false );
	SetCursor( null );

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );

	m_nTextureID = vgui::surface()->CreateNewTextureID();
	
	//if we have not uploaded yet, lets go ahead and do so
	vgui::surface()->DrawSetTextureFile(m_nTextureID,( const char * )"console/loading",true, true);
	int wide, tall;
	vgui::surface()->DrawGetTextureSize( m_nTextureID, wide, tall );

	SetSize( wide, tall );
	SetPos( ( ScreenWidth() - wide ) / 2, ( ScreenHeight() - tall ) / 2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CLoadingDiscPanel::~CLoadingDiscPanel( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadingDiscPanel::Paint() 
{
	int r,g,b,a;
	r = 255;
	g = 255;
	b = 255;
	a = 255;

	//set the texture current, set the color, and draw the biatch
	vgui::surface()->DrawSetTexture(m_nTextureID);
	vgui::surface()->DrawSetColor(r,g,b,a);

	int wide, tall;
	GetSize( wide, tall );
	vgui::surface()->DrawTexturedRect(0, 0, wide, tall );
}

class CLoadingDisc : public ILoadingDisc
{
private:
	CLoadingDiscPanel *loadingDiscPanel;
public:
	CLoadingDisc( void )
	{
		loadingDiscPanel = NULL;
	}

	void Create( vgui::VPANEL parent )
	{
		loadingDiscPanel = new CLoadingDiscPanel( parent );
	}

	void Destroy( void )
	{
		if ( loadingDiscPanel )
		{
			loadingDiscPanel->SetParent( (vgui::Panel *)NULL );
			delete loadingDiscPanel;
		}
	}

	void SetVisible( bool bVisible )
	{
		loadingDiscPanel->SetVisible( bVisible );
	}

};

static CLoadingDisc g_LoadingDisc;
ILoadingDisc *loadingdisc = ( ILoadingDisc * )&g_LoadingDisc;