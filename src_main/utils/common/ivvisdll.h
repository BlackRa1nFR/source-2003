//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef IVVISDLL_H
#define IVVISDLL_H
#ifdef _WIN32
#pragma once
#endif


#define VVIS_INTERFACE_VERSION "vvis_dll_1"


class IVVisDLL
{
public:
	virtual int main( int argc, char **argv ) = 0;
};


#endif // IVVISDLL_H
