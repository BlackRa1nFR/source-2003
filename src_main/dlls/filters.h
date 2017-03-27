//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines a base class for entities which filter I/O connections
//			by activator.
//
// $NoKeywords: $
//=============================================================================

#ifndef BASEFILTER_H
#define BASEFILTER_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "entityoutput.h"

// ###################################################################
//	> BaseFilter
// ###################################################################
class CBaseFilter : public CLogicalEntity
{
	DECLARE_CLASS( CBaseFilter, CLogicalEntity );
public:
	DECLARE_DATADESC();

	bool PassesFilter(CBaseEntity *pEntity);

	bool m_bNegated;

	// Inputs
	void InputTestActivator( inputdata_t &inputdata );

	// Outputs
	COutputEvent	m_OnPass;		// Fired when filter is passed
	COutputEvent	m_OnFail;		// Fired when filter is failed

protected:
	virtual bool PassesFilterImpl(CBaseEntity *pEntity);
};

#endif // BASEFILTER_H
