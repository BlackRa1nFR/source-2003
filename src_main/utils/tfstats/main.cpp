//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: dummy main.cpp
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "TFStatsApplication.h"


//------------------------------------------------------------------------------------------------------
// Function:	main
// Purpose:	dummy main. passes off execution to TFstats main
// Input:	argc - argument count
//				argv[] - argument list
//------------------------------------------------------------------------------------------------------

void main(int argc, const char* argv[])
{
	//make OS application object, and operating system interface
	g_pApp=new CTFStatsApplication;
	g_pApp->majorVer=1;
	g_pApp->minorVer=5;

#ifdef WIN32
	g_pApp->os=new CTFStatsWin32Interface();
#else
	g_pApp->os=new CTFStatsLinuxInterface();
#endif
	//hand off execution to real main
	g_pApp->main(argc,argv);
}
