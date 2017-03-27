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

#include "VPanel.h"
#include "vgui_internal.h"

#include <vgui/IClientPanel.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Protects internal VPanel through the versionable interface IPanel
//-----------------------------------------------------------------------------
class VPanelWrapper : public vgui::IPanel
{
public:
	virtual void Init(VPANEL vguiPanel, IClientPanel *panel)
	{
		((VPanel *)vguiPanel)->Init(panel);
	}

	// returns a pointer to the Client panel
	virtual IClientPanel *Client(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->Client();
	}

	// methods
	virtual void SetPos(VPANEL vguiPanel, int x, int y)
	{
		((VPanel *)vguiPanel)->SetPos(x, y);
	}

	virtual void GetPos(VPANEL vguiPanel, int &x, int &y)
	{
		((VPanel *)vguiPanel)->GetPos(x, y);
	}

	virtual void SetSize(VPANEL vguiPanel, int wide,int tall)
	{
		((VPanel *)vguiPanel)->SetSize(wide, tall);
	}

	virtual void GetSize(VPANEL vguiPanel, int &wide, int &tall)
	{
		((VPanel *)vguiPanel)->GetSize(wide, tall);
	}

	virtual void SetMinimumSize(VPANEL vguiPanel, int wide, int tall)
	{
		((VPanel *)vguiPanel)->SetMinimumSize(wide, tall);
	}

	virtual void GetMinimumSize(VPANEL vguiPanel, int &wide, int &tall)
	{
		((VPanel *)vguiPanel)->GetMinimumSize(wide, tall);
	}

	virtual void SetZPos(VPANEL vguiPanel, int z)
	{
		((VPanel *)vguiPanel)->SetZPos(z);
	}

	virtual int GetZPos(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->GetZPos();
	}

	virtual void GetAbsPos(VPANEL vguiPanel, int &x, int &y)
	{
		((VPanel *)vguiPanel)->GetAbsPos(x, y);
	}

	virtual void GetClipRect(VPANEL vguiPanel, int &x0, int &y0, int &x1, int &y1)
	{
		((VPanel *)vguiPanel)->GetClipRect(x0, y0, x1, y1);
	}

	virtual void SetInset(VPANEL vguiPanel, int left, int top, int right, int bottom)
	{
		((VPanel *)vguiPanel)->SetInset(left, top, right, bottom);
	}

	virtual void GetInset(VPANEL vguiPanel, int &left, int &top, int &right, int &bottom)
	{
		((VPanel *)vguiPanel)->GetInset(left, top, right, bottom);
	}

	virtual void SetVisible(VPANEL vguiPanel, bool state)
	{
		((VPanel *)vguiPanel)->SetVisible(state);
	}

	virtual bool IsVisible(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->IsVisible();
	}

	virtual void SetParent(VPANEL vguiPanel, VPANEL newParent)
	{
		((VPanel *)vguiPanel)->SetParent((VPanel *)newParent);
	}

	virtual int GetChildCount(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->GetChildCount();
	}

	virtual VPANEL GetChild(VPANEL vguiPanel, int index)
	{
		return (VPANEL)((VPanel *)vguiPanel)->GetChild(index);
	}

	virtual VPANEL GetParent(VPANEL vguiPanel)
	{
		return (VPANEL)((VPanel *)vguiPanel)->GetParent();
	}

	virtual void MoveToFront(VPANEL vguiPanel)
	{
		((VPanel *)vguiPanel)->MoveToFront();
	}

	virtual void MoveToBack(VPANEL vguiPanel)
	{
		((VPanel *)vguiPanel)->MoveToBack();
	}

	virtual bool HasParent(VPANEL vguiPanel, VPANEL potentialParent)
	{
		if (!vguiPanel)
			return false;

		return ((VPanel *)vguiPanel)->HasParent((VPanel *)potentialParent);
	}

	virtual bool IsPopup(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->IsPopup();
	}

	virtual void SetPopup(VPANEL vguiPanel, bool state)
	{
		((VPanel *)vguiPanel)->SetPopup(state);
	}

	virtual bool Render_GetPopupVisible(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->Render_IsPopupPanelVisible();
	}

	virtual void Render_SetPopupVisible(VPANEL vguiPanel, bool state)
	{
		((VPanel *)vguiPanel)->Render_SetPopupVisible(state);
	}

	// calculates the panels current position within the hierarchy
	virtual void Solve(VPANEL vguiPanel)
	{
		((VPanel *)vguiPanel)->Solve();
	}

