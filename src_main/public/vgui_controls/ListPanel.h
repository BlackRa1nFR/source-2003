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

#ifndef LISTPANEL_H
#define LISTPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <UtlLinkedList.h>
#include <UtlVector.h>
#include <UtlRBTree.h>
#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

class KeyValues;

namespace vgui
{

class ScrollBar;
class TextImage;
class ImagePanel;
class Label;
class Button;
class IDraggerEvent;

typedef int __cdecl SortFunc( const void *elem1, const void *elem2 );

// column header attributes
#define	NOT_RESIZABLE false  
#define	RESIZABLE true

//-----------------------------------------------------------------------------
// Purpose: Generic class for ListPanel items
//-----------------------------------------------------------------------------
class ListPanelItem
{
public:
	KeyValues		*kv;
	unsigned int 	userData;
};

//-----------------------------------------------------------------------------
// Purpose: A spread-sheet type data view, similar to MFC's 
//-----------------------------------------------------------------------------
class ListPanel : public Panel
{
public:
	ListPanel(Panel *parent, const char *panelName);
	~ListPanel();

	// COLUMN HANDLING
	// all indices are 0 based, limit of 255 columns
	// columns are resizable by default
	virtual void AddColumnHeader(int index, const char *columnName, const char *columnText, 
		int width, bool isTextImage, int minWidth, int maxWidth, bool windowResizable);  // adds a column header
	// If you use default args, use default args for all default args.
	virtual void AddColumnHeader(int index, const char *columnName, const char *columnText, 
		int width, bool isTextImage = true, bool sliderResizable = RESIZABLE, bool windowResizable = NOT_RESIZABLE); // adds a column header by attribute

	virtual void RemoveColumn(int col);	// removes a column
	virtual int  FindColumn(const char *columnName);
	virtual void SetColumnHeaderHeight( int height );
	virtual void SetColumnHeaderText(int col, const char *text);
	virtual void SetColumnHeaderText(int col, wchar_t *text);

	virtual void SetSortFunc(int column, SortFunc *func);
	virtual void SetSortColumn(int column);
	virtual void SortList( void );
	virtual void SetColumnSortable(int column, bool sortable);
	
	// DATA HANDLING
	// data->GetName() is used to uniquely identify an item
	// data sub items are matched against column header name to be used in the table
	virtual int AddItem(KeyValues *data, unsigned int userData, bool bScrollToItem, bool bSortOnAdd); // Takes a copy of the data for use in the table. Returns the index the item is at.
	virtual int	GetItemCount( void );			// returns the number of VISIBLE items
	virtual int GetItem(const char *itemName);	// gets the row index of an item by name (data->GetName())
	virtual KeyValues *GetItem(int itemID); // returns pointer to data the row holds
	virtual int GetItemCurrentRow(int itemID);		// returns -1 if invalid index or item not visible
	virtual int GetItemIDFromRow(int currentRow);			// returns -1 if invalid row
	virtual unsigned int GetItemUserData(int itemID);
	virtual ListPanelItem *GetItemData(int itemID);
	virtual void SetUserData( int itemID, unsigned int userData );
	virtual void ApplyItemChanges(int itemID); // applies any changes to the data, performed by modifying the return of GetItem() above
	virtual void RemoveItem(int itemID); // removes an item from the table (changing the indices of all following items)
	virtual void RereadAllItems(); // updates the view with the new data
	virtual void DeleteAllItems(); // clears and deletes all the memory used by the data items
	virtual void GetCellText(int itemID, int column, wchar_t *buffer, int bufferSize); // returns the data held by a specific cell
	virtual IImage *GetCellImage(int itemID, int column); //, ImagePanel *&buffer); // returns the image held by a specific cell

	virtual int InvalidItemID();
	virtual bool IsValidItemID(int itemID);

	// sets whether the dataitem is visible or not
	// it is removed from the row list when it becomes invisible, but stays in the indexes
	// this is much faster than a normal remove
	virtual void SetItemVisible(int itemID, bool state);

	virtual void SetFont(HFont font);

	// image handling
	virtual void SetImageList(ImageList *imageList, bool deleteImageListWhenDone);

	// SELECTION
	
	// returns the count of selected items
	virtual int GetSelectedItemsCount();

	// returns the selected item by selection index, valid in range [0, GetNumSelectedRows)
	virtual int GetSelectedItem(int selectionIndex);

	// sets no item as selected
	virtual void ClearSelectedItems();

	// adds a item to the select list
	virtual void AddSelectedItem(int itemID);

	// sets this single item as the only selected item
	virtual void SetSingleSelectedItem(int itemID);

