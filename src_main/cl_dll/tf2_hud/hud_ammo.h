/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef HUD_AMMO_H
#define HUD_AMMO_H

#ifdef _WIN32
#pragma once
#endif

// This is *like* a hud element, but isn't actually a hud element.
// It's meant to be called from hud drawing methods in various places in the code
class CHudAmmo
{
public:
	// Accessor methods to set various state associated with the ammo display
	void SetPrimaryAmmo( int nAmmoType, int nTotalAmmo, int nClipCount = -1, int nMaxClipCount = -1 );
	void SetSecondaryAmmo( int nAmmoType, int nTotalAmmo, int nClipCount = -1, int nMaxClipCount = -1 );

	bool	ShouldShowPrimaryClip() const;
	bool	ShouldShowSecondary() const;

	void	ShowHideHudControls();

private:
	int		m_nClip1;
	int		m_nMaxClip1;
	int		m_nTotalAmmo1;
	int		m_nAmmoType1;

	int		m_nClip2;
	int		m_nMaxClip2;
	int		m_nTotalAmmo2;
 	int		m_nAmmoType2;

friend class CHudAmmoPrimary;
friend class CHudAmmoPrimaryClip;
friend class CHudAmmoSecondary;
};

//-----------------------------------------------------------------------------
// Singleton...
//-----------------------------------------------------------------------------
CHudAmmo* GetHudAmmo();

#endif // HUD_AMMO_H

