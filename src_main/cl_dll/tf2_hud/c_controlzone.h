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
// $NoKeywords: $
//=============================================================================

#if !defined( C_CONTROLZONE_H )
#define C_CONTROLZONE_H
#ifdef _WIN32
#pragma once
#endif

class ConVar;
//-----------------------------------------------------------------------------
// Purpose: Client side rep of control zone entity ( trigger, so not usually visible )
//-----------------------------------------------------------------------------
class C_ControlZone : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_ControlZone, C_BaseEntity );
	DECLARE_CLIENTCLASS();

					C_ControlZone();
	virtual			~C_ControlZone();

	virtual	bool	ShouldDraw();
	virtual void	OnDataChanged( DataUpdateType_t updateType );

public:
	int				m_nZoneNumber;

private:
	const ConVar	*m_pShowTriggers;
};

#endif // C_CONTROLZONE_H