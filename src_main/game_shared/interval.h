//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef INTERVAL_H
#define INTERVAL_H
#ifdef _WIN32
#pragma once
#endif

struct interval_t
{
	float start;
	float range;
};

interval_t ReadInterval( const char *pString );
float RandomInterval( const interval_t &interval );

#endif // INTERVAL_H
