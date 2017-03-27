//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
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

#include "vrad.h"

bool bOrigFacesTouched[MAX_MAP_FACES];


//-----------------------------------------------------------------------------
// Pupose: clear (reset) the bOrigFacesTouched list -- parellels the original
//         face list allowing an original face to only be processed once in 
//         pairing edges!
//-----------------------------------------------------------------------------
void ResetOrigFacesTouched( void )
{
    for( int i = 0; i < MAX_MAP_FACES; i++ )
    {
        bOrigFacesTouched[i] = false;
    }
}


//-----------------------------------------------------------------------------
// Purpose: mark an original faces as touched (dirty)
//   Input: index - index of the original face touched
//-----------------------------------------------------------------------------
void SetOrigFaceTouched( int index )
{
    bOrigFacesTouched[index] = true;
}


//-----------------------------------------------------------------------------
// Purpose: return whether or not an original face has been touched
//   Input: index - index of the original face touched
//  Output: true/false
//-----------------------------------------------------------------------------
bool IsOrigFaceTouched( int index )
{
    return bOrigFacesTouched[index];
}
