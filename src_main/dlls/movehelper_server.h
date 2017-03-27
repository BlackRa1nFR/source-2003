//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MOVEHELPER_SERVER_H
#define MOVEHELPER_SERVER_H

#ifdef _WIN32
#pragma once
#endif

#include "imovehelper.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class CBasePlayer;
class CBaseEntity;


//-----------------------------------------------------------------------------
// Implementation of the movehelper on the server
//-----------------------------------------------------------------------------

class IMoveHelperServer : public IMoveHelper
{
public:
	virtual void SetHost( CBasePlayer *host ) = 0;
};

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------

IMoveHelperServer* MoveHelperServer();


#endif // MOVEHELPER_SERVER_H
