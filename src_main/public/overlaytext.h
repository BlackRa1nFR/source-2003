//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( OVERLAYTEXT_H )
#define OVERLAYTEXT_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"

class  OverlayText_t
{
public:
	OverlayText_t()
	{
		nextOverlayText = 0;
		origin.Init();
		bUseOrigin = false;
		lineOffset = 0;
		flXPos = 0;
		flYPos = 0;
		text[ 0 ] = 0;
		m_flEndTime = 0.0f;
		m_nCreationMessage = -1;	
		m_nServerCount = -1;
		m_nCreationFrame = -1;
		r = g = b = a = 255;
	}

	bool			IsDead();
	void			SetEndTime( float duration );

	Vector			origin;
	bool			bUseOrigin;
	int				lineOffset;
	float			flXPos;
	float			flYPos;
	char			text[512];
	float			m_flEndTime;			// When does this text go away
	int				m_nCreationMessage;		// If message has duration 0, then this is the
											//  message when it was created, which is used instead of an endtime
	int				m_nCreationFrame;		// If message has duration -1, then this is the frame when it was created
	int				m_nServerCount;
	int				r;
	int				g;
	int				b;
	int				a;
	OverlayText_t	*nextOverlayText;
};

#endif // OVERLAYTEXT_H