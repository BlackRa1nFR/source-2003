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

#include "quakedef.h"
#include "server.h"
#include "filter.h"
#include "filesystem.h"
#include "filesystem_engine.h"
#include "proto_oob.h"

static ipfilter_t	ipfilters[MAX_IPFILTERS];
static int			numipfilters;
static userfilter_t userfilters[MAX_USERFILTERS];
static int			numuserfilters = 0;
static ConVar		sv_filterban( "sv_filterban", "1" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *adr - 
//-----------------------------------------------------------------------------
void Filter_SendBan( netadr_t *adr )
{
	char        szMessage[64];
	Q_snprintf( szMessage, sizeof( szMessage ), "banned by server\n" );

	SZ_Clear(&net_message);
	MSG_WriteLong   (&net_message,     0xFFFFFFFF ); // -1 -1 -1 -1 signal
	MSG_WriteByte   (&net_message,     A2C_PRINT );
	MSG_WriteString (&net_message,     szMessage );
	NET_SendPacket( NS_SERVER, net_message.cursize, net_message.data, *adr );
	SZ_Clear(&net_message);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *adr - 
// Output : qboolean
//-----------------------------------------------------------------------------
qboolean Filter_ShouldDiscard( netadr_t *adr )
{
	int		i, j;
	unsigned	in;
	
	in = *(unsigned *)&adr->ip[0];

	// Handle timeouts 
	for (i=numipfilters - 1 ; i >= 0 ; i--)
	{
		if ( ( ipfilters[i].compare != 0xffffffff) &&
			 ( ipfilters[i].banEndTime != 0.0f ) &&
			 ( ipfilters[i].banEndTime <= realtime ) )
		{
			for (j=i+1 ; j<numipfilters ; j++)
			{
				ipfilters[j-1] = ipfilters[j];
			}
			numipfilters--;
			continue;
		}

		// Only get here if ban is still in effect.
		if ( (in & ipfilters[i].mask) == ipfilters[i].compare)
		{
			return sv_filterban.GetFloat();
		}
	}
	return !sv_filterban.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *s - 
//			*f - 
// Output : qboolean Filter_ConvertString
//-----------------------------------------------------------------------------
qboolean Filter_ConvertString (char *s, ipfilter_t *f)
{
	char	num[128];
	int		i, j;
	byte	b[4];
	byte	m[4];
	
	for (i=0 ; i<4 ; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}
	
	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			Con_Printf ("Bad filter address: %s\n", s);
			return false;
		}
		
		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		if (b[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}
	
	f->mask = *(unsigned int *)m;
	f->compare = *(unsigned int *)b;
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Filter_Add_f (void)
{
	int		i;
	float banTime;

	if (Cmd_Argc() != 3)
	{
		Con_Printf("Usage:  addip <minutes> <ipaddress>\nUse 0 minutes for permanent\n");
		return;
	}
	
	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].compare == 0xffffffff)
			break;		// free spot
	
	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			Con_Printf ("IP filter list is full\n");
			return;
		}
		numipfilters++;
	}
	
	banTime = atof(Cmd_Argv(1));
	if (banTime < 0.01f)
		banTime = 0.0f;
	
	ipfilters[i].banTime = banTime;

	if (banTime)
	{
		banTime *= 60.0f;
		banTime += realtime; // Time when we are done banning.
	}
	
	// Time to unban.
	ipfilters[i].banEndTime = banTime;

	if (!Filter_ConvertString (Cmd_Argv(2), &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Filter_Remove_f (void)
{
	ipfilter_t	f;
	int			i, j;

	if (!Filter_ConvertString (Cmd_Argv(1), &f))
		return;

	for (i=0 ; i<numipfilters ; i++)
	{
		if ( (ipfilters[i].mask == f.mask) &&
			 (ipfilters[i].compare == f.compare) )
		{
			for (j=i+1 ; j<numipfilters ; j++)
			{
				ipfilters[j-1] = ipfilters[j];
			}
			numipfilters--;
			Con_Printf ("Filter removed.\n");
			return;
		}
	}
	Con_Printf ("Didn't find %s.\n", Cmd_Argv(1));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Filter_List_f (void)
{
	int		i;
	byte	b[4];

	Con_Printf ("Filter list:\n");
	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;
		if (ipfilters[i].banTime != 0.0f)
			Con_Printf ("%3i.%3i.%3i.%3i : %.3f min\n", b[0], b[1], b[2], b[3], ipfilters[i].banTime);
		else
			Con_Printf ("%3i.%3i.%3i.%3i : permanent\n", b[0], b[1], b[2], b[3]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Filter_Save_f (void)
{
	FileHandle_t f;
	char	name[MAX_OSPATH];
	byte	b[4];
	int		i;
	float banTime;

	Q_snprintf (name, sizeof( name ), "listip.cfg");

	Con_Printf ("Writing %s.\n", name);

	f = g_pFileSystem->Open (name, "wb");
	if (!f)
	{
		Con_Printf ("Couldn't open %s\n", name);
		return;
	}
	
	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;

		// Only store out the permanent bad guys from this server.
		banTime = ipfilters[i].banTime;
		if (banTime != 0.0f)
			continue;

		g_pFileSystem->FPrintf (f, "addip 0.0 %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}
	
	g_pFileSystem->Close (f);
}

// IP Address filtering
static ConCommand addip( "addip", Filter_Add_f, "Add an IP address to the ban list." );
static ConCommand removeip( "removeip", Filter_Remove_f, "Remove an IP address to the ban list."  );
static ConCommand listip( "listip", Filter_List_f, "List IP addresses on the ban list."  );
static ConCommand writeip( "writeip", Filter_Save_f, "Save the ban list to listip.cfg." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Filter_Init( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Filter_Shutdown( void )
{
}
