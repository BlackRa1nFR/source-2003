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

#ifndef PROPERTYPAGE_H
#define PROPERTYPAGE_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/PHandle.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Property page, as held by a set of property sheets
//-----------------------------------------------------------------------------
class PropertyPage : public EditablePanel
{
public:
	PropertyPage(Panel *parent, const char *panelName, bool paintBorder = true);
	~PropertyPage();

	// Called when page is loaded.  Data should be reloaded from document into controls.
	virtual void OnResetData();

	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	virtual void OnApplyChanges();

	// called when the page is shown/hidden
	virtual void OnPageShow();
	virtual void OnPageHide();

	virtual void SetPaintBorder(bool state) { _paintRaised=state; };
	virtual void OnKeyCodeTyped(KeyCode code);

	DECLARE_PANELMAP();

protected:
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void PaintBorder();
	virtual void SetVisible(bool state);

	// called to be notified of the tab button used to Activate this page
	// if overridden this must be chained back to
	virtual void OnPageTabActivated(Panel *pageTab);

private:
	PHandle _pageTab;
	bool _paintRaised;

	typedef EditablePanel BaseClass;
};

} // namespace vgui

#endif // PROPERTYPAGE_H
