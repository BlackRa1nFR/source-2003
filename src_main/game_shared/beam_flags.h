//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
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
#if !defined( BEAM_FLAGS_H )
#define BEAM_FLAGS_H
#ifdef _WIN32
#pragma once
#endif

enum
{
	FBEAM_STARTENTITY		= 0x00000001,
	FBEAM_ENDENTITY			= 0x00000002,
	FBEAM_FADEIN			= 0x00000004,
	FBEAM_FADEOUT			= 0x00000008,
	FBEAM_SINENOISE			= 0x00000010,
	FBEAM_SOLID				= 0x00000020,
	FBEAM_SHADEIN			= 0x00000040,
	FBEAM_SHADEOUT			= 0x00000080,
	FBEAM_ONLYNOISEONCE		= 0x00000100,		// Only calculate our noise once
	FBEAM_STARTVISIBLE		= 0x10000000,		// Has this client actually seen this beam's start entity yet?
	FBEAM_ENDVISIBLE		= 0x20000000,		// Has this client actually seen this beam's end entity yet?
	FBEAM_ISACTIVE			= 0x40000000,
	FBEAM_FOREVER			= 0x80000000,
};

#endif // BEAM_FLAGS_H