//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "skycamera.h"


//=============================================================================

CSkyCamera *CSkyList::m_pSkyCameras = NULL;
CSkyList g_SkyList;

LINK_ENTITY_TO_CLASS( sky_camera, CSkyCamera );

BEGIN_DATADESC( CSkyCamera )

	DEFINE_KEYFIELD( CSkyCamera, m_skyboxData.scale, FIELD_INTEGER, "scale" ),
	DEFINE_FIELD( CSkyCamera, m_skyboxData.origin, FIELD_VECTOR ),
	DEFINE_FIELD( CSkyCamera, m_skyboxData.area, FIELD_INTEGER ),

	// Quiet down classcheck
	// DEFINE_FIELD( CSkyCamera, m_skyboxData, sky3dparams_t ),

	// This is re-set up during precache
	// DEFINE_FIELD( CSkyCamera, m_pNextSkyCamera, CSkyCamera ),

	// fog data for 3d skybox
	DEFINE_KEYFIELD( CSkyCamera, m_skyboxData.fog.enable,			FIELD_BOOLEAN, "fogenable" ),
	DEFINE_KEYFIELD( CSkyCamera, m_skyboxData.fog.blend,			FIELD_BOOLEAN, "fogblend" ),
	DEFINE_KEYFIELD( CSkyCamera, m_skyboxData.fog.dirPrimary,		FIELD_VECTOR, "fogdir" ),
	DEFINE_KEYFIELD( CSkyCamera, m_skyboxData.fog.colorPrimary,		FIELD_COLOR32, "fogcolor" ),
	DEFINE_KEYFIELD( CSkyCamera, m_skyboxData.fog.colorSecondary,	FIELD_COLOR32, "fogcolor2" ),
	DEFINE_KEYFIELD( CSkyCamera, m_skyboxData.fog.start,			FIELD_FLOAT, "fogstart" ),
	DEFINE_KEYFIELD( CSkyCamera, m_skyboxData.fog.end,				FIELD_FLOAT, "fogend" ),

END_DATADESC()


void CSkyCamera::Spawn( void ) 
{ 
	m_skyboxData.origin = GetLocalOrigin();
	m_skyboxData.area = engine->GetArea( m_skyboxData.origin );
	Precache(); 
}

void CSkyCamera::Precache( void )
{
	m_pNextSkyCamera = CSkyList::m_pSkyCameras;
	CSkyList::m_pSkyCameras = this;
	BaseClass::Precache();
}


