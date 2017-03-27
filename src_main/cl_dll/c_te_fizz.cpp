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
#include "cbase.h"
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"

//-----------------------------------------------------------------------------
// Purpose: Fizz TE
//-----------------------------------------------------------------------------
class C_TEFizz : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEFizz, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEFizz( void );
	virtual			~C_TEFizz( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	int				m_nEntity;
	int				m_nModelIndex;
	int				m_nDensity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEFizz::C_TEFizz( void )
{
	m_nEntity		= 0;
	m_nModelIndex	= 0;
	m_nDensity		= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEFizz::~C_TEFizz( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEFizz::PostDataUpdate( DataUpdateType_t updateType )
{
	C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_nEntity );
	if (pEnt != NULL)
	{
		tempents->FizzEffect(pEnt, m_nModelIndex, m_nDensity );
	}
}

void TE_Fizz( IRecipientFilter& filter, float delay,
	const C_BaseEntity *ed, int modelindex, int density )
{
	C_BaseEntity *pEnt = (C_BaseEntity *)ed;
	if (pEnt != NULL)
	{
		tempents->FizzEffect(pEnt, modelindex, density );
	}
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEFizz, DT_TEFizz, CTEFizz)
	RecvPropInt( RECVINFO(m_nEntity)),
	RecvPropInt( RECVINFO(m_nModelIndex)),
	RecvPropInt( RECVINFO(m_nDensity)),
END_RECV_TABLE()


