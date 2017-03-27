//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <vgui/IScheme.h>

#include <vgui_controls/Divider.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
Divider::Divider(Panel *parent, const char *name) : Panel(parent, name)
{
	SetSize(128, 2);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
Divider::~Divider()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets up the border as the line to draw as a divider
//-----------------------------------------------------------------------------
void Divider::ApplySchemeSettings(IScheme *pScheme)
{
	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
	BaseClass::ApplySchemeSettings(pScheme);
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t Divider::m_MessageMap[] =
{
	MAP_MESSAGE( Divider, "fakeEntry", ApplySchemeSettings ),
};
IMPLEMENT_PANELMAP(Divider, BaseClass);