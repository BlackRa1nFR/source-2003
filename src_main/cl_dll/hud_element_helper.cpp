//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Helper for the CHudElement class to add themselves to the list of hud elements
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hud_element_helper.h"

// Start with empty list
CHudElementHelper *CHudElementHelper::m_sHelpers = NULL;

//-----------------------------------------------------------------------------
// Purpose: Constructs a technology factory
// Input  : *name - 
//			void - 
//-----------------------------------------------------------------------------
CHudElementHelper::CHudElementHelper( 
	CHudElement *( *pfnCreate )( void ) )
{
	// Link to global list
	m_pNext			= m_sHelpers;
	m_sHelpers		= this;

	// Set attributes
	assert( pfnCreate );
	m_pfnCreate		= pfnCreate;
}

//-----------------------------------------------------------------------------
// Purpose: Returns next object in list
// Output : CHudElementHelper
//-----------------------------------------------------------------------------
CHudElementHelper *CHudElementHelper::GetNext( void )
{ 
	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: Static class creation factory
//  Searches list of registered factories for a match and then instances the 
//  requested technology by name
// Input  : *name - 
// Output : CBaseTFTechnology
//-----------------------------------------------------------------------------
void CHudElementHelper::CreateAllElements( void )
{
	// Start of list
	CHudElementHelper *p = m_sHelpers;
	while ( p )
	{
		// Dispatch creation function directly
		CHudElement *( *fCreate )( void ) = p->m_pfnCreate;
		CHudElement *newElement = (fCreate)();
		if ( newElement )
		{
			gHUD.AddHudElement( newElement );
		}

		p = p->GetNext();
	}
}

