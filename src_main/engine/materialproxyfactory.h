//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MATERIALPROXYFACTORY_H
#define MATERIALPROXYFACTORY_H
#pragma once

#include "materialsystem/imaterialproxyfactory.h"

class CMaterialProxyFactory : public IMaterialProxyFactory
{
public:
	IMaterialProxy *CreateProxy( const char *proxyName );
	void DeleteProxy( IMaterialProxy *pProxy );
};

#endif // MATERIALPROXYFACTORY_H
