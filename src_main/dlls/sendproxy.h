//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: implements various common send proxies
//
// $NoKeywords: $
//=============================================================================

#ifndef SENDPROXY_H
#define SENDPROXY_H


#include "dt_send.h"


class DVariant;

void SendProxy_Color32ToInt( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_EHandleToInt( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

SendProp SendPropBool(
	char *pVarName,
	int offset,
	int sizeofVar );

SendProp SendPropEHandle(
	char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	SendVarProxyFn proxyFn=SendProxy_EHandleToInt );

SendProp SendPropTime(
	char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE );

SendProp SendPropPredictableId(
	char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE	);

//-----------------------------------------------------------------------------
// Purpose: Proxy that only sends data to team members
//-----------------------------------------------------------------------------
void* SendProxy_OnlyToTeam( const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );


#endif // SENDPROXY_H
