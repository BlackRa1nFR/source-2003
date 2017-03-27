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

#ifndef PANELLISTPANEL_H
#define PANELLISTPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <UtlLinkedList.h>
#include <UtlVector.h>
#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

class KeyValues;

namespace vgui
{
class ScrollBar;


//-----------------------------------------------------------------------------
// Purpose: A list of variable height child panels
//  each list item consists of a label-panel pair. Height of the item is
// determined from the lable.
//-----------------------------------------------------------------------------
class PanelListPanel : public Panel
{
public:
	PanelListPanel( vgui::Panel *parent, char const *panelName );
	~PanelListPanel();

	// DATA & ROW HANDLING
	// The list now owns the panel
	virtual int	computeVPixelsNeeded( void );
	virtual int AddItem( Panel *labelPanel , Panel *panel);
	virtual int	GetItemCount( void );

	virtual Panel *GetItemLabel(int itemID); 
	virtual Panel *GetItemPanel(int itemID); 

	virtual void RemoveItem(int itemID); // removes an item from the table (changing the indices of all following items)
	virtual void DeleteAllItems(); // clears and deletes all the memory used by the data items
	void RemoveAll();

	// PAINTING
	virtual vgui::Panel *GetCellRenderer( int row );

	// overrides
	virtual void OnSizeChanged(int wide, int tall);
	virtual void OnSliderMoved( int position );

	virtual void MoveScrollBarToTop();

	void SetFirstColumnWidth( int width );
	int GetFirstColumnWidth();

	DECLARE_PANELMAP();

protected:

	virtual void PerformLayout();
	virtual void PaintBackground();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme, KeyValues *inResourceData);

private:
	enum { SCROLLBAR_SIZE = 18, DEFAULT_HEIGHT = 24, PANELBUFFER = 5 };

	typedef struct dataitem_s
	{
		// Always store a panel pointer
		Panel *panel;
		Panel *labelPanel;
	} DATAITEM;

	// list of the column headers

	CUtlLinkedList<DATAITEM*, int>		m_DataItems;
	CUtlVector<int>						m_SortedItems;

	ScrollBar				*m_vbar;
	Panel					*m_pPanelEmbedded;

	int						m_iFirstColumnWidth;
	int						m_iScrollbarSize;
	int						m_iDefaultHeight;
	int						m_iPanelBuffer;

	typedef vgui::Panel BaseClass;
};

}
#endif // PANELLISTPANEL_H