	// returns the selected column, -1 for particular column selected
	virtual int GetSelectedColumn();

	// whether or not to select specific cells (off by default)
	virtual void SetSelectIndividualCells(bool state);

	// sets a single cell - all other previous rows are cleared
	virtual void SetSelectedCell(int row, int column);

	virtual bool GetCellAtPos(int x, int y, int &row, int &column);	// returns true if any found, row and column are filled out. x, y are in screen space
	virtual bool GetCellBounds( int row, int column, int& x, int& y, int& wide, int& tall );

	// sets the text which is displayed when the list is empty
	virtual void SetEmptyListText(const char *text);
	virtual void SetEmptyListText(const wchar_t *text);

	// relayout the scroll bar in response to changing the items in the list panel
	// do this if you DeleteAllItems()
	void ResetScrollBar();

protected:
	// PAINTING
	virtual Panel *GetCellRenderer(int row, int column);

	// overrides
	virtual void OnMouseWheeled(int delta);
	virtual void OnSizeChanged(int wide, int tall); 
	virtual void PerformLayout();
	virtual void Paint();
	virtual void PaintBackground();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnMousePressed(enum MouseCode code);
	virtual void OnMouseDoublePressed(enum MouseCode code);
	virtual void OnKeyCodeTyped(enum KeyCode code);
	virtual void OnSliderMoved();
	virtual void OnColumnResized(int column, int delta);
	virtual void OnSetSortColumn(int column);
	virtual float GetRowsPerPage();
	virtual int GetStartItem();


	/* MESSAGES SENT
		"ItemSelected" - query which items are selected
	*/

private:

	// adds the item into the column indexes
	void IndexItem(int itemID);

	// pre-sorted columns
	struct IndexItem_t
	{
		ListPanelItem *dataItem;
		int duplicateIndex;
	};
	typedef CUtlRBTree<IndexItem_t, int> IndexRBTree_t;

	struct column_t
	{
		Button*			m_pHeader;
		int				m_iMinWidth;
		int				m_iMaxWidth;
		int				m_iDesiredWidth;	// in case we get squeezed down by a neighbor
		bool			m_bResizesWithWindow;
		Panel*			m_pResizer;
		SortFunc*		m_pSortFunc;
		bool			m_bTypeIsText;
		IndexRBTree_t	m_SortedTree;		
	};

	// list of the column headers
	CUtlLinkedList<column_t, unsigned char> 		m_ColumnsData;

	// persistent list of all columns ever created, indexes into m_ColumnsData - used for matching up DATAITEM m_SortedTreeIndexes
	CUtlVector<unsigned char>						m_ColumnsHistory;

	// current list of columns, indexes into m_ColumnsData
	CUtlVector<unsigned char>						m_CurrentColumns;

	int				    m_iColumnDraggerMoved; // which column dragger was moved->which header to resize
	int					m_lastBarWidth;

	CUtlLinkedList<ListPanelItem*, int>					m_DataItems;
	CUtlVector<int>										m_VisibleItems;

	// set to true if the table needs to be sorted before it's drawn next
	bool 				m_bNeedsSort;
	int 				m_iSortColumn;
	int 				m_iSortColumnSecondary;
	bool 				m_bSortAscending;
	bool 				m_bSortAscendingSecondary;

	void 				ResortColumnRBTree(int col);
	static bool 		RBTreeLessFunc(vgui::ListPanel::IndexItem_t &item1, vgui::ListPanel::IndexItem_t &item2);

	TextImage			*m_pTextImage; // used in rendering
	ImagePanel			*m_pImagePanel; // used in rendering
	Label				*m_pLabel;	  // used in rendering
	ScrollBar			*m_hbar;
	ScrollBar			*m_vbar;

	bool				m_bCanSelectIndividualCells;

	int					m_iSelectedColumn;
	bool				m_bShiftHeldDown;

	int					m_iHeaderHeight;
	int 				m_iRowHeight;
	
	// selection data
	CUtlVector<int> 	m_SelectedItems;		// array of selected rows
	int					m_LastItemSelected;	// remember the last row selected for future shift clicks


	int 		m_iTableStartX;
	int	 		m_iTableStartY;

	Color 		m_LabelFgColor;
	Color 		m_SelectionFgColor;

	bool 		m_bDeleteImageListWhenDone;
	ImageList 	*m_pImageList;
	TextImage 	*m_pEmptyListText;

	void ResetColumnHeaderCommands();

	DECLARE_PANELMAP();

	typedef Panel BaseClass;
};

}

#endif // LISTPANEL_H
