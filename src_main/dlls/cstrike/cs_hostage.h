//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef CS_HOSTAGE_H
#define CS_HOSTAGE_H
#ifdef _WIN32
#pragma once
#endif


#include "basecombatcharacter.h"
#include "utlvector.h"


class CHostage : public CBaseCombatCharacter
{
public:
	CHostage();
	virtual ~CHostage();
};


// All the hostage entities.
extern CUtlVector<CHostage*> g_Hostages;


#endif // CS_HOSTAGE_H
