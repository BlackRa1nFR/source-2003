//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: declaration of type InputVar
//=============================================================================

#ifndef INPUTVAR_H
#define INPUTVAR_H

#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "entitylist.h"

//-----------------------------------------------------------------------------
// Purpose: Used to request a value, or a set of values, from a set of entities.
//			used when a multi-input variable needs to refresh it's inputs
//-----------------------------------------------------------------------------
class CMultiInputVar
{
public:
	CMultiInputVar() : m_InputList(NULL) {}
	~CMultiInputVar();

	struct inputitem_t
	{
		variant_t value;	// local copy of variable (maybe make this a variant?) 
		int	outputID;		// the ID number of the output that sent this
		inputitem_t *next;

		// allocate and free from MPool memory
		static void *operator new( size_t stAllocBlock );	
		static void *operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine );
		static void operator delete( void *pMem );
	};

	inputitem_t *m_InputList;	// list of data
	int m_bUpdatedThisFrame;

	void AddValue( variant_t newVal, int outputID );
	
	static typedescription_t m_SaveData[];
};

#endif // INPUTVAR_H
