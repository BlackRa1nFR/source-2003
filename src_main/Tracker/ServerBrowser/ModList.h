//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MODLIST_H
#define MODLIST_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlVector.h"

//-----------------------------------------------------------------------------
// Purpose: Handles parsing of half-life directory for mod info
//-----------------------------------------------------------------------------
class CModList
{
public:
	CModList();

	// returns number of mods 
	int ModCount();

	// returns mod directory string, index valid in range [0, ModCount)
	const char *GetModDir(int index);

private:
	struct mod_t
	{
		char modName[64];
	};

	void ParseInstalledMods();
	void ParseSteamMods();

	CUtlVector<mod_t> m_ModList;
};

// singleton accessor
extern CModList &ModList();


#endif // MODLIST_H
