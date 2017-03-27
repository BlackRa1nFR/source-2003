//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <assert.h>

#include <vgui/MouseCode.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/PanelListPanel.h>

#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
PanelListPanel::PanelListPanel( vgui::Panel *parent, char const *panelName ) : Panel( parent, panelName )
{
	SetBounds( 0, 0, 100, 100 );

	m_vbar = new ScrollBar(this, "PanelListPanelVScroll", true);
	m_vbar->SetBounds( 0, 0, 20, 20 );
	m_vbar->SetVisible(false);
	m_vbar->AddActionSignalTarget( this );

	m_pPanelEmbedded = new EditablePanel(this, "PanelListEmbedded");
	m_pPanelEmbedded->SetBounds(0, 0, 20, 20);
	m_pPanelEmbedded->SetPaintBackgroundEnabled( false );
	m_pPanelEmbedded->SetPaintBorderEnabled(false);

	m_iFirstColumnWidth = 100; // default width

	if( IsProportional() )
	{
		int width, height;
		int sw,sh;
		surface()->GetProportionalBase( width, height );
		surface()->GetScreenSize(sw, sh);
		
		m_iScrollbarSize = static_cast<int>( static_cast<float>( SCROLLBAR_SIZE )*( static_cast<float>( sw )/ static_cast<float>( width )));
		m_iDefaultHeight = static_cast<int>( static_cast<float>( DEFAULT_HEIGHT )*( static_cast<float>( sw )/ static_cast<float>( width )));
		m_iPanelBuffer = static_cast<int>( static_cast<float>( PANELBUFFER )*( static_cast<float>( sw )/ static_cast<float>( width )));
	}
	else
	{
		m_iScrollbarSize = SCROLLBAR_SIZE;
		m_iDefaultHeight = DEFAULT_HEIGHT;
		m_iPanelBuffer = PANELBUFFER;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
PanelListPanel::~PanelListPanel()
{
	// free data from table
	DeleteAllItems();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	PanelListPanel::computeVPixelsNeeded( void )
{
	int pixels = 0;

	int i; 
	for ( i = 0; i < m_SortedItems.Count(); i++ )
	{
		Panel *panel = m_DataItems[ m_SortedItems[i] ]->panel;
		if ( !panel )
			continue;

		int w, h;
		panel->GetSize( w, h );

		pixels += m_iPanelBuffer; // add in buffer. between items.
		pixels += h;	
	}

	pixels += m_iPanelBuffer; // add in buffer below last item

	return pixels;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the panel to use to render a cell
//-----------------------------------------------------------------------------
Panel *PanelListPanel::GetCellRenderer( int row )
{
	if ( !m_SortedItems.IsValidIndex(row) )
		return NULL;

	Panel *panel = m_DataItems[ m_SortedItems[row] ]->panel;
	return panel;
}

//-----------------------------------------------------------------------------
// Purpose: adds an item to the view
//			data->GetName() is used to uniquely identify an item
//			data sub items are matched against column header name to be used in the table
//-----------------------------------------------------------------------------
int PanelListPanel::AddItem( Panel *labelPanel, Panel *panel)
{
	assert(labelPanel && panel);

	DATAITEM *newitem = new DATAITEM;
	newitem->labelPanel = labelPanel;
	newitem->panel = panel;

	labelPanel->SetParent( m_pPanelEmbedded );
	panel->SetParent( m_pPanelEmbedded );

	int itemID = m_DataItems.AddToTail(newitem);
	m_SortedItems.AddToTail(itemID);

	InvalidateLayout();
	return itemID;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
int	PanelListPanel::GetItemCount( void )
{
	return m_DataItems.Count();
}

//-----------------------------------------------------------------------------
// Purpose: returns label panel for this itemID
//-----------------------------------------------------------------------------
Panel *PanelListPanel::GetItemLabel(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;

	return m_DataItems[itemID]->labelPanel;
}

//-----------------------------------------------------------------------------
// Purpose: returns label panel for this itemID
//-----------------------------------------------------------------------------
Panel *PanelListPanel::GetItemPanel(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;

	return m_DataItems[itemID]->panel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelListPanel::RemoveItem(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;

	DATAITEM *item = m_DataItems[itemID];
	if ( item->panel )
	{
		item->panel->MarkForDeletion();
	}
	if ( item->labelPanel )
	{
		item->labelPanel->MarkForDeletion();
	}
	delete item;

	m_DataItems.Remove(itemID);
	m_SortedItems.FindAndRemove(itemID);

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: clears and deletes all the memory used by the data items
//-----------------------------------------------------------------------------
void PanelListPanel::DeleteAllItems()
{
	FOR_EACH_LL( m_DataItems, i )
	{
		if ( m_DataItems[i] )
		{
			if ( m_DataItems[i]->panel )
			{
				delete m_DataItems[i]->panel;
			}

			delete m_DataItems[i];
		}
	}

	m_DataItems.RemoveAll();
	m_SortedItems.RemoveAll();

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: clears and deletes all the memory used by the data items
//-----------------------------------------------------------------------------
void PanelListPanel::RemoveAll()
{
	m_DataItems.RemoveAll();
	m_SortedItems.RemoveAll();

	// move the scrollbar to the top of the list
	m_vbar->SetValue(0);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelListPanel::OnSizeChanged(int wide, int tall)
{
	BaseClass::OnSizeChanged(wide, tall);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: relayouts out the panel after any internal changes
//-----------------------------------------------------------------------------
void PanelListPanel::PerformLayout()
{
	int wide, tall;
	GetSize( wide, tall );

	int vpixels = computeVPixelsNeeded();

	//!! need to make it recalculate scroll positions
	m_vbar->SetVisible(true);
	m_vbar->SetEnabled(false);
	m_vbar->SetRange( 0, vpixels - tall + m_iDefaultHeight  );
	m_vbar->SetRangeWindow( m_iDefaultHeight );
	m_vbar->SetButtonPressedScrollValue( m_iDefaultHeight ); // standard height of labels/buttons etc.
	m_vbar->SetPos(wide - m_iScrollbarSize, 1);
	m_vbar->SetSize(m_iScrollbarSize, tall - 2);

	int top = m_vbar->GetValue();

	m_pPanelEmbedded->SetPos( 1, -top );
	m_pPanelEmbedded->SetSize( wide-m_iScrollbarSize -2, vpixels );

	int sliderPos = m_vbar->GetValue();
	// Now lay out the controls on the embedded panel
	int y = 0;
	int h = 0;
	int totalh = 0;
	
	int i;
	for ( i = 0; i < m_SortedItems.Count(); i++, y += h )
	{
		// add in a little buffer between panels
		y += m_iPanelBuffer;
		DATAITEM *item = m_DataItems[ m_SortedItems[i] ];
		assert(item);

		h = item->panel->GetTall();

		totalh += h;
		if (totalh >= sliderPos)
		{
			item->labelPanel->SetBounds( 8, y, m_iFirstColumnWidth, h );
			item->panel->SetBounds( 8 + m_iFirstColumnWidth + m_iPanelBuffer, y, wide - m_iFirstColumnWidth - m_iScrollbarSize - 8 - 2, h );	
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelListPanel::PaintBackground()
{
	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PanelListPanel::ApplySchemeSettings(IScheme *pScheme, KeyValues *inResourceData)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
	SetBgColor(GetSchemeColor("LabelBgColor", GetSchemeColor("WindowDisabledBgColor", pScheme), pScheme));

//	SetBorder(pScheme->GetBorder("DepressedButtonBorder"));

//	_labelFgColor = GetSchemeColor("WindowFgColor");
//	_selectionFgColor = GetSchemeColor("ListSelectionFgColor", _labelFgColor);
}

void PanelListPanel::OnSliderMoved( int position )
{
	InvalidateLayout();
	Repaint();
}

void PanelListPanel::MoveScrollBarToTop()
{
	m_vbar->SetValue(0);
}

void PanelListPanel::SetFirstColumnWidth( int width )
{
	m_iFirstColumnWidth = width;
}

int PanelListPanel::GetFirstColumnWidth()
{
	return m_iFirstColumnWidth;
}

//-----------------------------------------------------------------------------
// Purpose: Message mapping 
//-----------------------------------------------------------------------------
MessageMapItem_t PanelListPanel::m_MessageMap[] =
{
	MAP_MESSAGE_INT( PanelListPanel, "ScrollBarSliderMoved", OnSliderMoved, "position" ),		// custom message
};

IMPLEMENT_PANELMAP(PanelListPanel, BaseClass);
