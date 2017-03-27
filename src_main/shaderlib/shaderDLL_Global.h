//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#ifndef SHADERDLL_GLOBAL_H
#define SHADERDLL_GLOBAL_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class IShaderSystem;


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
inline IShaderSystem *GetShaderSystem()
{
	extern IShaderSystem* g_pSLShaderSystem;
	return g_pSLShaderSystem;
}


#endif	// SHADERDLL_GLOBAL_H