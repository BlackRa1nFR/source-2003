//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef LABELEDCOMBOBOX_H
#define LABELEDCOMBOBOX_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Panel.h>
#include "UtlVector.h"

class CLabeledComboBox : public vgui::Panel
{
public:
	CLabeledComboBox( vgui::Panel *parent, const char *panelName, const char *label );
	~CLabeledComboBox( void );

	virtual void	setLabel( char const *text );
	virtual void	GetText( char *buffer, int len );
	virtual void	RemoveAllItems();
	virtual void	AddItem( char const *text );
	virtual void	ActivateItem(int itemIndex);

	virtual void	OnSizeChanged(int newWide, int newTall);

	DECLARE_PANELMAP();
	
private:
	typedef vgui::Panel BaseClass;

	void			OnTextChanged( char const *text );

	vgui::ComboBox		*m_pComboBox;
	vgui::Label			*m_pLabel;
};

#endif // LABELEDCOMBOBOX_H
