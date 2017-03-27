//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Return a pointer to the Ammo at the Index passed in
//-----------------------------------------------------------------------------
Ammo_t *CAmmoDef::GetAmmoOfIndex(int nAmmoIndex)
{
	if ( nAmmoIndex >= m_nAmmoIndex )
		return NULL;

	return &m_AmmoType[ nAmmoIndex ];
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CAmmoDef::Index(const char *psz)
{
	int i;

	if (!psz)
		return -1;

	for (i = 1; i < m_nAmmoIndex; i++)
	{
		if (stricmp( psz, m_AmmoType[i].pName ) == 0)
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::PlrDamage(int nAmmoIndex)
{
	if ( nAmmoIndex < 1 || nAmmoIndex >= m_nAmmoIndex )
		return 0;

	if ( m_AmmoType[nAmmoIndex].pPlrDmg == USE_CVAR )
	{
		if ( m_AmmoType[nAmmoIndex].pPlrDmgCVar )
			return m_AmmoType[nAmmoIndex].pPlrDmgCVar->GetFloat();

		return 0;
	}
	else
	{
		return m_AmmoType[nAmmoIndex].pPlrDmg;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::NPCDamage(int nAmmoIndex)
{
	if ( nAmmoIndex < 1 || nAmmoIndex >= m_nAmmoIndex )
		return 0;

	if ( m_AmmoType[nAmmoIndex].pNPCDmg == USE_CVAR )
	{
		if ( m_AmmoType[nAmmoIndex].pNPCDmgCVar )
			return m_AmmoType[nAmmoIndex].pNPCDmgCVar->GetFloat();

		return 0;
	}
	else
	{
		return m_AmmoType[nAmmoIndex].pMaxCarry;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::MaxCarry(int nAmmoIndex)
{
	if ( nAmmoIndex < 1 || nAmmoIndex >= m_nAmmoIndex )
		return 0;

	if ( m_AmmoType[nAmmoIndex].pMaxCarry == USE_CVAR )
	{
		if ( m_AmmoType[nAmmoIndex].pMaxCarryCVar )
			return m_AmmoType[nAmmoIndex].pMaxCarryCVar->GetFloat();

		return 0;
	}
	else
	{
		return m_AmmoType[nAmmoIndex].pMaxCarry;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::DamageType(int nAmmoIndex)
{
	if (nAmmoIndex < 1				||
		nAmmoIndex >= m_nAmmoIndex)
	{
		return 0;
	}
	else
	{
		return m_AmmoType[nAmmoIndex].nDamageType;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::TracerType(int nAmmoIndex)
{
	if (nAmmoIndex < 1				||
		nAmmoIndex >= m_nAmmoIndex)
	{
		return 0;
	}
	else
	{
		return m_AmmoType[nAmmoIndex].eTracerType;
	}
}

float CAmmoDef::DamageForce(int nAmmoIndex)
{
	if ( nAmmoIndex < 1 || nAmmoIndex >= m_nAmmoIndex )
		return 0;

	return m_AmmoType[nAmmoIndex].physicsForceImpulse;
}

//-----------------------------------------------------------------------------
// Purpose: Create an Ammo type with the name, decal, and tracer.
// Does not increment m_nAmmoIndex because the functions below do so and 
//  are the only entry point.
//-----------------------------------------------------------------------------
bool CAmmoDef::AddAmmoType(char const* name, int damageType, int tracerType)
{
	if (m_nAmmoIndex == MAX_AMMO_TYPES)
		return false;

	int len = strlen(name);
	m_AmmoType[m_nAmmoIndex].pName = new char[len+1];
	Q_strncpy(m_AmmoType[m_nAmmoIndex].pName, name,len+1);
	m_AmmoType[m_nAmmoIndex].nDamageType	= damageType;
	m_AmmoType[m_nAmmoIndex].eTracerType	= tracerType;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Add an ammo type with it's damage & carrying capability specified via cvars
//-----------------------------------------------------------------------------
void CAmmoDef::AddAmmoType(char const* name, int damageType, int tracerType, char const* plr_cvar, char const* npc_cvar, char const* carry_cvar, float physicsForceImpulse)
{
	if ( AddAmmoType( name, damageType, tracerType ) == false )
		return;

	if (plr_cvar)
	{
		m_AmmoType[m_nAmmoIndex].pPlrDmgCVar	= cvar->FindVar(plr_cvar);
		if (!m_AmmoType[m_nAmmoIndex].pPlrDmgCVar)
		{
			Msg("ERROR: Ammo (%s) found no CVar named (%s)\n",name,plr_cvar);
		}
		m_AmmoType[m_nAmmoIndex].pPlrDmg = USE_CVAR;
	}
	if (npc_cvar)
	{
		m_AmmoType[m_nAmmoIndex].pNPCDmgCVar	= cvar->FindVar(npc_cvar);
		if (!m_AmmoType[m_nAmmoIndex].pNPCDmgCVar)
		{
			Msg("ERROR: Ammo (%s) found no CVar named (%s)\n",name,npc_cvar);
		}
		m_AmmoType[m_nAmmoIndex].pNPCDmg = USE_CVAR;
	}
	if (carry_cvar)
	{
		m_AmmoType[m_nAmmoIndex].pMaxCarryCVar= cvar->FindVar(carry_cvar);
		if (!m_AmmoType[m_nAmmoIndex].pMaxCarryCVar)
		{
			Msg("ERROR: Ammo (%s) found no CVar named (%s)\n",name,carry_cvar);
		}
		m_AmmoType[m_nAmmoIndex].pMaxCarry = USE_CVAR;
	}
	m_AmmoType[m_nAmmoIndex].physicsForceImpulse = physicsForceImpulse;
	m_nAmmoIndex++;
}

//-----------------------------------------------------------------------------
// Purpose: Add an ammo type with it's damage & carrying capability specified via integers
//-----------------------------------------------------------------------------
void CAmmoDef::AddAmmoType(char const* name, int damageType, int tracerType, int plr_dmg, int npc_dmg, int carry, float physicsForceImpulse)
{
	if ( AddAmmoType( name, damageType, tracerType ) == false )
		return;

	m_AmmoType[m_nAmmoIndex].pPlrDmg = plr_dmg;
	m_AmmoType[m_nAmmoIndex].pNPCDmg = npc_dmg;
	m_AmmoType[m_nAmmoIndex].pMaxCarry = carry;
	m_AmmoType[m_nAmmoIndex].physicsForceImpulse = physicsForceImpulse;

	m_nAmmoIndex++;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAmmoDef::CAmmoDef(void)
{
	// Start with an index of 1.  Client assumes 0 is an invalid ammo type
	m_nAmmoIndex = 1;
	memset( m_AmmoType, 0, sizeof( m_AmmoType ) );
}

CAmmoDef::~CAmmoDef( void )
{
	for ( int i = 1; i < MAX_AMMO_TYPES; i++ )
	{
		delete[] m_AmmoType[ i ].pName;
	}
}


