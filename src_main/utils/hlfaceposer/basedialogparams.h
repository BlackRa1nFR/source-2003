//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEDIALOGPARAMS_H
#define BASEDIALOGPARAMS_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "vstdlib/strtools.h"

struct CBaseDialogParams
{
	// i dialog title
	char		m_szDialogTitle[ 128 ];

	bool		m_bPositionDialog;
	int			m_nLeft;
	int			m_nTop;

	void		PositionSelf( void * self );
};

#endif // BASEDIALOGPARAMS_H
