//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MULTIPLAYERCUSTOMIZEDIALOG_H
#define MULTIPLAYERCUSTOMIZEDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>

class CLabeledCommandComboBox;
class CBitmapImagePanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMultiplayerCustomizeDialog : public vgui::Frame
{
public:
	CMultiplayerCustomizeDialog( vgui::Panel *menuTarget );
	~CMultiplayerCustomizeDialog();

	virtual void OnCommand( const char *command );
	virtual void OnClose();

	virtual void	OnTick();

	virtual void	OnApplyChanges();

	DECLARE_PANELMAP();

protected:
	typedef vgui::Frame BaseClass;

private:
	void					InitModelList( CLabeledCommandComboBox *cb );
	void					InitLogoList( CLabeledCommandComboBox *cb );

	void					RemapModel();
	void					RemapLogo();

	void					OnTextChanged();

	void					RemapPalette( char *filename, int topcolor, int bottomcolor );
	void					RemapLogoPalette( char *filename, int r, int g, int b );

	void					ColorForName( char const *pszColorName, int&r, int&g, int&b );

	CBitmapImagePanel		*m_pModelImage;
	CLabeledCommandComboBox	*m_pModelList;

	CBitmapImagePanel		*m_pLogoImage;
	CLabeledCommandComboBox	*m_pLogoList;

	CLabeledCommandComboBox	*m_pColorList;

	int						m_nTopColor;
	int						m_nBottomColor;

	int						m_nLogoR;
	int						m_nLogoG;
	int						m_nLogoB;

	// For opening advanced dialog as a top level dialog
	vgui::Panel				*m_pMenuTarget;
};


#endif // MULTIPLAYERCUSTOMIZEDIALOG_H
