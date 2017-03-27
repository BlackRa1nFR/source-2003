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

#ifndef CMDLINE_H
#define CMDLINE_H
#pragma once

extern		void		ArgsInit( int argc, char *argv[] );
extern		bool		ArgsExist( const char *pName );
extern		const char *ArgsGet( const char *pName, const char *pDefault );
extern		void		Error( const char *pString, ... );


#endif // CMDLINE_H
