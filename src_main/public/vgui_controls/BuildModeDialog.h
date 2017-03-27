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

#ifndef BUILDMODEDIALOG_H
#define BUILDMODEDIALOG_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>

struct PanelItem_t;

namespace vgui
{

class ComboBox;
class Label;
class Menu;
class Divider;

//-----------------------------------------------------------------------------
// Purpose: Dialog for use in build mode editing
//-----------------------------------------------------------------------------
class BuildModeDialog : public Frame
{
public:
	BuildModeDialog( BuildGroup *buildGroup );
	~BuildModeDialog();
	
	// Set the current control to edit
	virtual void SetActiveControl( Panel *controlToEdit );

	// Update the current control with the current resource settings.
	virtual void UpdateControlData( Panel *control );
	
	// Handle build mode dialog button commands.
	virtual void OnCommand( const char *command );

	// Store the current settings of the current panel 
	virtual void StoreUndoSettings();

	// Store the current settings of all panels in the build group.
	virtual KeyValues *StoreSettings();

	DECLARE_PANELMAP();
	/* CUSTOM MESSAGE HANDLING
		"SetActiveControl"
			input:	"PanelPtr"	- panel to set active control to edit to
	*/	

protected:
	virtual void PerformLayout();
	virtual void OnClose();

private:
	void CreateControls();
	
	void OnKeyCodeTyped(KeyCode code);
	bool ApplyDataToControls();
	void OnTextChanged();
	void DeletePanel();
	void ExitBuildMode();
	Panel *OnNewControl(const char *name, int x = 0, int y = 0);
	void DoUndo();
	void DoCopy();
	void DoPaste();
	void EnableSaveButton();
	void RevertToSaved();
	void ShowHelp();
	void ShutdownBuildMode();
	void OnPanelMoved();

	Panel *m_pCurrentPanel;
	BuildGroup *m_pBuildGroup;
	Label *m_pStatusLabel;
	Divider *m_pDivider;

	class PanelList;
	PanelList *m_pPanelList;

	Button *m_pSaveButton;
	Button *m_pApplyButton;
	Button *m_pExitButton;
	Button *m_pDeleteButton;
	Button *m_pOptionsButton;

	bool _autoUpdate;

	ComboBox *m_pAddNewControlCombo; // combo box for adding new controls
	KeyValues *_undoSettings; // settings for the Undo command
	KeyValues *_copySettings; // settings for the Copy/Paste command
	char _copyClassName[255];

	void RemoveAllControls( void );
	void UpdateEditControl(PanelItem_t &panelItem, const char *datstring);

	enum {
		TYPE_STRING,
		TYPE_INTEGER,
		TYPE_COLOR,
		TYPE_ALIGNMENT,
		TYPE_AUTORESIZE,
		TYPE_CORNER,
		TYPE_LOCALIZEDSTRING,
	};

	typedef Frame BaseClass;
	friend class PanelList;
};

} // namespace vgui


#endif // BUILDMODEDIALOG_H

