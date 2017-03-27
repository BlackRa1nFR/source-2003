//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "effect_dispatch_data.h"
#include "coordsize.h"


#ifdef CLIENT_DLL

	#include "dt_recv.h"

	BEGIN_RECV_TABLE_NOBASE( CEffectData, DT_EffectData )

		RecvPropFloat( RECVINFO( m_vOrigin[0] ) ),
		RecvPropFloat( RECVINFO( m_vOrigin[1] ) ),
		RecvPropFloat( RECVINFO( m_vOrigin[2] ) ),

		RecvPropFloat( RECVINFO( m_vStart[0] ) ),
		RecvPropFloat( RECVINFO( m_vStart[1] ) ),
		RecvPropFloat( RECVINFO( m_vStart[2] ) ),

		RecvPropQAngles( RECVINFO( m_vAngles ) ),

		RecvPropVector(	RECVINFO( m_vNormal ) ),

		RecvPropInt( RECVINFO( m_fFlags ) ),
		RecvPropFloat( RECVINFO( m_flMagnitude ) ),
		RecvPropFloat( RECVINFO( m_flScale ) ),
		RecvPropInt( RECVINFO( m_iEffectName ) ),

		RecvPropInt( RECVINFO( m_nMaterial ) ),
		RecvPropInt( RECVINFO( m_nDamageType ) ),
		RecvPropInt( RECVINFO( m_nHitBox ) ),

		RecvPropInt( RECVINFO( m_nEntIndex ) ),

		RecvPropInt( RECVINFO( m_nColor ) ),

		RecvPropFloat( RECVINFO( m_flRadius ) ),
	END_RECV_TABLE()

#else

	#include "dt_send.h"

	BEGIN_SEND_TABLE_NOBASE( CEffectData, DT_EffectData )

		// Everything uses _NOCHECK here since this is not an entity and we don't need
		// the functionality of CNetworkVars.
		SendPropFloat( SENDINFO_NOCHECK( m_vOrigin[0] ), COORD_INTEGER_BITS, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
		SendPropFloat( SENDINFO_NOCHECK( m_vOrigin[1] ), COORD_INTEGER_BITS, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
		SendPropFloat( SENDINFO_NOCHECK( m_vOrigin[2] ), COORD_INTEGER_BITS, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
		SendPropFloat( SENDINFO_NOCHECK( m_vStart[0] ), COORD_INTEGER_BITS, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
		SendPropFloat( SENDINFO_NOCHECK( m_vStart[1] ), COORD_INTEGER_BITS, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),
		SendPropFloat( SENDINFO_NOCHECK( m_vStart[2] ), COORD_INTEGER_BITS, 0, MIN_COORD_INTEGER, MAX_COORD_INTEGER ),

		SendPropQAngles( SENDINFO_NOCHECK( m_vAngles ), 7 ),

		SendPropVector( SENDINFO_NOCHECK( m_vNormal ), 0, SPROP_NORMAL ),

		SendPropInt( SENDINFO_NOCHECK( m_fFlags ), MAX_EFFECT_FLAG_BITS, SPROP_UNSIGNED ),
		SendPropFloat( SENDINFO_NOCHECK( m_flMagnitude ), 12, SPROP_ROUNDDOWN, 0.0f, 1023.0f ),
		SendPropFloat( SENDINFO_NOCHECK( m_flScale ), 0, SPROP_NOSCALE ),
		SendPropInt( SENDINFO_NOCHECK( m_iEffectName ), MAX_EFFECT_DISPATCH_STRING_BITS, SPROP_UNSIGNED ),

		SendPropInt( SENDINFO_NOCHECK( m_nMaterial ), 16, SPROP_UNSIGNED ),
		SendPropInt( SENDINFO_NOCHECK( m_nDamageType ), 32, SPROP_UNSIGNED ),
		SendPropInt( SENDINFO_NOCHECK( m_nHitBox ), 11, SPROP_UNSIGNED ),

		SendPropInt( SENDINFO_NOCHECK( m_nEntIndex ), MAX_EDICT_BITS, SPROP_UNSIGNED ),

		SendPropInt( SENDINFO_NOCHECK( m_nColor ), 8, SPROP_UNSIGNED ),

		SendPropFloat( SENDINFO_NOCHECK( m_flRadius ), 10, SPROP_ROUNDDOWN, 0.0f, 1023.0f ),

	END_SEND_TABLE()

#endif


