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
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include <malloc.h>
#include "eiface.h"
#include "server.h"
#include "bspfile.h"
#include "cmodel_engine.h"
#include "game_interface.h"
#include "vengineserver_impl.h"
#include "vox.h"
#include "sound.h"
#include "gl_model_private.h"
#include "host_saverestore.h"
#include "world.h"
#include "l_studio.h"
#include "decal.h"
#include "sys_dll.h"
#include "sv_log.h"
#include "sv_main.h"
#include "vstdlib/strtools.h"
#include "collisionutils.h"
#include "staticpropmgr.h"
#include "string_t.h"
#include "vstdlib/random.h"
#include "EngineSoundInternal.h"
#include "dt_send_eng.h"
#include "PlayerState.h"
#include "irecipientfilter.h"
#include <KeyValues.h>
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include "tmessage.h"

#define MAX_MESSAGE_SIZE	2500


// ---------------------------------------------------------------------- //
// Globals.
// ---------------------------------------------------------------------- //
qboolean			g_bInEditMode;		// In wc edit mode?

struct MsgData
{
	MsgData() : buffer( "MsgData.buffer", data, sizeof( data ) )
	{
		Reset();
	}

	void Reset()
	{
		filter			= NULL;
		type			= 0;
		subtype			= 0;
		started			= false;
		usermessagesize = -1;
		usermessagename = NULL;
	}

	IRecipientFilter	*filter;
	byte				data[ MAX_MESSAGE_SIZE ];
	bf_write			buffer;
	int					type;				// SVC_TEMPENTITY,  etc.
	int					subtype;			// usermessage index
	bool				started;			// IS THERE A MESSAGE IN THE PROCESS OF BEING SENT?
	int					usermessagesize;
	char const			*usermessagename;
};

static MsgData msg;

extern ConVar ent_developer;

void SeedRandomNumberGenerator( bool random_invariant )
{
	if (!random_invariant)
	{
		int iSeed = -(long)Sys_FloatTime();
		if (1000 < iSeed)
		{
			iSeed = -iSeed;
		}
		else if (-1000 < iSeed)
		{
			iSeed -= 22261048;
		}
		RandomSeed( iSeed );
	}
	else
	{
		// Make those random numbers the same every time!
		RandomSeed( 0 );
	}
}

// ---------------------------------------------------------------------- //
// Static helpers.
// ---------------------------------------------------------------------- //

static void PR_CheckEmptyString (const char *s)
{
	if (s[0] <= ' ')
		Host_Error ("Bad string: %s", s);
}
// Average a list a vertices to find an approximate "center"
void CenterVerts( Vector verts[], int vertCount, Vector& center )
{
	int i;
	float scale;

	if ( vertCount )
	{
		Vector edge0, edge1, normal;

		VectorCopy( vec3_origin, center );
		// sum up verts
		for ( i = 0; i < vertCount; i++ )
		{
			VectorAdd( center, verts[i], center );
		}
		scale = 1.0f / (float)vertCount;
		VectorScale( center, scale, center );	// divide by vertCount

		// Compute 2 poly edges
		VectorSubtract( verts[1], verts[0], edge0 );
		VectorSubtract( verts[vertCount-1], verts[0], edge1 );
		// cross for normal
		CrossProduct( edge0, edge1, normal );
		// Find the component of center that is outside of the plane
		scale = DotProduct( center, normal ) - DotProduct( verts[0], normal );
		// subtract it off
		VectorMA( center, scale, normal, center );
		// center is in the plane now
	}
}


// Copy the list of verts from an msurface_t int a linear array
void SurfaceToVerts( model_t *model, int surfID, Vector verts[], int *vertCount )
{
	int i;

	if ( *vertCount > MSurf_VertCount( surfID, model ) )
		*vertCount = MSurf_VertCount( surfID, model );

	// Build the list of verts from 0 to n
	for ( i = 0; i < *vertCount; i++ )
	{
		int vertIndex = model->brush.vertindices[ MSurf_FirstVertIndex( surfID, model ) + i ];
		Vector& vert = model->brush.vertexes[ vertIndex ].position;
		VectorCopy( vert, verts[i] );
	}
	// vert[0] is the first and last vert, there is no copy
}


// Calculate the surface area of an arbitrary msurface_t polygon (convex with collinear verts)
float SurfaceArea( model_t *model, int surfID )
{
	Vector	center, verts[32];
	int		vertCount = 32;
	float	area;
	int		i;

	// Compute a "center" point and fan
	SurfaceToVerts( model, surfID, verts, &vertCount );
	CenterVerts( verts, vertCount, center );

	area = 0;
	// For a triangle of the center and each edge
	for ( i = 0; i < vertCount; i++ )
	{
		Vector edge0, edge1, out;
		int next;

		next = (i+1)%vertCount;
		VectorSubtract( verts[i], center, edge0 );			// 0.5 * edge cross edge (0.5 is done once at the end)
		VectorSubtract( verts[next], center, edge1 );
		CrossProduct( edge0, edge1, out );
		area += VectorLength( out );
	}
	return area * 0.5;										// 0.5 here
}


// Average the list of vertices to find an approximate "center"
void SurfaceCenter( model_t *model, int surfID, Vector& center )
{
	Vector	verts[32];		// We limit faces to 32 verts elsewhere in the engine
	int		vertCount = 32;

	SurfaceToVerts( model, surfID, verts, &vertCount );
	CenterVerts( verts, vertCount, center );
}


