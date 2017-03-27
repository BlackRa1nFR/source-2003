//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef FUNC_MONITOR_H
#define FUNC_MONITOR_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "modelentities.h"

class CPointCamera;

class CFuncMonitor : public CFuncBrush
{
public:
	DECLARE_CLASS( CFuncMonitor, CFuncBrush );
	DECLARE_SERVERCLASS();

	void Activate();
	CPointCamera *GetCamera();

	void InputSetCamera(inputdata_t &inputdata);

	DECLARE_DATADESC();

private:

	void SetCameraByName(const char *szName);

	CNetworkVar( int, m_TargetIndex );
	CPointCamera *m_pCamera;
};

#endif // FUNC_MONITOR_H
