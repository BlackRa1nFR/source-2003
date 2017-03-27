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

// base vgui interfaces
#include <VGUI_Controls.h>
#include <VGUI_ISurface.h>

// vgui controls
#include <VGUI_Button.h>
#include <VGUI_ComboBox.h>
#include <VGUI_Frame.h>
#include <VGUI_KeyValues.h>
#include <VGUI_HTML.h>
#include <VGUI_ImagePanel.h>
#include <VGUI_AnimatingImagePanel.h>
#include <VGUI_IImage.h>
#include <VGUI_MessageBox.h>


#include "vinternetdlg.h"
#include "browser.h"

using namespace vgui;

static VInternetDlg *s_InternetDlg = NULL;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
VInternetDlg::VInternetDlg( void ) : Frame(NULL, "VInternetDlg")
{
	MakePopup();
	s_InternetDlg=this;
	curAddress=-1;

	// set the smallest size of this window (in pixels)
	SetMinimumSize(550, 400);

	// set the default size ("Browser/browser.res" below can override this)
	SetSize(550, 400);

	// create a HTML component
	m_pHTML = new MyHTML(this, "WebPage");

	// create the address bar
	m_pAddressBar = new ComboBox(GetPanel(), "AddressCombo", MAX_ADDRESSES,true);
	// tell the address bar to send us a message when the ENTER key is hit
	m_pAddressBar->SendNewLine(true);

	// Create an Animating Image Panel
	m_pAnimImagePanel = new AnimatingImagePanel(this, "AnAnimatingImagePanel");

	// Each image file is named c1, c2, c3... c20, one image for each frame of the animation.
	// This loads the 20 images in to the Animation class.
	m_pAnimImagePanel->LoadAnimation("resource\\steam\\c", 20);
	//m_pAnimImagePanel->SetVisible(true);

	// load our control file to setup our window
	LoadControlSettings("Browser/browser.res");

	// put the loading animation on top
	m_pAnimImagePanel->MoveToFront();

	// browse to a default URL
	m_pHTML->OpenURL("http://www.valvesoftware.com");

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
VInternetDlg::~VInternetDlg()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called once to set up
//-----------------------------------------------------------------------------
void VInternetDlg::Initialize()
{
	// setup the name in the title
	SetTitle("Browser", true);
	// hide the window
	SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: Called when the window should be displayed
//-----------------------------------------------------------------------------
void VInternetDlg::Open( void )
{	
	
	surface()->SetMinimized(GetVPanel(), false);
	// make ourselves visible
	SetVisible(true);
	// ask for keyboard focus
	RequestFocus();
	// move to the front of the desktop
	MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: relayouts the dialogs controls
//-----------------------------------------------------------------------------
void VInternetDlg::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	
	// HTML window in middle and get it to fill the window
	m_pHTML->SetBounds(x+10, y+55, GetWide() - 30, tall - 80);

	// put the reload picture in the middle of the screen
	int anim_w,anim_h;
	m_pAnimImagePanel->GetSize(anim_w,anim_h);
	m_pAnimImagePanel->SetPos(x+GetWide()/2-anim_w/2,y+tall/2-anim_h/2);

	Repaint();

}

/*
//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a static instance of this dialog, useful in sort functions
// Output : VInternetDlg
//-----------------------------------------------------------------------------
VInternetDlg *VInternetDlg::GetInstance()
{
	return s_InternetDlg;
}
*/
//-----------------------------------------------------------------------------
// Purpose: Callback from HTML object, updates the status text label
//-----------------------------------------------------------------------------
void VInternetDlg::OnStatus(const char *text)
{
	SetLabelText("StatusText",text);
}

//-----------------------------------------------------------------------------
// Purpose: Callback from HTML object, called when a new URL is loaded
//		This function handles the address history management
//-----------------------------------------------------------------------------
bool VInternetDlg::OnStartURL(const char *text)
{

/*	// This is an example of restricting which pages you let people go to
	
	// check if valvesoftware.com or sierra.com is in the URL 
	if(!strstr(text,"valvesoftware.com") && !strstr(text,"sierra.com") )
	{
		// if it ain't don't let them surf their
		m_pAddressBar->ActivateItem(curAddress);
		return false;
	}
*/

	// stop the window auto-redrawing
	m_pHTML->StopAnimate();


	if( strstr(text,"mailto:") )
	{
		// its an email link, don't add it to the URL list but let them surf to it
		m_pAddressBar->ActivateItem(curAddress);
		return true;	
	}

	// check if the URL ends in .pdf
	if( strstr(text+strlen(text)-4,".pdf") )
	{
		// PDF files don't display properly...
		MessageBox *dlg = new MessageBox("Error", "Acrobat PDF files are currently unsupported.");
		dlg->DoModal();


		m_pAddressBar->ActivateItem(curAddress);
		return false;	
	}

	// if too many entries in the history
	if( m_AddressHistory.Count() >= MAX_ADDRESSES )
	{
		// remove the first one on the list, which is the oldest
		m_AddressHistory.Remove(0);
		curAddress--; // we removed one item
	}

	
	// see if we already have this address
	int i=m_AddressHistory.Count();
	if( m_AddressHistory.Count() )
	{
		for( i=0;i<m_AddressHistory.Count();i++ )
		{
			if( !stricmp(text,m_AddressHistory[i]) )
			{
				break;
			} 
		} // for all addresses
	} // if addresses in history


	if( i==m_AddressHistory.Count() )
	{ // new entry
		char *newUrl = new char[strlen(text)];
		strcpy(newUrl,text);

		if( m_AddressHistory.Count() && curAddress!=m_AddressHistory.Count()-1 )
		{
			curAddress++;
			m_AddressHistory.Remove(curAddress);
			m_AddressHistory.InsertBefore(curAddress,newUrl);
			// update this entry
			m_pAddressBar->AddItem(m_AddressHistory[curAddress],curAddress);
		}
		else
		{
			curAddress++;
			m_AddressHistory.AddToTail(newUrl);
			m_pAddressBar->AddItem(m_AddressHistory[curAddress]);
		}
	}
	else
	{ // existing entry
		curAddress=i;
		// update this item
		m_pAddressBar->AddItem(m_AddressHistory[curAddress],curAddress);
	}

	// now activate the current URL
	m_pAddressBar->ActivateItem(curAddress);

	// and make the logo spin :) (hmm, perhaps a flaming logo...)
	m_pAnimImagePanel->SetVisible(true);


	return true;
}

void VInternetDlg::OnFinishURL()
{
	// make the loading logo go away
	m_pAnimImagePanel->SetVisible(false);

	// get the window to redraw every 1 second, so animiated gifs move
	//m_pHTML->StartAnimate(1000);

}

//-----------------------------------------------------------------------------
// Purpose: Main message handling function, called when buttons are pressed.
//
//		This loop handles the "Go","Stop","Forward" and "Back" button
//
//-----------------------------------------------------------------------------
void VInternetDlg::OnCommand(const char *command)
{
	if(!stricmp(command,"back")) // the back button
	{	
		// backup one 
		curAddress--;
		if(curAddress<0)
		{
			curAddress=0; // already at the start
		}	
		else
		{
			// now ask it to open this URL
			m_pHTML->OpenURL(m_AddressHistory[curAddress]);
		}
	}
	else if(!stricmp(command,"forward")) // the forward button
	{
		// go forward one
		curAddress++;
		if(curAddress>=m_AddressHistory.Count())
		{
			curAddress=m_AddressHistory.Count()-1; // already at the end
		}
		else
		{
			// now ask it to open the URL
			m_pHTML->OpenURL(m_AddressHistory[curAddress]);
		}
	}
	else if(!stricmp(command,"go")) // the go button
	{
		char newUrl[512];
		m_pAddressBar->GetText(0,newUrl,512);
		// open url calls OnStartURL where we handle the history list
		m_pHTML->OpenURL(newUrl);
	}
	else if(!stricmp(command,"stop")) // the stop button
	{
		m_pHTML->StopLoading();
	}
	else if(!stricmp(command,"refresh")) // the refresh button
	{
		m_pHTML->Refresh();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when enter is hit in AddressBar Combo box, passes 
//			a "go" to OnCommand()
//-----------------------------------------------------------------------------
void VInternetDlg::OnAddressBar()
{
	OnCommand("go");
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void VInternetDlg::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t VInternetDlg::m_MessageMap[] =
{
	MAP_MESSAGE( VInternetDlg, "TextNewLine", OnAddressBar ), // message from the addressbar
};
IMPLEMENT_PANELMAP( VInternetDlg, Frame );