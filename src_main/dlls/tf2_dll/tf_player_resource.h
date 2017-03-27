//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_PLAYER_RESOURCE_H
#define TF_PLAYER_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

class CTFPlayerResource : public CPlayerResource
{
	DECLARE_CLASS( CTFPlayerResource, CPlayerResource );
public:
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	virtual void GrabPlayerData( void );

public:
};

#endif // TF_PLAYER_RESOURCE_H
