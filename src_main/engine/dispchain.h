//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
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

#ifndef DISPCHAIN_H
#define DISPCHAIN_H
#pragma once

#include "dispbuild.h"

class CDispChain
{
public:

	//=========================================================================
	//
	// Constructor/Deconstructor
	//
	CDispChain( bool bForRayCasts = false ) { m_pHead = NULL; m_bForRayCasts = bForRayCasts; }

	void SetForRayCasts( bool b ) { m_bForRayCasts = b; };

	//=========================================================================
	//
	// Update Functions
	//
	void AddTo( IDispInfo *pDispInfo );

	//=========================================================================
	//
	// Render Functions
	//
	void RenderWireframeInLightmapPage( void );
	
	//=========================================================================
	//
	// 
	//
	inline void Clear( void );

	inline void Reset( void )
	{
		m_pHead = NULL;
	}

	inline IDispInfo *GetHead( void );

private:

	IDispInfo *m_pHead;			// points to the head of the displacement chain
	bool m_bForRayCasts;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CDispChain::Clear( void )
{
	m_pHead = NULL;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline IDispInfo *CDispChain::GetHead( void )
{
	return m_pHead;
}

//
// Functions are in DispBuild.cpp
//
void AddDispSurfsToLeafs( model_t *pWorld );

#endif // DISPCHAIN_H


