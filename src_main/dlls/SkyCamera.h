//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Resource collection entity
//
// $NoKeywords: $
//=============================================================================

#ifndef SKYCAMERA_H
#define SKYCAMERA_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "baseentity.h"

class CSkyCamera;

//=============================================================================
//
// Sky List Class
//
class CSkyList : public CAutoGameSystem
{
public:

	virtual void LevelShutdownPostEntity()  { m_pSkyCameras = NULL; }

	static CSkyCamera *m_pSkyCameras;
};

// automatically hooks in the system's callbacks
extern CSkyList g_SkyList;

//=============================================================================
//
// Sky Camera Class
//
class CSkyCamera : public CLogicalEntity
{
	DECLARE_CLASS( CSkyCamera, CLogicalEntity );

public:

	DECLARE_DATADESC();

	void Spawn( void );
	void Precache( void );

public:

	sky3dparams_t	m_skyboxData;

	CSkyCamera		*m_pNextSkyCamera;
};

#endif // SKYCAMERA_H
