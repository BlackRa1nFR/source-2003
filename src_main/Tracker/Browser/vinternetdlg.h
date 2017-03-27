//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( VINTERNETDLG_H )
#define VINTERNETDLG_H
#ifdef _WIN32
#pragma once
#endif

#include <VGUI_Frame.h>
#include <UtlVector.h>

#include <VGUI_HTML.h>

namespace vgui
{
class Label;
class Button;
class ComboBox;
class HTML;
class AnimatingImagePanel;
}

const int MAX_ADDRESSES = 10;  // the size of the address list

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class VInternetDlg : public vgui::Frame
{
	typedef vgui::Frame BaseClass;

public:
	// Construction/destruction
						VInternetDlg();
	virtual				~VInternetDlg();

	// called to start initialize the browser
	virtual void		Initialize( void );

	// displays the dialog, moves it into focus, updates if it has to
	virtual void		Open( void );
	// called on window changes, to allow dynamic resizing of panels
	virtual void		PerformLayout();

	// button presses
	void OnCommand(const char *command);


	// returns a pointer to a static instance of this dialog
	// valid for use only in sort functions
//	static VInternetDlg *GetInstance();

	// Callbacks from the HTML element
	void OnStatus(const char *text);
	bool OnStartURL(const char *text);
	void OnFinishURL();

private:
	
	// used to set the text in static labels
	void SetLabelText(const char *textEntryName, const char *text);

	// we derive a new class from HTML to override its callback functions 
	class MyHTML: public vgui::HTML
	{
	public:
		MyHTML(VInternetDlg *parent, const char *panelName):vgui::HTML(parent,panelName) {m_parent=parent; };
		~MyHTML() {};

		// override the callbacks, but DON'T forget to still call the base classes ones
		virtual bool OnStartURL(const char *url) { HTML::OnStartURL(url); return m_parent->OnStartURL(url); }
		virtual void OnFinishURL(const char *url) { HTML::OnFinishURL(url); m_parent->OnFinishURL();}
		virtual void OnProgressURL(long current, long maximum) { HTML::OnProgressURL(current,maximum);}
		virtual void OnSetStatusText(const char *text) { HTML::OnSetStatusText(text); m_parent->OnStatus(text); }

	private:
		// store a pointer to the parent, so we can call back on it
		VInternetDlg *m_parent;

	};

	// message handlers
	// called when enter is hit on the address bar
	void OnAddressBar();


	// GUI elements
	// the HTML window
	MyHTML  *m_pHTML;

	// the "Go" button
	vgui::Button *m_pGo;

	// the Address bar
	vgui::ComboBox *m_pAddressBar;

	// the loading logo
	vgui::AnimatingImagePanel *m_pAnimImagePanel;



	// state variables
	// a vector of URL's we have visited
	CUtlVector<char *> m_AddressHistory;
	// the current position within the above list (to support back/forward buttons)
	int curAddress;

	typedef vgui::Frame BaseClass;
	DECLARE_PANELMAP();

};

#endif // VINTERNETDLG_H