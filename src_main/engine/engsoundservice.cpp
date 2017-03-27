//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose: Engine implementation of services required by the audio subsystem
//
// $NoKeywords: $
//=============================================================================

#include "winquake.h"
#include "quakedef.h"
#include "SoundService.h"
#include "zone.h"
#include "cdll_engine_int.h"
#include "gl_model_private.h"
#include "icliententity.h"
#include "icliententitylist.h"
#include "mouthinfo.h"
#include "host.h"
#include "vstdlib/random.h"
#include "vstdlib/icommandline.h"
#include "igame.h"


class CEngineSoundServices : public ISoundServices
{
public:
	virtual int CheckParm( const char *pszParm )
	{
		return CommandLine()->FindParm( pszParm );
	}

	virtual void ConSafePrint( const char *pszFormat)
	{
		Con_SafePrintf(pszFormat);
	}

	virtual void *LevelAlloc( int nBytes, const char *pszTag )
	{
		return Hunk_AllocName(nBytes, pszTag);
	}

	virtual bool IsDirectSoundSupported()
	{
		return (Win32AtLeastV4 != 0);
	}

	virtual bool QuerySetting( const char *pszCvar, bool bDefault )
	{
		// parse config.cfg for the cvar and return true or false depending on its
		// value
		char szFile[ MAX_OSPATH ];
		char *pbuffer, *pstart;
		bool bResult = bDefault;

		Q_snprintf( szFile, sizeof( szFile ), "cfg/config.cfg" );

		// Read them in from WONCOMM.LST
		pbuffer = (char*)COM_LoadFile( szFile, 5, NULL ); // Use malloc
		if ( !pbuffer )
		{
			return bDefault;
		}

		pstart = pbuffer;

		while ( 1 )
		{
			pstart = COM_Parse( pstart );
			
			if ( strlen(com_token) <= 0)
				break;

			if ( !stricmp( com_token, "bind" ) )
			{
				// Parse keyname and binding
				pstart = COM_Parse( pstart );
				pstart = COM_Parse( pstart );
				continue;
			}
			else if ( com_token[0] == '+' )  // It's +mlook or +jlook
			{	
				continue;
			}
			else // it's a cvar
			{
				if ( !stricmp( com_token, pszCvar ) )
				{
					// Find the value
					pstart = COM_Parse( pstart );

					bResult = atoi( com_token ) ? true : false;
					break;
				}
				else
				{
					// Throw away the value
					pstart = COM_Parse( pstart );
				}
			}
		}

		free( pbuffer );
		return bResult;
	}

	virtual void OnExtraUpdate()
	{
		if ( g_ClientDLL && game && game->IsActiveApp() )
		{
			g_ClientDLL->IN_Accumulate();
		}
	}

	virtual bool GetSoundSpatialization( int entIndex, SpatializationInfo_t& info )
	{
		// Entity has been deleted
		IClientEntity *pClientEntity = entitylist->GetClientEntity( entIndex );
		if ( !pClientEntity )
		{
			// FIXME:  Should this assert?
			return false;
		}
			
		return pClientEntity->GetSoundSpatialization( info );
	}

	virtual long RandomLong( long lLow, long lHigh )
	{
		return ::RandomInt( lLow, lHigh );
	}

	virtual double GetClientTime()
	{
		return cl.gettime();
	}

	// Filtered local time
	virtual double GetHostTime()
	{
		return host_time;
	}

	virtual int GetViewEntity()
	{
		return cl.viewentity;
	}

	virtual double GetHostFrametime()
	{
		return host_frametime;
	}

	virtual int GetServerCount()
	{
		return cl.servercount;
	}

	virtual bool IsPlayer( SoundSource source )
	{
		return ( source == cl.playernum + 1 );
	}

	virtual float GetRealTime()
	{
		return Sys_FloatTime();
	}

	virtual void OnChangeVoiceStatus( int entity, bool status)
	{
		ClientDLL_VoiceStatus(entity, status);
	}

};


static CEngineSoundServices g_EngineSoundServices;
ISoundServices *g_pSoundServices = &g_EngineSoundServices;
