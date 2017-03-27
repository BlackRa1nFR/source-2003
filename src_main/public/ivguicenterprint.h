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
#if !defined( IVGUICENTERPRINT_H )
#define IVGUICENTERPRINT_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

//-----------------------------------------------------------------------------
// Purpose: Engine Center Print Interface
//-----------------------------------------------------------------------------
class ICenterPrint
{
public:
	virtual void		SetTextColor( int r, int g, int b, int a ) = 0;
	virtual void		Print( char *fmt, ... ) = 0;
	virtual void		ColorPrint( int r, int g, int b, int a, char *fmt, ... ) = 0;
	virtual void		Clear( void ) = 0;
};

extern ICenterPrint *centerprint;

#define VCENTERPRINT_INTERFACE_VERSION "VCENTERPRINT001"

#endif // IVGUICENTERPRINT_H