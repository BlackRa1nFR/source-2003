//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Deals with singleton  
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#if !defined( CLIENTEFFECTPRECACHESYSTEM_H )
#define CLIENTEFFECTPRECACHESYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "IGameSystem.h"
#include "CommonMacros.h"
#include "utlvector.h"
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMaterial.h"

//-----------------------------------------------------------------------------
// Interface to automated system for precaching materials
//-----------------------------------------------------------------------------
class IClientEffect
{
public:
	virtual void Cache( bool precache = true )	= 0;
};

//-----------------------------------------------------------------------------
// Responsible for managing precaching of particles
//-----------------------------------------------------------------------------

class CClientEffectPrecacheSystem : public IGameSystem
{
public:
	// constructor, destructor
	CClientEffectPrecacheSystem() {}
	virtual ~CClientEffectPrecacheSystem() {}

	// Init, shutdown
	virtual bool Init() { return true; }
	virtual void Shutdown();

	// Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();

	virtual void OnSave() {}
	virtual void OnRestore() {}

	// Gets called each frame
	virtual void Update ( float frametime ) {}

	// Pre-rendering setup
	void PreRender( ) {}

	void Register( IClientEffect *effect );

protected:

	CUtlVector< IClientEffect * >	m_Effects;
};

//Singleton accessor
extern CClientEffectPrecacheSystem	*ClientEffectPrecacheSystem();

//-----------------------------------------------------------------------------
// Deals with automated registering and precaching of materials for effects
//-----------------------------------------------------------------------------

class CClientEffect : public IClientEffect
{
public:

	CClientEffect( void )
	{
		//Register with the main effect system
		ClientEffectPrecacheSystem()->Register( this );
	}

//-----------------------------------------------------------------------------
// Purpose: Precache a material by artificially incrementing its reference counter
// Input  : *materialName - name of the material
//		  : increment - whether to increment or decrement the reference counter
//-----------------------------------------------------------------------------

	inline void ReferenceMaterial( const char *materialName, bool increment = true )
	{
		bool		found;
		IMaterial	*material = materials->FindMaterial( materialName, &found );

		if ( found && ( material != NULL ) )
		{
			if ( increment )
			{
				material->IncrementReferenceCount();
			}
			else
			{
				material->DecrementReferenceCount();
			}
		}
	}
};

//Automatic precache macros

//Beginning
#define	CLIENTEFFECT_REGISTER_BEGIN( className )\
namespace className {\
class ClientEffectRegister : public CClientEffect\
{\
private:\
	static const char *m_pszMaterials[];\
public:\
	void Cache( bool precache = true );\
};\
const char *ClientEffectRegister::m_pszMaterials[] = {

//Material definitions
#define	CLIENTEFFECT_MATERIAL( materialName )	materialName,

//End
#define	CLIENTEFFECT_REGISTER_END( )	};\
void ClientEffectRegister::Cache( bool precache )\
{\
	for ( int i = 0; i < ARRAYSIZE( m_pszMaterials ); i++ )\
	{\
		ReferenceMaterial( m_pszMaterials[i], precache );\
	}\
}\
ClientEffectRegister	register_ClientEffectRegister;\
}

#endif	//CLIENTEFFECTPRECACHESYSTEM_H