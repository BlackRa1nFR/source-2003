//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: This is the public interface sported by all client-side entities
// Methods that belong in IClientEntityInternal are ones that we need all
// client entities to implement but which we don't want to expose to the server. 
//
// $Revision:$
// $NoKeywords: $

//=============================================================================
#ifndef ICLIENTENTITYINTERNAL_H
#define ICLIENTENTITYINTERNAL_H
#ifdef _WIN32
#pragma once
#endif

#include "IClientEntity.h"
#include "ClientLeafSystem.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class ClientClass;


//-----------------------------------------------------------------------------
// represents a handle used only by the client DLL
//-----------------------------------------------------------------------------

typedef CBaseHandle ClientEntityHandle_t;
typedef unsigned short SpatialPartitionHandle_t;



#endif // ICLIENTENTITYINTERNAL_H