//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "cs_hostage.h"


LINK_ENTITY_TO_CLASS( hostage_entity, CHostage );


CUtlVector<CHostage*> g_Hostages;


CHostage::CHostage()
{
	g_Hostages.AddToTail( this );
}


CHostage::~CHostage()
{
	g_Hostages.FindAndRemove( this );
}

