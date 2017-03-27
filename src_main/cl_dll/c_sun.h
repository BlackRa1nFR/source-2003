//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef C_SUN_H
#define C_SUN_H
#ifdef _WIN32
#pragma once
#endif


#include "c_baseentity.h"
#include "utllinkedlist.h"
#include "glow_overlay.h"



class C_Sun : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_Sun, C_BaseEntity );
	DECLARE_CLIENTCLASS();

					C_Sun();
					~C_Sun();

	virtual void	OnDataChanged( DataUpdateType_t updateType );


public:
	CGlowOverlay	m_Overlay;
	Vector			m_vDirection;
	
	int				m_nLayers;

	int				m_HorzSize;
	int				m_VertSize;

	int				m_bOn;
};


#endif // C_SUN_H
