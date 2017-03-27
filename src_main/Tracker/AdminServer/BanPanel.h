//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BANPANEL_H
#define BANPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>
#include <VGUI_PHandle.h>
#include <VGUI_ListPanel.h>
#include <VGUI_KeyValues.h>
#include <VGUI_PropertyPage.h>

#include "BanContextMenu.h"

class KeyValues;

namespace vgui
{
class Button;
class ToggleButton;
class RadioButton;
class Label;
class TextEntry;
class ListPanel;
class MessageBox;
class ComboBox;
class Panel;
class PropertySheet;
};


//-----------------------------------------------------------------------------
// Purpose: Dialog for displaying information about a game server
//-----------------------------------------------------------------------------
class CBanPanel: public vgui::PropertyPage
{
public:
	CBanPanel(vgui::Panel *parent, const char *name);
	~CBanPanel();


	// ListPanel function passthroughs
	virtual void AddColumnHeader(int index, const char *columnName, const char *columnText, 
			int width, bool isTextImage = true, bool sliderResizable = RESIZABLE, bool windowResizable = NOT_RESIZABLE);
	virtual void SetSortFunc(int column, vgui::SortFunc *func);
	virtual void SetSortColumn(int column); 
		// returns the count of selected rows
	virtual int GetNumSelectedRows();
	// returns the selected row by selection index, valid in range [0, GetNumSelectedRows)
	virtual int GetSelectedRow(int selectionIndex);
	virtual void DeleteAllItems(); // clears and deletes all the memory used by the data items

	virtual int AddItem(vgui::KeyValues *data, unsigned int userData = 0 ); // Takes a copy of the data for use in the table. Returns the index the item is at.
	virtual void SortList( void );
	virtual vgui::ListPanel::DATAITEM *GetDataItem( int itemIndex );
	virtual vgui::KeyValues *GetItem(int itemIndex); // returns pointer to data the row holds


	//property page handlers
	virtual void OnPageShow();
	virtual void OnPageHide();


	// vgui overrides
	virtual void PerformLayout();
	virtual void OnCommand(const char *command);

private:

	// msg handlers
	void OnOpenContextMenu(int row);
	void OnEffectPlayer(vgui::KeyValues *data);

	// an inner class
	class BanListPanel: public vgui::ListPanel
	{
	public:
		BanListPanel(vgui::Panel *parent, const char *panelName): vgui::ListPanel(parent,panelName) { m_pParent=parent;};
		
		virtual void	OnMouseDoublePressed( vgui::MouseCode code );
	private:
		vgui::Panel *m_pParent;

	};


	void OnTextChanged(vgui::Panel *panel, const char *text);

	BanListPanel *m_pBanListPanel;
	vgui::Button *m_pAddButton;
	vgui::Button *m_pRemoveButton;
	vgui::Button *m_pChangeButton;

	CBanContextMenu *m_pBanContextMenu;

	vgui::Panel *m_pParent;
		
	DECLARE_PANELMAP();
	typedef vgui::PropertyPage BaseClass;
};

#endif // BANPANEL_H