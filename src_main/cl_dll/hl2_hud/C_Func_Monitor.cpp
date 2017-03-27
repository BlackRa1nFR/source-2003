//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

class C_FuncMonitor : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_FuncMonitor, C_BaseEntity );
	DECLARE_CLIENTCLASS();

// C_BaseEntity.
public:
	virtual bool	ShouldDraw();

private:
	int			m_TargetIndex;
};

IMPLEMENT_CLIENTCLASS_DT( C_FuncMonitor, DT_FuncMonitor, CFuncMonitor )
	RecvPropInt( RECVINFO( m_TargetIndex ) ), 
END_RECV_TABLE()

bool C_FuncMonitor::ShouldDraw()
{
	return true;
}
