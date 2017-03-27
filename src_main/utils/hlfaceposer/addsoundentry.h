//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef ADDSOUNDENTRY_H
#define ADDSOUNDENTRY_H
#ifdef _WIN32
#pragma once
#endif

#include "basedialogparams.h"
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct CAddSoundParams : public CBaseDialogParams
{
	char		m_szWaveFile[ 256 ];

	// i/o input text
	char		m_szSoundName[ 256 ];
	char		m_szScriptName[ 256 ];
};

// Display/create dialog
int AddSound( CAddSoundParams *params, HWND parent );

#endif // ADDSOUNDENTRY_H
