//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: interface and implementation of PID.  
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef PID_H
#define PID_H
#ifdef WIN32
#pragma once
#pragma warning(disable:4786)
#endif
typedef unsigned long PID;
#include <map>
extern std::map<int,PID> pidMap;
#endif // PID_H