static qboolean ValidCmd( const char *pCmd )
{
	int len;

	len = strlen(pCmd);

	// Valid commands all have a ';' or newline '\n' as their last character
	if ( len && (pCmd[len-1] == '\n' || pCmd[len-1] == ';') )
		return true;

	return false;
}


// ---------------------------------------------------------------------- //
// CVEngineServer
// ---------------------------------------------------------------------- //
class CVEngineServer : public IVEngineServer
{
public:
	/*
	==============
	ChangeLevel
	==============
	*/
	virtual void ChangeLevel(char* s1, char* s2)
	{
		static	int	last_spawncount;

		// make sure we don't issue two changelevels
		if (svs.spawncount == last_spawncount)
			return;
		last_spawncount = svs.spawncount;
		
		//if (svs.changelevel_issued)
		//	return;
		//svs.changelevel_issued = true;
		
		if ( !s2 ) // no indication of where they are coming from;  so just do a standard old changelevel
			Cbuf_AddText (va("changelevel %s\n",s1));
		else
			Cbuf_AddText (va("changelevel2 %s %s\n",s1, s2));
	}


	virtual int	IsMapValid( char *filename )
	{
		return modelloader->Map_IsValid( filename );
	}

	virtual int IsDedicatedServer( void )
	{
		return isDedicated;
	}

	virtual int IsInEditMode( void )
	{
		return g_bInEditMode;
	}

	virtual int PrecacheDecal( const char *name, bool preload /*=false*/ )
	{
		PR_CheckEmptyString( name );
		int i = SV_FindOrAddDecal( name, preload );
		if ( i >= 0 )
		{
			return i;
		}

		Host_Error( "CVEngineServer::PrecacheDecal: '%s' overflow, too many decals", name );
		return 0;
	}

	virtual int PrecacheModel( const char *s, bool preload /*= false*/ )
	{
		PR_CheckEmptyString (s);
		int i = SV_FindOrAddModel( s, preload );
		if ( i >= 0 )
		{
			return i;
		}

		Host_Error( "CVEngineServer::PrecacheModel: '%s' overflow, too many models", s );
		return 0;
	}


