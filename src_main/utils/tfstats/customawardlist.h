//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of CCustomAwardList
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef CUSTOMAWARDLIST_H
#define CUSTOMAWARDLIST_H
#ifdef WIN32
#pragma once
#endif
#include "CustomAward.h"
#include <list>

using namespace std;
typedef list<CCustomAward*>::iterator CCustomAwardIterator;
//------------------------------------------------------------------------------------------------------
// Purpose: this is just a thin wrapper around a list of CCustomAward*s
// also provided is a static factory method to read a list of custom awards
// out of a configuration file
//------------------------------------------------------------------------------------------------------
class CCustomAwardList
{
public:
	list<CCustomAward*> theList;

	//factory method
	static CCustomAwardList* readCustomAwards(string mapname);

	CCustomAwardIterator begin(){return theList.begin();}
	CCustomAwardIterator end(){return theList.end();}
};
#endif // CUSTOMAWARDLIST_H
