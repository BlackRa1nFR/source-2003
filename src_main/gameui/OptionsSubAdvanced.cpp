//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "OptionsSubAdvanced.h"

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <KeyValues.h>
#include <vgui_controls/Label.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/TextEntry.h>
#include "CvarToggleCheckButton.h"
#include "ContentControlDialog.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

/*
class CContentControlCheckButton : public vgui::CheckButton
{
	typedef vgui::CheckButton BaseClass;

public:
	CContentControlCheckButton( vgui::Panel *parent, char const *panelName, char const *text );
	~CContentControlCheckButton();

	DECLARE_PANELMAP();

	void OnCheckButtonChecked( int state );
private:

	bool	m_bRequirePassword;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CContentControlCheckButton::CContentControlCheckButton( vgui::Panel *parent, char const *panelName, char const *text ) :
	vgui::CheckButton( parent, panelName, text )
{
	AddActionSignalTarget( this );

	m_bRequirePassword = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CContentControlCheckButton::~CContentControlCheckButton()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CContentControlCheckButton::OnCheckButtonChecked( int state )
{
	if ( m_bRequirePassword )
	{
	}
}

//-----------------------------------------------------------------------------
// Purpose: empty message map
//-----------------------------------------------------------------------------
vgui::MessageMapItem_t CContentControlCheckButton::m_MessageMap[] =
{
	MAP_MESSAGE_INT( CContentControlCheckButton, "CheckButtonChecked", OnCheckButtonChecked, "state" ),	// custom message
};
IMPLEMENT_PANELMAP( CContentControlCheckButton, BaseClass );
*/

// ********************************************************************************

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubAdvanced::COptionsSubAdvanced(vgui::Panel *parent) : PropertyPage(parent, NULL)
{
	m_pContentCheckButton = new vgui::CheckButton( this, "ContentlockButton", "#GameUI_ContentLock" );
	m_pContentCheckButton->SetCommand( "ContentControl" );
//	vgui::Label *l2 = new vgui::Label( this, "ContentlockLabel", "#GameUI_ContentLockLabel" );

	LoadControlSettings("Resource\\OptionsSubAdvanced.res");

    // set up the content control dialog
    m_pContentControlDialog = new CContentControlDialog(this);
    m_pContentControlDialog->AddActionSignalTarget(this);

    int x, y, ww, wt, wide, tall;
    surface()->GetWorkspaceBounds( x, y, ww, wt );
    m_pContentControlDialog->GetSize(wide, tall);

    // Center it, keeping requested size
    m_pContentControlDialog->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));

    m_pContentCheckButton->SetSelected( m_pContentControlDialog->IsPasswordEnabled() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubAdvanced::OnCommand( const char *command )
{
	if ( !stricmp( command, "ContentControl" ) )
	{
        // we're trying to turn it off
        if (!m_pContentCheckButton->IsSelected())
        {
            // if there's no password originally, we can just reset it and move on
            if (!m_pContentControlDialog->IsPasswordEnabled())
            {
                m_pContentControlDialog->ResetPassword();
                return;
            }
        }
        OnOpenContentControlDialog();
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubAdvanced::~COptionsSubAdvanced()
{

//    m_pContentControlDialog->MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubAdvanced::OnResetData()
{
    m_pContentControlDialog->ResetPassword();
    m_pContentCheckButton->SetSelected( m_pContentControlDialog->IsPasswordEnabled() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubAdvanced::OnApplyChanges()
{
    m_pContentControlDialog->ApplyPassword();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubAdvanced::OnContentControlClose()
{
    m_pContentCheckButton->SetSelected(m_pContentControlDialog->IsPasswordEnabledInDialog());

	if (m_pContentControlDialog->IsPasswordEnabled() != m_pContentControlDialog->IsPasswordEnabledInDialog())
    {
        PostActionSignal(new KeyValues("ApplyButtonEnable"));
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubAdvanced::OnOpenContentControlDialog()
{
	m_pContentControlDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: empty message map
//-----------------------------------------------------------------------------
vgui::MessageMapItem_t COptionsSubAdvanced::m_MessageMap[] =
{
//	MAP_MESSAGE( COptionsSubAdvanced, "ControlModified", OnControlModified ),	// custom message
    MAP_MESSAGE( COptionsSubAdvanced, "ContentControlClose", OnContentControlClose ),
};
IMPLEMENT_PANELMAP( COptionsSubAdvanced, BaseClass );
