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

#ifndef TREEVIEW_H
#define TREEVIEW_H

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
class TextImage;
class ImagePanel;
class Label;
class Button;
class IDraggerEvent;

class ExpandButton;
class TreeNode;
class TreeViewSubPanel;

// sorting function, should return true if node1 should be displayed before node2
typedef bool (*TreeViewSortFunc_t)(KeyValues *node1, KeyValues *node2);

class TreeView : public Panel
{
public:
    TreeView(Panel *parent, const char *panelName);
    ~TreeView();

    void SetSortFunc(TreeViewSortFunc_t pSortFunc);

    virtual int AddItem(KeyValues *data, int parentItemIndex);

    virtual int GetItemCount(void);
    virtual KeyValues *GetItemData(int itemIndex);
    virtual KeyValues *GetSelectedItemData();
    virtual void RemoveItem(int itemIndex, bool bPromoteChildren);
    virtual void RemoveAll();
    virtual bool ModifyItem(int itemIndex, KeyValues *data);

    virtual void SetFont(HFont font);

    virtual void SetImageList(ImageList *imageList, bool deleteImageListWhenDone);

    virtual void SetSelectedItem(int itemIndex);

	// set colors for individual elments
	virtual void SetItemFgColor(int itemIndex, Color color);
	virtual void SetItemBgColor(int itemIndex, Color color);

	// returns the id of the currently selected item, -1 if nothing is selected
	virtual int GetSelectedItem();

	// returns true if the itemID is valid for use
	virtual bool IsItemIDValid(int itemIndex);

	// item iterators
	// iterate from [0..GetHighestItemID()], 
	// and check each with IsItemIDValid() before using
	virtual int GetHighestItemID();

	// gets the local coordinates of a cell
//	virtual bool GetCellBounds(int itemID, int column, int &x, int &y, int &wide, int &tall);

    virtual void ExpandItem(int itemIndex, bool bExpand);

    virtual void MakeItemVisible(int itemIndex);

	// set up a field for editing
//	virtual void EnterEditMode(int itemID, int column, vgui::Panel *editPanel);

	// leaves editing mode
//	virtual void LeaveEditMode();

	// returns true if we are currently in inline editing mode
//	virtual bool IsInEditMode();

	virtual HFont GetFont();


protected:

	// overrides
	virtual void OnMouseWheeled(int delta);
	virtual void OnSizeChanged(int wide, int tall); 
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnSliderMoved();
	virtual void SetBgColor( Color color );

//	virtual void OnMousePressed(enum MouseCode code);
//	virtual void OnKeyCodeTyped(enum KeyCode code);

private:
	ScrollBar			*_hbar, *_vbar;
	int                 _rowHeight;

	bool m_bDeleteImageListWhenDone;
	ImageList *m_pImageList;


    friend TreeNode;
    IImage* GetImage(int index);        // to be accessed by TreeNodes
    int GetRowHeight();

    TreeNode                *m_pRootNode;
    TreeViewSortFunc_t      m_pSortFunc;
    HFont                   m_Font;

    // cross reference - no hierarchy ordering in this list
    CUtlLinkedList<TreeNode *, int>   m_NodeList;
    
    TreeNode                *m_pSelectedItem;
    TreeViewSubPanel        *m_pSubPanel;

    DECLARE_PANELMAP();

    typedef vgui::Panel BaseClass;
};

}

#endif // TREEVIEW_H
