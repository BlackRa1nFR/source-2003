//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PHYSICS_MATERIAL_H
#define PHYSICS_MATERIAL_H
#pragma once

#include "vphysics_interface.h"
class IVP_Material;
class IVP_Material_Manager;

class IPhysicsSurfacePropsInternal : public IPhysicsSurfaceProps
{
public:
	virtual IVP_Material *GetIVPMaterial( int materialIndex ) = 0;
	
	// some indices are reserved by the system for implementing hard coded behaviors
	// returns a reserved material if this index is reserved, NULL otherwise
	virtual IVP_Material *GetReservedMaterial( int materialIndex ) = 0;

	virtual int GetIVPMaterialIndex( IVP_Material *pIVP ) = 0;
	virtual IVP_Material_Manager *GetIVPManager( void ) = 0;
	virtual int RemapIVPMaterialIndex( int ivpMaterialIndex ) = 0;
};

extern IPhysicsSurfacePropsInternal	*physprops;

// Special material indices outside of the normal system
enum
{
	MATERIAL_INDEX_SHADOW = 0xF000,
};

#endif // PHYSICS_MATERIAL_H
