//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef C_AI_BASENPC_H
#define C_AI_BASENPC_H
#ifdef _WIN32
#pragma once
#endif


#include "c_basecombatcharacter.h"

// NOTE: MOved all controller code into c_basestudiomodel
class C_AI_BaseNPC : public C_BaseCombatCharacter
{
	DECLARE_CLASS( C_AI_BaseNPC, C_BaseCombatCharacter );

public:
	DECLARE_CLIENTCLASS();

	C_AI_BaseNPC();
	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const { return MASK_NPCSOLID; }

private:
	C_AI_BaseNPC( const C_AI_BaseNPC & ); // not defined, not accessible
};


#endif // C_AI_BASENPC_H
