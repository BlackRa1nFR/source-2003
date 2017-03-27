//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "init_factory.h"

static factorylist_t s_factories;

// Store off the factories
void FactoryList_Store( const factorylist_t &sourceData )
{
	s_factories = sourceData;
}

// retrieve the stored factories
void FactoryList_Retrieve( factorylist_t &destData )
{
	destData = s_factories;
}
