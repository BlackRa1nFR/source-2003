//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#ifndef ISPATIALPARTITIONINTERNAL_H
#define ISPATIALPARTITIONINTERNAL_H

#include "ispatialpartition.h"


//-----------------------------------------------------------------------------
// These methods of the spatial partition manager are only used in the engine
//-----------------------------------------------------------------------------
class ISpatialPartitionInternal : public ISpatialPartition
{
public:
	// Call this to clear out the spatial partition and to re-initialize
	// it given a particular world size
	virtual void Init( const Vector& worldmin, const Vector& worldmax ) = 0;
};


//-----------------------------------------------------------------------------
// Method to get at the singleton implementation of the spatial partition mgr
//-----------------------------------------------------------------------------
ISpatialPartitionInternal* SpatialPartition();


#endif	// ISPATIALPARTITIONINTERNAL_H



