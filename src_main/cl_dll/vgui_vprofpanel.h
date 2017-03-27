//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUI_VPROFPANEL_H
#define VGUI_VPROFPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/TreeView.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Panel.h>
#include <vgui/IScheme.h>
#include <vgui/MouseCode.h>

//#include "hud.h"
#include "hudelement.h"

class CVProfNode;

//-----------------------------------------------------------------------------

//typedef vgui::TreeView CProfileHierarchyPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CVProfPanel : public CHudElement, public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CVProfPanel, vgui::Frame );
public:

	CVProfPanel( const char *pElementName );
	~CVProfPanel();

	DECLARE_MULTIPLY_INHERITED();

	void UpdateProfile(void);

	// CHudBase overrides
	virtual void Init( void );
	virtual bool ShouldDraw( void );
	
	// Command handlers
	void UserCmd_ShowVProf( void );
	void UserCmd_HideVProf( void );

	void Initialize( void );

	void MouseOverCell(int row, int col);

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void Close();

	virtual void Paint();

	void ExpandAll( void );
	void Reset();
	// InputSignal overrides.
public:
	int				m_fShowVprofHeld;

private:
	void FillTree( KeyValues *pKeyValues, CVProfNode *pNode, int parent );

	class CProfileHierarchyPanel : public vgui::TreeView 
	{
	public:
		CProfileHierarchyPanel(vgui::Panel *parent, const char *panelName): TreeView(parent,panelName) {}
		~CProfileHierarchyPanel() {}

		virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
		{
			//SetProportional( true );

			TreeView::ApplySchemeSettings( pScheme );
			SetFont( pScheme->GetFont( "DefaultVerySmall" ) );
			SetBgColor( Color(0, 0, 0, 175) );
		}
	};

	CProfileHierarchyPanel m_Hierarchy;
};

#endif // VGUI_VPROFPANEL_H
