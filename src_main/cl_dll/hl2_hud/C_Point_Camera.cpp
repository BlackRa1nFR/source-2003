//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "C_Point_Camera.h"
#include "parsemsg.h"

IMPLEMENT_CLIENTCLASS_DT( C_PointCamera, DT_PointCamera, CPointCamera )
	RecvPropFloat( RECVINFO( m_FOV ) ), 
	RecvPropFloat( RECVINFO( m_Resolution ) ), 
	RecvPropInt( RECVINFO( m_bActive ) ),
END_RECV_TABLE()

C_PointCamera::C_PointCamera()
{
	m_bActive = false;
}

bool C_PointCamera::ShouldDraw()
{
	return false;
}

float C_PointCamera::GetFOV()
{
	return m_FOV;
}

float C_PointCamera::GetResolution()
{
	return m_Resolution;
}

bool C_PointCamera::IsActive()
{
	return m_bActive;
}