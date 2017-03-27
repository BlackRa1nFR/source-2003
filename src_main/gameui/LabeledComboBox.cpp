#include "LabeledComboBox.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLabeledComboBox::CLabeledComboBox( vgui::Panel *parent, const char *panelName, const char *label )
 : vgui::Panel( parent, panelName )
{
	m_pComboBox = new vgui::ComboBox( this, "LabeledComboBox", 5, false );

	m_pLabel = new vgui::Label( this, "LabeledComboBoxLabel", label );
	m_pLabel->SetContentAlignment( vgui::Label::a_west );

	SetSize( 100, 100 );
	SetPaintBackgroundEnabled( false );

	m_pComboBox->AddActionSignalTarget( this );
}

CLabeledComboBox::~CLabeledComboBox( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: default message handler
//-----------------------------------------------------------------------------
void CLabeledComboBox::OnSizeChanged(int newWide, int newTall)
{
	m_pLabel->SetBounds( 2, 2, newWide-4, 15 );
	m_pComboBox->SetBounds( 2, 20, newWide-4, 18 );
}

void CLabeledComboBox::setLabel( char const *text )
{
	m_pLabel->SetText( text );
}

void CLabeledComboBox::RemoveAllItems()
{
	m_pComboBox->RemoveAllItems();
}

void CLabeledComboBox::ActivateItem(int itemIndex)
{
	m_pComboBox->ActivateItem( itemIndex );
}

void CLabeledComboBox::AddItem( char const *text )
{
	m_pComboBox->AddItem( text );
}

void CLabeledComboBox::OnTextChanged( char const *text )
{
	PostMessage( GetParent()->GetVPanel(), new KeyValues( "TextChanged", "text", text ) );
}

void CLabeledComboBox::GetText( char *buffer, int len )
{
	m_pComboBox->GetText( buffer, len );
}

//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
vgui::MessageMapItem_t CLabeledComboBox::m_MessageMap[] =
{
	MAP_MESSAGE_CONSTCHARPTR( CLabeledComboBox, "TextChanged", OnTextChanged, "text" ),	// custom message
};

IMPLEMENT_PANELMAP( CLabeledComboBox, BaseClass );