//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
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

#ifndef COLORQUANT_H
#define COLORQUANT_H
#pragma once

int CalcPaletteForImage( unsigned char *image, int numPixels, int bytesPerPixel,
						 unsigned char *palette, int *frequencyCounts, int numPalEntries );

#endif // COLORQUANT_H
