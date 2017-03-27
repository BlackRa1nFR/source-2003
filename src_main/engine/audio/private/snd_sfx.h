//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef SND_SFX_H
#define SND_SFX_H

#if defined( _WIN32 )
#pragma once
#endif

class CAudioSource;

class CSfxTable
{
public:
	CSfxTable()
	{
		name[0] = 0;
		pSource = NULL;
	}

	char const *getname();
	void	setname( char const *name );

	char 			name[MAX_PATH];			// pSource points back at this name

	// Index into dictionary
	CAudioSource	*pSource;
};

#endif // SND_SFX_H
