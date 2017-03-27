//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Code for dealing with vertex shaders
//=============================================================================

#ifndef VERTEXDECL_H
#define VERTEXDECL_H

#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/IMaterial.h"
#include "locald3dtypes.h"


//-----------------------------------------------------------------------------
// Gets the declspec associated with a vertex format
//-----------------------------------------------------------------------------
IDirect3DVertexDeclaration9 *FindOrCreateVertexDecl( VertexFormat_t fmt, bool bStaticLit );


//-----------------------------------------------------------------------------
// Clears out all declspecs
//-----------------------------------------------------------------------------
void ReleaseAllVertexDecl( );


#endif // VERTEXDECL_H 