	// used by ISurface to store platform-specific data
	virtual SurfacePlat *Plat(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->Plat();
	}

	virtual void SetPlat(VPANEL vguiPanel, SurfacePlat *Plat)
	{
		((VPanel *)vguiPanel)->SetPlat(Plat);
	}

	virtual const char *GetName(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->GetName();
	}

	virtual const char *GetClassName(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->GetClassName();
	}

	virtual HScheme GetScheme(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->GetScheme();
	}

	virtual void SendMessage(VPANEL vguiPanel, KeyValues *params, VPANEL ifrompanel)
	{
		((VPanel *)vguiPanel)->SendMessage(params, ifrompanel);
	}

	virtual void Think(VPANEL vguiPanel)
	{
		((VPanel *)vguiPanel)->Client()->Think();
	}

	virtual void PerformApplySchemeSettings(VPANEL vguiPanel)
	{
		((VPanel *)vguiPanel)->Client()->PerformApplySchemeSettings();
	}

	virtual void PaintTraverse(VPANEL vguiPanel, bool forceRepaint, bool allowForce)
	{
		((VPanel *)vguiPanel)->Client()->PaintTraverse(forceRepaint, allowForce);
	}

	virtual void Repaint(VPANEL vguiPanel)
	{
		((VPanel *)vguiPanel)->Client()->Repaint();
	}

	virtual VPANEL IsWithinTraverse(VPANEL vguiPanel, int x, int y, bool traversePopups)
	{
		return ((VPanel *)vguiPanel)->Client()->IsWithinTraverse(x, y, traversePopups);
	}

	virtual void OnChildAdded(VPANEL vguiPanel, VPANEL child)
	{
		((VPanel *)vguiPanel)->Client()->OnChildAdded(child);
	}

	virtual void OnSizeChanged(VPANEL vguiPanel, int newWide, int newTall)
	{
		((VPanel *)vguiPanel)->Client()->OnSizeChanged(newWide, newTall);
	}

	virtual void InternalFocusChanged(VPANEL vguiPanel, bool lost)
	{
		((VPanel *)vguiPanel)->Client()->InternalFocusChanged(lost);
	}

	virtual bool RequestInfo(VPANEL vguiPanel, KeyValues *outputData)
	{
		return ((VPanel *)vguiPanel)->Client()->RequestInfo(outputData);
	}

	virtual void RequestFocus(VPANEL vguiPanel, int direction = 0)
	{
		((VPanel *)vguiPanel)->Client()->RequestFocus();
	}

	virtual VPANEL GetCurrentKeyFocus(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->Client()->GetCurrentKeyFocus();
	}

	virtual Panel *GetPanel(VPANEL vguiPanel, const char *moduleName)
	{
		if (vguiPanel == surface()->GetEmbeddedPanel())
			return NULL;

/*#ifdef _DEBUG
		// assert that the specified vpanel is from the same module as requesting the cast
		if (stricmp(GetModuleName(vguiPanel), moduleName))
		{
			// FIXME!!!!
			// DON'T CHECK ME IN!!!
			//assert(!("GetPanel() used to retrieve a Panel * from a different dll than which which it was created. This is bad, you can't pass Panel * across dll boundaries else you'll break the versioning.  Please only use a VPANEL."));
		}
#endif*/
		return ((VPanel *)vguiPanel)->Client()->GetPanel();
	}

	virtual const char *GetModuleName(VPANEL vguiPanel)
	{
		return ((VPanel *)vguiPanel)->Client()->GetModuleName();
	}


	virtual void SetKeyBoardInputEnabled( VPANEL vguiPanel, bool state ) 
	{
		((VPanel *)vguiPanel)->SetKeyBoardInputEnabled(state);
	}

	virtual void SetMouseInputEnabled( VPANEL vguiPanel, bool state ) 
	{
		((VPanel *)vguiPanel)->SetMouseInputEnabled(state);
	}

	virtual bool IsMouseInputEnabled( VPANEL vguiPanel ) 
	{
		return ((VPanel *)vguiPanel)->IsMouseInputEnabled();
	}

	virtual bool IsKeyBoardInputEnabled( VPANEL vguiPanel ) 
	{
		return ((VPanel *)vguiPanel)->IsKeyBoardInputEnabled();
	}



};

EXPOSE_SINGLE_INTERFACE(VPanelWrapper, IPanel, VGUI_PANEL_INTERFACE_VERSION);

