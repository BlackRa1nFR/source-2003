//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CACHE_USER_H
#define CACHE_USER_H

#ifdef _WIN32
#pragma once
#endif

// make sure and change VENGINE_CACHE_INTERFACE_VERSION in cdll_int.h if you change this!
struct cache_user_t
{
	void *data;
};

#endif // CACHE_USER_H
