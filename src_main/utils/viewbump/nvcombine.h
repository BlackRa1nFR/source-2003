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

#ifndef NVCOMBINE_H
#define NVCOMBINE_H
#pragma once

// texture 0 = normal map
// texture 1 = cube map
void InitNVForBumpMapping( void );

// texture0 = normal map
// constantColor0 = light vector
// texture1 = lightmap
void InitNVForBumpDotConstantVectorTimesLightmap( void );

#endif // NVCOMBINE_H
