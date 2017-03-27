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
#if !defined( ISPECTATORINTERFACE_H )
#define ISPECTATORINTERFACE_H
#ifdef _WIN32
#pragma once
#endif

class ISpectatorInterface
{
public:
	virtual void Activate() = 0;	// shows spectator stuff (black bars etc...)
	virtual void Deactivate() = 0;	// hides spectator stuff
	virtual void ShowGUI() = 0;	// shows spectator menu buttons
	virtual void HideGUI() = 0; // hide user interface again
	
	virtual bool IsGUIVisible() = 0;	// true, if spectator GUI elements are visible
	virtual void SetLogoImage(const char *image) = 0; // set the logo image

	virtual void Update() = 0; // update all elements
	virtual void UpdateSpectatorPlayerList() = 0; // update spectator list panel
	virtual void UpdateTimer() = 0; // update only timer 

	virtual void SetVisible( bool state ) = 0;
	virtual bool IsVisible() = 0;	// true if spectator panel is visible
	virtual void SetParent( vgui::VPANEL parent ) = 0;
	virtual int  GetTopBarHeight() = 0;
	virtual int  GetBottomBarHeight() = 0;


};

#endif // ISPECTATORINTERFACE_H

