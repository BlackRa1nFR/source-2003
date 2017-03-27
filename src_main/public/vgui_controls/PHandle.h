//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PHANDLE_H
#define PHANDLE_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

namespace vgui
{

class Panel;

//-----------------------------------------------------------------------------
// Purpose: Safe pointer class for handling Panel or derived panel classes
//-----------------------------------------------------------------------------
class PHandle
{
public:
	PHandle() : m_iPanelID(INVALID_PANEL) {} //m_iSerialNumber(0), m_pListEntry(0) {}

	Panel *Get();
	Panel *Set( Panel *pPanel );

	operator Panel *()						{ return Get(); }
	Panel * operator ->()					{ return Get(); }
	Panel * operator = (Panel *pPanel)		{ return Set(pPanel); }

	bool operator == (Panel *pPanel)		{ return (Get() == pPanel); }
	operator bool ()						{ return Get() != 0; }

private:
	HPanel m_iPanelID;
};

//-----------------------------------------------------------------------------
// Purpose: DHANDLE is a templated version of PHandle
//-----------------------------------------------------------------------------
template< class PanelType >
class DHANDLE : public PHandle
{
public:
	PanelType *Get()					{ return (PanelType *)PHandle::Get(); }
	PanelType *Set( PanelType *pPanel )	{ return (PanelType *)PHandle::Set(pPanel); }

	operator PanelType *()						{ return (PanelType *)PHandle::Get(); }
	PanelType * operator ->()					{ return (PanelType *)PHandle::Get(); }
	PanelType * operator = (PanelType *pPanel)	{ return (PanelType *)PHandle::Set(pPanel); }
	bool operator == (Panel *pPanel)			{ return (PHandle::Get() == pPanel); }
	operator bool ()							{ return PHandle::Get() != NULL; }
};

};

#endif // PHANDLE_H