	virtual int PrecacheGeneric(const char *s, bool preload /*= false*/ )
	{
		int		i;

		PR_CheckEmptyString (s);
		i = SV_FindOrAddGeneric( s, preload );
		if (i >= 0)
		{
			return i;
		}

		Host_Error ("CVEngineServer::PrecacheGeneric: '%s' overflow", s);
		return 0;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Precache a sentence file (parse on server, send to client)
	// Input  : *s - file name
	//-----------------------------------------------------------------------------
	virtual int PrecacheSentenceFile( const char *s, bool preload /*= false*/ )
	{
		// UNDONE:  Set up preload flag

		// UNDONE: Send this data to the client to support multiple sentence files
		VOX_ReadSentenceFile( s );

		return 0;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Forces an entity to relink itself into the collision tree
	// Input  : *e - 
	//			bFireTriggers - 
	// Output : 	virtual void
	//-----------------------------------------------------------------------------
	virtual void RelinkEntity( edict_t *e, bool bFireTriggers /*=false*/, const Vector *pPrevAbsOrigin /*= 0*/ )
	{
		SV_LinkEdict( e, bFireTriggers, pPrevAbsOrigin );
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *e - 
	// Output : 	virtual void
	//-----------------------------------------------------------------------------
	virtual void FastRelink( edict_t *e, int temphandle )
	{
		SV_FastRelink( e, temphandle );
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *e - 
	// Output : 	virtual void
	//-----------------------------------------------------------------------------
	virtual int FastUnlink( edict_t *e )
	{
		return SV_FastUnlink( e );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Retrieves the pvs for an origin into the specified array
	// Input  : *org - origin
	//			outputpvslength - size of outputpvs array in bytes
	//			*outputpvs - If null, then return value is the needed length
	// Output : int - length of pvs array used ( in bytes )
	//-----------------------------------------------------------------------------
	virtual int GetClusterForOrigin( const Vector& org )
	{
		return CM_LeafCluster( CM_PointLeafnum( org ) );
	}

	virtual int GetPVSForCluster( int clusterIndex, int outputpvslength, unsigned char *outputpvs )
	{
		byte *pvs;

		pvs = CM_ClusterPVS( clusterIndex );
		
		int length = (CM_NumClusters()+7)>>3;

		if ( outputpvs )
		{
			if ( outputpvslength < length )
			{
				Sys_Error( "GetPVSForOrigin called with inusfficient sized pvs array, need %i bytes!", length );
				return length;
			}
	
			memcpy( outputpvs, pvs, length );
		}

		return length;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Test origin against pvs array retreived from GetPVSForOrigin
	// Input  : *org - origin to chec
	//			checkpvslength - length of pvs array
	//			*checkpvs - 
	// Output : bool - true if entity is visible
	//-----------------------------------------------------------------------------
	virtual bool CheckOriginInPVS( const Vector& org, const unsigned char *checkpvs )
	{
		int clusterIndex = CM_LeafCluster( CM_PointLeafnum( org ) );

		if ( clusterIndex < 0 )
			return false;

		if ( !(checkpvs[clusterIndex>>3] & (1<<(clusterIndex&7)) ) )
		{
			return false;
		}

		return true;
	}
	
	//-----------------------------------------------------------------------------
	// Purpose: Test origin against pvs array retreived from GetPVSForOrigin
	// Input  : *org - origin to chec
	//			checkpvslength - length of pvs array
	//			*checkpvs - 
	// Output : bool - true if entity is visible
	//-----------------------------------------------------------------------------
	virtual bool CheckBoxInPVS( const Vector& mins, const Vector& maxs, const unsigned char *checkpvs )
	{
		if ( !CM_BoxVisible( mins, maxs, ( byte * )checkpvs ) )
		{
			return false;
		}

		return true;
	}

	virtual int GetPlayerUserId( edict_t *e )
	{
		client_t *cl;
		int i;
		
		if ( !sv.active )
			return -1;

		if ( !e )
			return -1;

		for ( i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++ )
		{
			if ( cl->edict == e )
			{
				return cl->userid;
			}
		}

		// Couldn't find it
		return -1;
	}


	virtual int IndexOfEdict(const edict_t *pEdict)
	{
		int index;

		if ( !pEdict )
		{
			return 0;
		}

		index = (int) ( pEdict - sv.edicts );
		if ( index < 0 || index > sv.max_edicts )
		{
			Sys_Error( "Bad entity in IndexOfEdict() index %i pEdict %p sv.edicts %p\n",
				index, pEdict, sv.edicts );
		}
		
		return index;
	}


	// Returns a pointer to an entity from an index,  but only if the entity
	// is a valid DLL entity (ie. has an attached class)
	virtual edict_t* PEntityOfEntIndex(int iEntIndex)
	{
		if ( iEntIndex >= 0 && iEntIndex < sv.max_edicts )
		{
			edict_t *pEdict = EDICT_NUM( iEntIndex );
			if ( !pEdict->free )
			{
				return pEdict;
			}
		}

		return NULL;
	}

	virtual int	GetEntityCount( void )
	{
		return sv.num_edicts;
	}

	virtual int	EntityToClientIndex(int iEntIndex)
	{
		return iEntIndex - 1;
	}

	virtual void GetPlayerConnectionInfo( int playerIndex, int& ping, int &packetloss )
	{
		if ( playerIndex < 1 || playerIndex > svs.maxclients )
			return;

		client_t *cl = svs.clients + playerIndex - 1;
		if ( !cl->active  )
		{
			ping		= 0;
			packetloss	= 0;
		}
		else
		{
			// Use the adjusted value here
			ping		= (int)SV_CalcLatency( cl );
			packetloss	= SV_CalcPacketLoss( cl );
		}
	}

	virtual float		GetPlayerPing( int playerIndex, int numsamples )
	{
		if ( playerIndex < 1 || playerIndex > svs.maxclients )
		{
			return 0.0f;
		}

		client_t *cl = svs.clients + playerIndex - 1;
		if ( !cl->active  )
		{
			return 0.0f;
		}
				
		float ping = SV_CalcPing( cl, numsamples );
		return ping;
	}
	

	virtual edict_t* CreateEdict(void)
	{
		edict_t	*pedict = ED_Alloc();
		return pedict;
	}


	virtual void RemoveEdict(edict_t* ed)
	{
		ED_Free(ed);
	}

	//
	// Request engine to allocate "cb" bytes on the entity's private data pointer.
	//
	virtual void *PvAllocEntPrivateData( long cb )
	{
		return calloc( 1, cb );
	}


	//
	// Release the private data memory, if any.
	//
	virtual void FreeEntPrivateData( void *pEntity )
	{
		#ifdef _DEBUG
			// set the memory to a known value
			int size = _msize( pEntity );
			memset( pEntity, 0xDD, size );
		#endif		

		if ( pEntity )
		{
			free( pEntity );
		}
	}

	virtual void		*SaveAllocMemory( size_t num, size_t size )
	{
#ifndef SWDS
		return ::SaveAllocMemory(num, size);
#else
		return NULL;
#endif
	}

	virtual void		SaveFreeMemory( void *pSaveMem )
	{
#ifndef SWDS
		::SaveFreeMemory(pSaveMem);
#endif
	}

	/*
	=================
	EmitAmbientSound

	=================
	*/
	virtual void EmitAmbientSound(edict_t *entity, const Vector& pos, const char *samp, float vol, 
										soundlevel_t soundlevel, int fFlags, int pitch, float soundtime /*=0.0f*/ )
	{
		int			soundnum;
		int			ent;
		bf_write	*pout;

		// if this is a sentence, get sentence number
		if ( TestSoundChar(samp, CHAR_SENTENCE) )
		{
			fFlags |= SND_SENTENCE;
			soundnum = atoi(PSkipSoundChars(samp));
			if ( soundnum >= VOX_SentenceCount() )
			{
				Con_Printf("invalid sentence number: %s", PSkipSoundChars(samp));
				return;
			}
		}
		else
		{
			// check to see if samp was properly precached
			soundnum = SV_SoundIndex(samp);
			if (soundnum <= 0)
			{
				Con_Printf ("no precache: %s\n", samp);
				return;
			}
		}

		ent = NUM_FOR_EDICT(entity);

		if (fFlags & SND_SPAWNING)
		{
			pout = &sv.signon; 		// we're starting a level, so...
									// add an svc_spawnambient command to the level signon packet

			// This really only applies to the first player to connect, but that works in single player well enought
			if ( !sv.allowsignonwrites )
			{
				Con_DPrintf( "EmitAmbientSound(SND_SPAWNING) %s:  Init message being created after signon buffer has been transmitted\n", samp );
			}
		}
		else
		{
			pout = &sv.datagram;	// write through normal message block
		}

		if ( vol != DEFAULT_SOUND_PACKET_VOLUME )
		{
			fFlags |= SND_VOLUME;
		}
		if ( soundlevel != SNDLVL_NORM ) 
		{
			fFlags |= SND_SOUNDLEVEL;
		}
		if ( pitch != DEFAULT_SOUND_PACKET_PITCH )
		{
			fFlags |= SND_PITCH;
		}
		if ( soundtime != 0.0f )
		{
			fFlags |= SND_DELAY;
		}

		pout->WriteByte (svc_spawnstaticsound);

		pout->WriteUBitLong( fFlags, SND_FLAG_BITS_ENCODE );

		// Round it off so now fractional position is sent
		pout->WriteBitCoord( (int)pos.x );
		pout->WriteBitCoord( (int)pos.y );
		pout->WriteBitCoord( (int)pos.z );

		pout->WriteUBitLong ( soundnum, MAX_SOUND_INDEX_BITS );

		if (fFlags & SND_VOLUME)
		{
			pout->WriteByte ( vol*255 );
		}
		if (fFlags & SND_SOUNDLEVEL)
		{
			pout->WriteByte ( soundlevel );
		}
		pout->WriteUBitLong( ent, MAX_EDICT_BITS );
		if (fFlags & SND_PITCH)
		{
			pout->WriteByte( pitch );
		}

		if ( fFlags & SND_DELAY )
		{
			int delay_msec = clamp( (int)( ( soundtime - sv.gettime() ) * 1000.0f ), -10 * MAX_SOUND_DELAY_MSEC, MAX_SOUND_DELAY_MSEC );
			if ( delay_msec < 0 )
			{
				delay_msec *= 0.1f;
			}

			pout->WriteBitLong( delay_msec, MAX_SOUND_DELAY_MSEC_ENCODE_BITS, true );
		}

	}


	virtual void FadeClientVolume(const edict_t *clientent,
		float fadePercent, float fadeOutSeconds, float holdTime, float fadeInSeconds)
	{
		client_t	*client;
		int entnum = NUM_FOR_EDICT(clientent);

		if (entnum < 1 || entnum > svs.maxclients)
		{
			Con_Printf ("tried to DLL_FadeClientVolume a non-client\n");
			return;
		}
			
		client = &svs.clients[entnum-1];
			
		client->netchan.message.WriteChar( svc_soundfade );

		client->netchan.message.WriteFloat( fadePercent );
		client->netchan.message.WriteFloat( holdTime );
		client->netchan.message.WriteFloat( fadeOutSeconds );
		client->netchan.message.WriteFloat( fadeInSeconds );
	}


	//-----------------------------------------------------------------------------
	//
	// Sentence API
	//
	//-----------------------------------------------------------------------------

	virtual int SentenceGroupPick( int groupIndex, char *name, int nameLen )
	{
		return VOX_GroupPick( groupIndex, name, nameLen );
	}


	virtual int SentenceGroupPickSequential( int groupIndex, char *name, int nameLen, int sentenceIndex, int reset )
	{
		return VOX_GroupPickSequential( groupIndex, name, nameLen, sentenceIndex, reset );
	}

	virtual int SentenceIndexFromName( const char *pSentenceName )
	{
		int sentenceIndex = -1;
		
		VOX_LookupString( pSentenceName, &sentenceIndex );
		
		return sentenceIndex;
	}

	virtual const char *SentenceNameFromIndex( int sentenceIndex )
	{
		return VOX_SentenceNameFromIndex( sentenceIndex );
	}


	virtual int SentenceGroupIndexFromName( const char *pGroupName )
	{
		return VOX_GroupIndexFromName( pGroupName );
	}

	virtual const char *SentenceGroupNameFromIndex( int groupIndex )
	{
		return VOX_GroupNameFromIndex( groupIndex );
	}


	virtual float SentenceLength( int sentenceIndex )
	{
		return VOX_SentenceLength( sentenceIndex );
	}
	//-----------------------------------------------------------------------------

	virtual int			CheckHeadnodeVisible( int nodenum, byte *visbits )
	{
		return CM_HeadnodeVisible(nodenum, visbits);
	}

	/*
	=================
	ServerCommand

	Sends text to servers execution buffer

	localcmd (string)
	=================
	*/
	virtual void ServerCommand( char *str )
	{
		if ( ValidCmd( str ) )
			Cbuf_AddText(str);
		else
			Con_Printf("Error, bad server command %s\n", str );
	}


	/*
	=================
	ServerExecute

	Executes all commands in server buffer

	localcmd (string)
	=================
	*/
	virtual void ServerExecute( void )
	{
		Cbuf_Execute();
	}


	/*
	=================
	ClientCommand

	Sends text over to the client's execution buffer

	stuffcmd (clientent, value)
	=================
	*/
	virtual void ClientCommand(edict_t* pEdict, char* szFmt, ...)
	{
		int entnum = NUM_FOR_EDICT(pEdict);
		client_t	*old;
		va_list		argptr; 
		static char	szOut[1024];

		va_start(argptr, szFmt);
		vsprintf(szOut, szFmt, argptr);
		va_end(argptr);
		
		if (entnum < 1 ||
			entnum > svs.maxclients)
		{
			Con_Printf("\n!!!\n\nStuffCmd:  Some entity tried to stuff '%s' to console buffer of entity %i when maxclients was set to %i, ignoring\n\n",
				szOut, entnum, svs.maxclients);
			return;
		}
		//	Host_Error ("StuffCmd: supplied entity is not a client");

		if ( ValidCmd( szOut ) )
		{
			old = host_client;
			host_client = &svs.clients[entnum-1];
			SV_SendClientCommands ("%s", szOut);
			host_client = old;
		}
		else
			Con_Printf("Tried to stuff bad command %s\n", szOut );
	}

	/*
	===============
	LightStyle

	void(float style, string value) lightstyle
	===============
	*/
	virtual void LightStyle(int style, char* val)
	{
		client_t	*client;
		int			j;

	// change the string in sv
		sv.lightstyles[style] = val;
		
	// send message to all clients on this server
		if (sv.state != ss_active)
			return;
		
		for (j=0, client = svs.clients ; j<svs.maxclients ; j++, client++)
		{
			if ( client->active || client->spawned )
			{
				client->netchan.message.WriteByte( svc_lightstyle );
				client->netchan.message.WriteOneBit( 0 );
				client->netchan.message.WriteUBitLong( style, MAX_LIGHTSTYLE_INDEX_BITS );
				client->netchan.message.WriteString( val );
			}
		}
	}


	virtual void StaticDecal( const Vector& origin, int decalIndex, int entityIndex, int modelIndex )
	{
		sv.signon.WriteByte( svc_bspdecal );
		sv.signon.WriteBitVec3Coord( origin );
		sv.signon.WriteUBitLong( decalIndex, MAX_DECAL_INDEX_BITS );
		sv.signon.WriteUBitLong( entityIndex, MAX_EDICT_BITS );
		if ( entityIndex )
		{
			sv.signon.WriteUBitLong( modelIndex, SP_MODEL_INDEX_BITS );
		}
	}

	void Message_DetermineMulticastRecipients( bool usepas, const Vector& origin, unsigned int& playerbits )
	{
		SV_DetermineMulticastRecipients( usepas, origin, playerbits );
	}

	/*
	===============================================================================

	MESSAGE WRITING

	===============================================================================
	*/

	virtual bf_write *MessageBegin( IRecipientFilter& filter, int msg_type /*=0*/ )
	{
		msg.Reset();

		msg.filter = &filter;

		if ( msg.started )
		{
			Sys_Error( "MessageBegin:  New message started when msg '%d' has not been sent yet", msg.type );
		}

		msg.started = true;

		if ( filter.IsEntityMessage() )
		{
			msg.type = svc_entitymessage;
		}
		else
		{
			msg.type = msg_type;
			Assert( msg.type != 0 );
		}

		msg.buffer.Reset();

		return &msg.buffer;
	}

	virtual bf_write *UserMessageBegin( IRecipientFilter& filter, int msg_index, int msg_size, char const *message_name )
	{
		msg.Reset();

		msg.filter = &filter;
	
		if ( msg.started )
		{
			Sys_Error( "UserMessageBegin:  New message (%s) started when msg '%d' has not been sent yet", message_name, msg_index );
		}

		msg.started = true;

		if ( filter.IsEntityMessage() )
		{
			Sys_Error( "UserMessageBegin:  Can't send usermessage (%s) to entity!", message_name );
		}

		msg.type				= svc_usermessage;
		msg.subtype				= msg_index;
		msg.usermessagesize		= msg_size;
		msg.usermessagename		= message_name;

		msg.buffer.Reset();

		return &msg.buffer;
	}

	void WriteMessageToBuffer( bool isvariablelength, bf_write *buffer )
	{
		// write the message type to the buffer
		buffer->WriteByte( msg.type );
		if ( msg.type == svc_usermessage )
		{
			buffer->WriteByte( msg.subtype );
		}

		if ( msg.filter->IsEntityMessage() )
		{
			buffer->WriteUBitLong( msg.filter->GetEntityIndex(), MAX_EDICT_BITS );
			buffer->WriteString( msg.filter->GetEntityMessageName() );
		}

		// If var-length user-message, record the data length
		if ( isvariablelength )
		{
			buffer->WriteByte( msg.buffer.GetNumBytesWritten() );
		}

		// Dump buffered data into message stream
		buffer->WriteBits( msg.buffer.m_pData, msg.buffer.GetNumBitsWritten() );
	}

	// Validates user message type and checks to see if it's variable length
	// returns true if variable length
	bool Mesage_CheckUserMessageLength( int bytesWritten )
	{
		Assert( msg.type == svc_usermessage );

		if ( msg.usermessagesize == -1 )
		{
			// Limit packet sizes
			if ( bytesWritten > MAX_USER_MSG_DATA )
			{
				Host_Error( "DLL_MessageEnd:  Refusing to send user message %s of %i bytes to client, user message size limit is %i bytes\n",
					msg.usermessagename, bytesWritten, MAX_USER_MSG_DATA );
				return false;
			}
			return true;
		}
		else if ( msg.usermessagesize != bytesWritten)
		{
			Sys_Error( "User Msg '%s': %d bytes written, expected %d\n", msg.usermessagename, bytesWritten, msg.usermessagesize );
			return false;
		}
		return false;
	}

	virtual void MessageEnd( void )
	{
		bool isvariablelengthmessage = false;

		if (!msg.started)
		{
			Sys_Error( "MESSAGE_END called with no active message\n" );
		}

		msg.started = false;

		// check to see if it's a valid message
		if ( msg.type == svc_usermessage )
		{
			isvariablelengthmessage = Mesage_CheckUserMessageLength( msg.buffer.GetNumBytesWritten() );
		}

		if ( msg.type == svc_entitymessage )
		{
			isvariablelengthmessage = true;

			if ( msg.buffer.GetNumBytesWritten() > 256 )
			{
				Sys_Error( "Entity Message to %i, %i bytes written (max is 256)\n",
					msg.filter->GetEntityIndex(),
					msg.buffer.GetNumBytesWritten() );
			}
		}

		if (isvariablelengthmessage)
		{
			// We must pad the message so it's byte aligned...
			int padBits = (msg.buffer.GetNumBitsWritten() & 0x7);
			if (padBits > 0)
			{
				padBits = 8 - padBits;
				msg.buffer.WriteUBitLong( 0, padBits );
			}
		}

		bool reliable = msg.filter->IsReliable();
		bool isinitmessage = msg.filter->IsInitMessage();

		if ( isinitmessage )
		{
			bf_write *pBuffer = &sv.signon;

			// This really only applies to the first player to connect, but that works in single player well enought
			if ( !sv.allowsignonwrites )
			{
				Con_DPrintf( "MessageEnd:  Init message being created after signon buffer has been transmitted\n" );
			}
			
			if ( msg.buffer.GetNumBitsWritten() > pBuffer->GetNumBitsLeft() )
			{
				Sys_Error( "MessageEnd:  Init message would overflow signon buffer!\n" );
				return;
			}

			WriteMessageToBuffer( isvariablelengthmessage, pBuffer );
		}
		else if ( msg.filter->IsBroadcastMessage() )
		{
			bf_write *pBuffer = NULL;

			if ( reliable )
			{
				pBuffer = &sv.reliable_datagram;
			}
			else
			{
				pBuffer = &sv.datagram;
			}

			if ( msg.buffer.GetNumBitsWritten() > pBuffer->GetNumBitsLeft() )
			{
				if ( reliable )
				{
					Msg( "MessageEnd:  Reliable broadcast message would overflow server reliable datagram!, Ignoring message\n" );
				}
				return;
			}

			WriteMessageToBuffer( isvariablelengthmessage, pBuffer );
		}
		else
		{
			// Now iterate the recipients and write the data into the appropriate buffer if possible
			int c = msg.filter->GetRecipientCount();
			int i;
			for ( i = 0; i < c; i++ )
			{
				int clientindex = msg.filter->GetRecipientIndex( i );
				if ( clientindex < 1 || clientindex > svs.maxclients )
				{
					Msg( "MessageEnd:  Recipient Filter for message type %i (entity message: %s, reliable: %s, broadcast: %s, init: %s) with bogus client index (%i) in list of %i clients\n", 
						msg.type, 
						msg.filter->IsEntityMessage() ? "yes" : "no",
						msg.filter->IsReliable() ? "yes" : "no",
						msg.filter->IsBroadcastMessage() ? "yes" : "no",
						msg.filter->IsInitMessage() ? "yes" : "no",
						clientindex,
						c );
					continue;
				}

				client_t *cl = &svs.clients[ clientindex - 1 ];
				// Never output to bots
				if ( cl->fakeclient )
					continue;

				if ( !cl->active && !cl->spawned )
					continue;

				bf_write *pBuffer = NULL;

				if ( reliable )
				{
					pBuffer = &cl->netchan.message;
				}
				else
				{
					pBuffer = &cl->datagram;
				}

				if ( !pBuffer->m_pData )
				{
					Con_DPrintf("Message sent to uninitialized buffer\n");
					continue;
				}

				if ( msg.buffer.GetNumBitsWritten() > pBuffer->GetNumBitsLeft() )
				{
					if ( reliable )
					{
						SV_DropClient( cl, false, "MessageEnd to %s for reliable message with no room in reliable buffer\n",
							cl->name );
					}
					continue;
				}

				WriteMessageToBuffer( isvariablelengthmessage, pBuffer );
			}
		}
	}

	/* single print to a specific client */
	virtual void ClientPrintf( edict_t *pEdict, const char *szMsg )
	{
		client_t	*client;
		int entnum = NUM_FOR_EDICT( pEdict );
		
		if (entnum < 1 || entnum > svs.maxclients)
		{
			Con_Printf ("tried to sprint to a non-client\n");
			return;
		}

		client = &svs.clients[ entnum-1];

		if ( client->fakeclient )
			return;

		client->netchan.message.WriteByte( svc_print );
		client->netchan.message.WriteString( (char *)szMsg );
	}

	virtual char		*Cmd_Args( void )
	{
		return ::Cmd_Args();
	}

	virtual char		*Cmd_Argv( int argc )
	{
		return ::Cmd_Argv(argc);
	}

	virtual int			Cmd_Argc( void )
	{
		return ::Cmd_Argc();
	}

	virtual void SetView(const edict_t *clientent, const edict_t *viewent)
	{
		client_t	*client;
		int clientnum;

		clientnum = NUM_FOR_EDICT( clientent );
		if (clientnum < 1 || clientnum > svs.maxclients)
			Host_Error ("DLL_SetView: not a client");

		client = &svs.clients[clientnum-1];
		client->pViewEntity = viewent;

		client->netchan.message.WriteByte ( svc_setview);
		client->netchan.message.WriteShort ( NUM_FOR_EDICT(viewent));
	}

	virtual float Time(void)
	{
		return Sys_FloatTime();
	}

	virtual void CrosshairAngle(const edict_t *clientent, float pitch, float yaw)
	{
		client_t	*client;
		int clientnum;

		clientnum = NUM_FOR_EDICT( clientent );
		if (clientnum < 1 || clientnum > svs.maxclients)
			Host_Error ("DLL_Crosshairangle: not a client");

		client = &svs.clients[clientnum-1];
		if (pitch > 180)
			pitch -= 360;
		if (pitch < -180)
			pitch += 360;
		if (yaw > 180)
			yaw -= 360;
		if (yaw < -180)
			yaw += 360;

		client->netchan.message.WriteByte ( svc_crosshairangle);
		client->netchan.message.WriteChar ( pitch * 5);
		client->netchan.message.WriteChar ( yaw * 5);
	}


	virtual void        GetGameDir( char *szGetGameDir )
	{
		COM_GetGameDir(szGetGameDir);
	}		

	
	virtual byte * LoadFileForMe(const char *filename, int *pLength)
	{
		return COM_LoadFileForMe(filename, pLength);
	}

	virtual void FreeFile (byte *buffer)
	{
		COM_FreeFile(buffer);
	}

	virtual int CompareFileTime(char *filename1, char *filename2, int *iCompare)
	{
		return COM_CompareFileTime(filename1, filename2, iCompare);
	}

	// For use with FAKE CLIENTS
	virtual edict_t* CreateFakeClient( const char *netname )
	{
		client_t *fakeclient=0;
		edict_t *ent;
		int i;

		for ( i = 0; i < svs.maxclients; i++ )
		{
			fakeclient = &svs.clients[i];
			if (!fakeclient->active && !fakeclient->spawned && !fakeclient->connected)
				break;
		}

		if ( i == svs.maxclients )
			return NULL;		// server is full

		ent         = EDICT_NUM( i + 1 );

		// Wipe the client structure
		SV_ClearClientStructure(fakeclient);
		
		// Wipe the remainder of the structure.
		assert( !fakeclient->frames );

		fakeclient->frames = (client_frame_t *)new client_frame_t[ SV_UPDATE_BACKUP ];
		fakeclient->numframes = SV_UPDATE_BACKUP;
		memset( fakeclient->frames, 0, SV_UPDATE_BACKUP * sizeof( client_frame_t ) );

		// Set up client structure.
		Q_strncpy( fakeclient->name, netname, 32 );
		fakeclient->active = true;
		fakeclient->spawned = true;
		fakeclient->connected = true;
		fakeclient->edict = ent;
		fakeclient->fakeclient = true;

		Info_SetValueForKey (fakeclient->userinfo, "name", fakeclient->name, MAX_INFO_STRING);
		fakeclient->sendinfo = true;

		SV_ExtractFromUserinfo (fakeclient);  // parse some info from the info strings

		return ent;

	}


	/*
	==============
	GetInfoKeyBuffer
	==============
	*/
	virtual char* GetInfoKeyBuffer( edict_t *e )
	{
		int		e1;
		char	*value;

		if ( !e )  // Passing in NULL is equivalent a request for the localinfo buffer
		{
			value = "";
			return value;
		}
		
		// otherwise, determine which buffer the game .dll wishes to examine.
		e1 = NUM_FOR_EDICT(e);

		if ( e1 > 0 && e1 <= MAX_CLIENTS )
		{
			value = svs.clients[e1-1].userinfo;
		}
		else
		{
			value = "";
		}

		return value;
	}


	virtual char* InfoKeyValue( char *infobuffer, const char *key )
	{
		return (char *)Info_ValueForKey( infobuffer, key );
	}


	virtual void SetClientKeyValue( int clientIndex, char *infobuffer, const char *key, const char *value )
	{
		client_t	*pClient;

		if ( clientIndex < 1 || clientIndex > svs.maxclients )
		{
			return;
		}

		Info_SetValueForKey( infobuffer, key, value, MAX_INFO_STRING );

		// Make sure this gets updated
		pClient = svs.clients + clientIndex - 1;
		pClient->sendinfo = true;
	}


	virtual char *COM_ParseFile(char *data, char *token)
	{
		return ::COM_ParseFile(data, token);
	}

	virtual byte *COM_LoadFile (const char *path, int usehunk, int *pLength)
	{
		return ::COM_LoadFile(path, usehunk, pLength);
	}

	virtual void AddOriginToPVS( const Vector& origin )
	{
		::SV_AddOriginToPVS(origin);
	}

	virtual void ResetPVS( byte* pvs )
	{
		::SV_ResetPVS(pvs);
	}

	virtual void		SetAreaPortalState( int portalNumber, int isOpen )
	{
		CM_SetAreaPortalState(portalNumber, isOpen);
	}

	//-----------------------------------------------------------------------------
	// Purpose: Sends a temp entity to the client ( follows the format of the original MESSAGE_BEGIN stuff from HL1
	// Input  : msg_dest - 
	//			delay - 
	//			*origin - 
	//			*recipient - 
	//			*pSender - 
	//			*pST - 
	//			classID - 
	//-----------------------------------------------------------------------------
	virtual void PlaybackTempEntity( IRecipientFilter& filter, float delay, const void *pSender, const SendTable *pST, int classID  )
	{
		client_t *cl;
		int slot;
		CEventState *es;
		CEventInfo *ei=0;
		int bestslot;
		int j;
		bool send_reliable=false;

		VPROF( "PlaybackTempEntity" );
		
		// Make this start at 1
		classID = classID + 1;

		if ( filter.IsEntityMessage() )
		{
			// Can't send these to entities
			Assert( 0 );
			return;
		}

		send_reliable = filter.IsReliable();

		int c = filter.GetRecipientCount();

		for ( slot = 0; slot < c; slot++ )
		{
			int index = filter.GetRecipientIndex( slot );
			if ( index < 1 || index > svs.maxclients )
				continue;

			cl = &svs.clients[ index - 1 ];

			if ( cl->fakeclient )
				continue;

			if ( !( cl->active && cl->spawned ) )
				continue;

			// Reliable event!!
			if ( send_reliable )
			{
				// A reliable event must pass in origin and angles expressly.  They will not be taken from the invoker in any way
				// Also, you can't send velocity in a reliable event.
				PlayReliableTempEntity( &cl->netchan.message, delay, pSender, pST, classID );
				continue;
			}

			es = &cl->events;

			bestslot = -1;

			for ( j = 0; j < MAX_EVENT_QUEUE; j++ )
			{
				ei = &es->ei[ j ];
		
				if ( ei->index == 0 )
				{
					// Found an empty slot
					bestslot = j;
					break;
				}
			}
				
			// No slot found for this player, oh well
			if ( bestslot == -1 )
			{
				continue;
			}

			ei->index			= classID;
			ei->fire_time		= delay;
			
			// Encode now!
			memset( ei->data, 0, sizeof( ei->data ) );
			bf_write writeBuf( "PlaybackTempEntity", ei->data, MAX_EVENT_DATA );
			if( !SendTable_Encode( pST, pSender, &writeBuf, NULL, classID, NULL, true ) )
			{
				Host_Error( "PlaybackTempEntity: SendTable_Encode returned false (ent %d), overflow? %i\n", classID, writeBuf.IsOverflowed() ? 1 : 0 );
			}
			ei->bits = writeBuf.GetNumBitsWritten();

		}
	}
	
	virtual int			CheckAreasConnected( int area1, int area2 )
	{
		return CM_AreasConnected(area1, area2);
	}


	//-----------------------------------------------------------------------------
	// Purpose: 
	// Input  : *origin - 
	//			*bits - 
	// Output : void
	//-----------------------------------------------------------------------------
	virtual int GetArea( const Vector& origin )
	{
		return CM_LeafArea( CM_PointLeafnum( origin ) );
	}

	virtual void GetAreaBits( int area, unsigned char *bits )
	{
		CM_WriteAreaBits( bits, area );
	}

	virtual bool GetAreaPortalPlane( Vector const &vViewOrigin, int portalKey, VPlane *pPlane )
	{
		return CM_GetAreaPortalPlane( vViewOrigin, portalKey, pPlane );
	}

	client_textmessage_t *TextMessageGet( const char *pName )
	{
		return ::TextMessageGet( pName );
	}

	void LogPrint(const char * msg)
	{
		g_Log.Print( msg );
	}

private:
	//-----------------------------------------------------------------------------
	// Purpose: Sends a temp entity to the client, using the reliable data stream
	// Input  : *cl - 
	//			delay - 
	//			*pSender - 
	//			*pST - 
	//			classID - 
	//-----------------------------------------------------------------------------
	void PlayReliableTempEntity( bf_write *destbuffer, float delay, const void *pSender, const SendTable *pST, int classID )
	{
		unsigned char data[ 1024 ];
		unsigned char raw[ MAX_EVENT_DATA ];
		int numbits;

		// Write event data into temp buffer
		bf_write msg( "PlayReliableTempEntity->msg", data, 1024 );

		msg.WriteByte( svc_event_reliable );

		// Write event object ID
		msg.WriteBitLong( classID, sv.serverclassbits, false );

		memset( raw, 0, sizeof( raw ) );

		// Encode event into separate buffer
		bf_write writeBuf( "PlayReliableTempEntity->writeBuf", raw, MAX_EVENT_DATA );
		if( !SendTable_Encode( pST, pSender, &writeBuf, NULL, -1, NULL, true ) )
		{
			Host_Error( "PlayReliableTempEntity: SendTable_Encode %s returned false, overflow? %i\n", pST->GetName(), writeBuf.IsOverflowed() ? 1 : 0 );
		}
		numbits = writeBuf.GetNumBitsWritten();

		Assert( numbits <= (1<<EVENT_DATA_LEN_BITS) );

		// Copy to real output
		msg.WriteBitLong( numbits, EVENT_DATA_LEN_BITS, false );
		msg.WriteBits( raw, numbits );

		// Delay
		if ( delay == 0.0 )
		{
			msg.WriteOneBit( 0 );
		}
		else
		{
			msg.WriteOneBit( 1 );
			msg.WriteBitLong( (int)(delay * 100.0), 16, false );
		}

		// Place into client's reliable queue
		destbuffer->WriteBits( msg.GetData(), msg.GetNumBitsWritten() );
	}

	virtual void ApplyTerrainMod( TerrainModType type, CTerrainModParams const &params )
	{
		ITerrainMod *pMod = SetupTerrainMod( type, params );
		
		// Apply to the terrain physics. Rendering changes are all done on the client.
		CM_ApplyTerrainMod( pMod );
	}

	// HACKHACK: Save/restore wrapper - Move this to a different interface
	virtual bool LoadGameState( char const *pMapName, bool createPlayers )
	{
#ifndef SWDS
		return saverestore->LoadGameState( pMapName, createPlayers ) != 0;
#else
		return 0;
#endif
	}
	
	virtual void LoadAdjacentEnts( const char *pOldLevel, const char *pLandmarkName )
	{
#ifndef SWDS
		saverestore->LoadAdjacentEnts( pOldLevel, pLandmarkName );
#endif
	}

	virtual void ClearSaveDir()
	{
#ifndef SWDS
		saverestore->RequestClearSaveDir();
#endif
	}

	virtual const char* GetMapEntitiesString()
	{
		return CM_EntityString();
	}
};

// Expose CVEngineServer to the game DLL.
static CVEngineServer	g_VEngineServer;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CVEngineServer, IVEngineServer, INTERFACEVERSION_VENGINESERVER, g_VEngineServer);


// Expose CVEngineServer to the engine.
IVEngineServer *g_pVEngineServer = &g_VEngineServer;
