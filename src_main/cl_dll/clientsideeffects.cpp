//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "clientsideeffects.h"
#include "tier0/vprof.h"

// TODO:  Sort effects and their children back to front from view positon?  At least with buckets or something.

//
//-----------------------------------------------------------------------------
// Purpose: Construct and activate effect
// Input  : *name - 
//-----------------------------------------------------------------------------
CClientSideEffect::CClientSideEffect( const char *name )
{
	m_pszName = name;
	Assert( name );

	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: Destroy effect
//-----------------------------------------------------------------------------
CClientSideEffect::~CClientSideEffect( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get name of effect
// Output : const char
//-----------------------------------------------------------------------------
const char *CClientSideEffect::GetName( void )
{
	return m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Is effect still active?
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CClientSideEffect::IsActive( void )
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: Mark effect for destruction
//-----------------------------------------------------------------------------
void CClientSideEffect::Destroy( void )
{
	m_bActive = false;
}

#define MAX_EFFECTS 128

//-----------------------------------------------------------------------------
// Purpose: Implements effects list interface
//-----------------------------------------------------------------------------
class CEffectsList : public IEffectsList
{
public:
					CEffectsList( void );
	virtual			~CEffectsList( void );

	//	Add an effect to the effects list
	void			AddEffect( CClientSideEffect *effect );
	// Remove the specified effect
	void			RemoveEffect( CClientSideEffect *effect );
	// Draw/update all effects in the current list
	void			DrawEffects( double frametime );
	// Flush out all effects from the list
	void			Flush( void );
private:
	// Current number of effects
	int				m_nEffects;
	// Pointers to current effects
	CClientSideEffect *m_rgEffects[ MAX_EFFECTS ];
};

// Implements effects list and exposes interface
static CEffectsList g_EffectsList;
// Public interface
IEffectsList *clienteffects = ( IEffectsList * )&g_EffectsList;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEffectsList::CEffectsList( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEffectsList::~CEffectsList( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Add effect to effects list
// Input  : *effect - 
//-----------------------------------------------------------------------------
void CEffectsList::AddEffect( CClientSideEffect *effect )
{
	if ( !effect )
	{
		return;
	}

	if ( m_nEffects >= MAX_EFFECTS )
	{
		DevWarning( 1, "No room for effect %s\n", effect->GetName() );
		return;
	}

	m_rgEffects[ m_nEffects++ ] = effect;
}

//-----------------------------------------------------------------------------
// Purpose: Remove specified effect
// Input  : *effect - 
//-----------------------------------------------------------------------------
void CEffectsList::RemoveEffect( CClientSideEffect *effect )
{
	int i;
	bool found = false;

	if ( !effect || ( m_nEffects <= 0 ) )
	{
		return;
	}

	for ( i = 0; i < m_nEffects; i++ )
	{
		if ( m_rgEffects[ i ] == effect )
		{
			m_rgEffects[ i ] = NULL;
			found = true;
			break;
		}
	}
	
	if ( !found )
	{
		DevWarning( 1, "Tried to remove %s from effects list, but it wasn't in the list\n", effect->GetName() );
		return;
	}

	for ( ; i < m_nEffects - 1; i++ )
	{
		m_rgEffects[ i ] = m_rgEffects [ i + 1 ];
	}

	m_nEffects--;

	effect->Destroy();

	delete effect;	//FIXME: Yes, no?
}

//-----------------------------------------------------------------------------
// Purpose: Iterate through list and simulate/draw stuff
// Input  : frametime - 
//-----------------------------------------------------------------------------
void CEffectsList::DrawEffects( double frametime )
{
	VPROF_BUDGET( "CEffectsList::DrawEffects", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	int i;
	CClientSideEffect *effect;

	// Go backwards so deleting effects doesn't screw up
	for ( i = m_nEffects - 1 ; i >= 0; i-- )
	{
		effect = m_rgEffects[ i ];
		if ( !effect )
			continue;

		// Simulate
		effect->Draw( frametime );

		// Remove it if needed
		if ( !effect->IsActive() )
		{
			RemoveEffect( effect );
		}
	}
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CEffectsList::Flush( void )
{
	int i;
	CClientSideEffect *effect;

	// Go backwards so deleting effects doesn't screw up
	for ( i = m_nEffects - 1 ; i >= 0; i-- )
	{
		effect = m_rgEffects[ i ];
		
		if ( effect == NULL )
			continue;

		RemoveEffect( effect );
	}
}
