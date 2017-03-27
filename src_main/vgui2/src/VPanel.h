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

#ifndef VPANEL_H
#define VPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/Dar.h>
#include <vgui/IPanel.h>

namespace vgui
{
// types
typedef unsigned short HPanelList; // MUST be smaller than HPanel, see CVGui::HandleToPanel()
const HPanelList INVALID_PANELLIST = 0xffff;


class SurfaceBase;
class IClientPanel;
struct SerialPanel_t;

//-----------------------------------------------------------------------------
// Purpose: VGUI private implementation of panel
//-----------------------------------------------------------------------------
class VPanel
{
public:
	VPanel();
	virtual ~VPanel();

	virtual void Init(IClientPanel *attachedClientPanel);

	virtual SurfacePlat *Plat();
	virtual void SetPlat(SurfacePlat *pl);

	virtual HPanelList GetListEntry() { return _listEntry; } // safe pointer handling
	virtual void SetListEntry(HPanelList listEntry) { _listEntry = listEntry; }

	virtual bool IsPopup();
	virtual void SetPopup(bool state);
	virtual bool Render_IsPopupPanelVisible();
	virtual void Render_SetPopupVisible(bool state);


	virtual void SetPos(int x, int y);
	virtual void GetPos(int &x, int &y);
	virtual void SetSize(int wide,int tall);
	virtual void GetSize(int& wide,int& tall);
	virtual void SetMinimumSize(int wide,int tall);
	virtual void GetMinimumSize(int& wide,int& tall);
	virtual void SetZPos(int z);
	virtual int  GetZPos();

	virtual void GetAbsPos(int &x, int &y);
	virtual void GetClipRect(int &x0, int &y0, int &x1, int &y1);
	virtual void SetInset(int left, int top, int right, int bottom);
	virtual void GetInset(int &left, int &top, int &right, int &bottom);

	virtual void Solve();

	virtual void SetVisible(bool state);
	virtual bool IsVisible();
	virtual void SetParent(VPanel *newParent);
	virtual int GetChildCount();
	virtual VPanel *GetChild(int index);
	virtual VPanel *GetParent();
	virtual void MoveToFront();
	virtual void MoveToBack();
	virtual bool HasParent(VPanel *potentialParent);

	// gets names of the object (for debugging purposes)
	virtual const char *GetName();
	virtual const char *GetClassName();

	virtual HScheme GetScheme();

	// handles a message
	virtual void SendMessage(KeyValues *params, VPANEL ifromPanel);

	// wrapper to get Client panel interface
	virtual IClientPanel *Client() { return _clientPanel; }

	// input interest
	virtual void SetKeyBoardInputEnabled(bool state);
	virtual void SetMouseInputEnabled(bool state);
	virtual bool IsKeyBoardInputEnabled();
	virtual bool IsMouseInputEnabled();

private:
	Dar<VPanel*> _childDar;
	VPanel *_parent;
	SurfacePlat	*_plat;	// platform-specific data
	HPanelList _listEntry;

	// our companion Client panel
	IClientPanel *_clientPanel;

	short _pos[2];
	short _size[2];
	short _minimumSize[2];

	short _inset[4];
	short _clipRect[4];
	short _absPos[2];

	short _zpos;	// z-order position
	
	bool _visible;
	bool _popup;
	bool _popupVisible;

	bool _mouseInput; // used for popups
	bool _kbInput;
};

}


#endif // VPANEL_H
