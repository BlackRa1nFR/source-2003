//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================


#include "disp_helpers.h"


void CalcMaxNumVertsAndIndices( int power, int *nVerts, int *nIndices )
{
	int sideLength = (1 << power) + 1;
	*nVerts = sideLength * sideLength;
	*nIndices = (sideLength - 1) * (sideLength - 1) * 2 * 3; 
}

