// vmf_tweak.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "chunkfile.h"
#include "utllinkedlist.h"
#include <stdlib.h>


char *CopyString( char const *pStr );


class CKeyValue
{
public:
	char	*m_pKey;
	char	*m_pValue;
};


class CChunk
{
public:
	// Returns true if the chunk has the specified key and if its value
	// is equal to pValueStr (case-insensitive).
	bool			CompareKey( char const *pKeyName, char const *pValueStr );

	// Look for a key by name.
	CKeyValue*		FindKey( char const *pKeyName );

	// Find a key by name, and replace any occurences with the new name.
	void			RenameKey( const char *szOldName, const char *szNewName );

public:

	char										*m_pChunkName;
	CUtlLinkedList<CKeyValue*, unsigned short>	m_Keys;
	CUtlLinkedList<CChunk*, unsigned short>		m_Chunks;
};


CChunk* ParseChunk( char const *pChunkName, bool bOnlyOne );


CChunk		*g_pCurChunk = 0;
CChunkFile	*g_pChunkFile = 0;
int g_DotCounter = 0;



// --------------------------------------------------------------------------------- //
// CChunk implementation.
// --------------------------------------------------------------------------------- //

bool CChunk::CompareKey( char const *pKeyName, char const *pValueStr )
{
	CKeyValue *pKey = FindKey( pKeyName );

	if( pKey && stricmp( pKey->m_pValue, pValueStr ) == 0 )
		return true;
	else
		return false;
}	


CKeyValue* CChunk::FindKey( char const *pKeyName )
{
	for( unsigned short i=m_Keys.Head(); i != m_Keys.InvalidIndex(); i=m_Keys.Next(i) )
	{
		CKeyValue *pKey = m_Keys[i];

		if( stricmp( pKey->m_pKey, pKeyName ) == 0 )
			return pKey;
	}

	return NULL;
}


void CChunk::RenameKey( const char *szOldName, const char *szNewName )
{
	if ((!szOldName) || (!szNewName))
		return;

	CKeyValue *pKey = FindKey( szOldName );
	if ( pKey )
	{
		delete pKey->m_pKey;
		pKey->m_pKey = CopyString( szNewName );
	}
}


// --------------------------------------------------------------------------------- //
// Util functions.
// --------------------------------------------------------------------------------- //
char *CopyString( char const *pStr )
{
	char *pRet = new char[ strlen(pStr) + 1 ];
	strcpy( pRet, pStr );
	return pRet;
}

ChunkFileResult_t MyDefaultHandler( CChunkFile *pFile, void *pData, char const *pChunkName )
{
	CChunk *pChunk = ParseChunk( pChunkName, true );
	g_pCurChunk->m_Chunks.AddToTail( pChunk );
	return ChunkFile_Ok;
}


ChunkFileResult_t MyKeyHandler( const char *szKey, const char *szValue, void *pData )
{
	// Add the key to the current chunk.
	CKeyValue *pKey = new CKeyValue;
	
	pKey->m_pKey   = CopyString( szKey );
	pKey->m_pValue = CopyString( szValue );

	g_pCurChunk->m_Keys.AddToTail( pKey );
	return ChunkFile_Ok;
}


CChunk* ParseChunk( char const *pChunkName, bool bOnlyOne )
{
	// Add the new chunk.
	CChunk *pChunk = new CChunk;
	pChunk->m_pChunkName = CopyString( pChunkName );

	// Parse it out..
	CChunk *pOldChunk = g_pCurChunk;
	g_pCurChunk = pChunk;

	//if( ++g_DotCounter % 16 == 0 )
	//	printf( "." );

	while( 1 )
	{
		if( g_pChunkFile->ReadChunk( MyKeyHandler ) != ChunkFile_Ok )
			break;
	
		if( bOnlyOne )
			break;
	}

	g_pCurChunk = pOldChunk;
	return pChunk;
}


CChunk* ReadChunkFile( char const *pInFilename )
{
	CChunkFile chunkFile;

	if( chunkFile.Open( pInFilename, ChunkFile_Read ) != ChunkFile_Ok )
	{
		printf( "Error opening chunk file %s for reading.\n", pInFilename );
		return NULL;
	}

	printf( "Reading.." );
	chunkFile.SetDefaultChunkHandler( MyDefaultHandler, 0 );
	g_pChunkFile = &chunkFile;
	
	CChunk *pRet = ParseChunk( "***ROOT***", false );
	printf( "\n\n" );

	return pRet;
}


void WriteChunks_R( CChunkFile *pFile, CChunk *pChunk, bool bRoot )
{
	if( !bRoot )
	{
		pFile->BeginChunk( pChunk->m_pChunkName );
	}

	//if( ++g_DotCounter % 16 == 0 )
	//	printf( "." );

	// Write keys.
	for( unsigned short i=pChunk->m_Keys.Head(); i != pChunk->m_Keys.InvalidIndex(); i = pChunk->m_Keys.Next( i ) )
	{
		CKeyValue *pKey = pChunk->m_Keys[i];
		pFile->WriteKeyValue( pKey->m_pKey, pKey->m_pValue );
	}

	// Write subchunks.
	for( i=pChunk->m_Chunks.Head(); i != pChunk->m_Chunks.InvalidIndex(); i = pChunk->m_Chunks.Next( i ) )
	{
		CChunk *pChild = pChunk->m_Chunks[i];
		WriteChunks_R( pFile, pChild, false );
	}

	if( !bRoot )
	{
		pFile->EndChunk();
	}
}


bool WriteChunkFile( char const *pOutFilename, CChunk *pRoot )
{
	CChunkFile chunkFile;

	if( chunkFile.Open( pOutFilename, ChunkFile_Write ) != ChunkFile_Ok )
	{
		printf( "Error opening chunk file %s for writing.\n", pOutFilename );
		return false;
	}

	printf( "Writing.." );
	WriteChunks_R( &chunkFile, pRoot, true );
	printf( "\n\n" );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Updates maps that are still using the "angles" key to indicate a
//			direction of motion (or anything other than an orientation).
//-----------------------------------------------------------------------------
void ScanFuncWaterMoveDir( CChunk *pChunk )
{
	if ( stricmp( pChunk->m_pChunkName, "entity" ) == 0 )
	{
		if ( pChunk->CompareKey( "classname", "func_water" ) )
		{
			pChunk->RenameKey("angles", "movedir");
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Updates maps that are still using the "angles" key to indicate a
//			direction of motion (or anything other than an orientation).
//-----------------------------------------------------------------------------
void ScanMoveDir( CChunk *pChunk )
{
	if ( stricmp( pChunk->m_pChunkName, "entity" ) == 0 )
	{
		if ( pChunk->CompareKey( "classname", "env_blood" ) )
		{
			pChunk->RenameKey("angles", "spraydir");
		}
		else if ( pChunk->CompareKey( "classname", "func_button" ) )
		{
			pChunk->RenameKey("angles", "movedir");
		}
		else if ( pChunk->CompareKey( "classname", "func_door" ) )
		{
			pChunk->RenameKey("angles", "movedir");
		}
		else if ( pChunk->CompareKey( "classname", "func_door_weldable" ) )
		{
			pChunk->RenameKey("angles", "movedir");
		}
		else if ( pChunk->CompareKey( "classname", "func_movelinear" ) )
		{
			pChunk->RenameKey("angles", "movedir");
		}
		else if ( pChunk->CompareKey( "classname", "momentary_door" ) )
		{
			pChunk->RenameKey("angles", "movedir");
		}
		else if ( pChunk->CompareKey( "classname", "func_conveyor" ) )
		{
			pChunk->RenameKey("angles", "movedir");
		}
		else if ( pChunk->CompareKey( "classname", "trigger_push" ) )
		{
			pChunk->RenameKey("angles", "pushdir");
		}
	}
}


void ScanMomentaryRotButton( CChunk *pChunk )
{
// Old spawnflags
#define SF_MOMENTARY_DOOR			1
#define SF_MOMENTARY_NOT_USABLE		2
#define SF_MOMENTARY_AUTO_RETURN	16

// New spawnflags
#define SF_ROTBUTTON_NOTSOLID		1
#define SF_BUTTON_TOGGLE			32		// Button stays pushed until reactivated
#define SF_DOOR_ROTATE_Z			64
#define SF_DOOR_ROTATE_X			128
#define SF_BUTTON_USE_ACTIVATES		1024	// Button fires when used.
#define SF_BUTTON_LOCKED			2048	// Whether the button is initially locked.


	if ( stricmp( pChunk->m_pChunkName, "entity" ) == 0 )
	{
		if ( pChunk->CompareKey( "classname", "momentary_rot_button" ) )
		{
			CKeyValue *pKey = pChunk->FindKey( "spawnflags" );
			if ( pKey )
			{
				// Fixup the spawnflags
				int nOldSpawnFlags = atoi( pKey->m_pValue );

				// Not all spawnflags are being tweaked, so copy the old ones over.
				int nNewSpawnFlags = nOldSpawnFlags;

				// Migrate the 'Not usable' spawnflag. Do this first because the 'Door hack'
				// spawnflag takes precedence with regards to usability.
				if (nOldSpawnFlags & SF_MOMENTARY_NOT_USABLE)
				{
					nNewSpawnFlags &= ~SF_BUTTON_USE_ACTIVATES;
				}
				else
				{
					nNewSpawnFlags |= SF_BUTTON_USE_ACTIVATES;
				}

				// Migrate the 'Door hack' spawnflag.
				if (nOldSpawnFlags & SF_MOMENTARY_DOOR)
				{
					// Door hacked momentaries are solid and not usable
					nNewSpawnFlags &= ~SF_ROTBUTTON_NOTSOLID;
					nNewSpawnFlags &= ~SF_BUTTON_USE_ACTIVATES;
				}
				else
				{
					// Otherwise, momentaries are not solid and usable
					nNewSpawnFlags |= SF_ROTBUTTON_NOTSOLID;
					nNewSpawnFlags |= SF_BUTTON_USE_ACTIVATES;
				}

				// Migrate the 'Auto return' spawnflag.
				if (nOldSpawnFlags & SF_MOMENTARY_AUTO_RETURN)
				{
					nNewSpawnFlags &= ~SF_BUTTON_TOGGLE;
				}
				else
				{
					nNewSpawnFlags |= SF_BUTTON_TOGGLE;
				}

				char str[256];
				sprintf( str, "%u", (unsigned)nNewSpawnFlags );
				pKey->m_pValue = CopyString( str );
			}
		}
	}
}


void ScanRopeSlack( CChunk *pChunk )
{
	if( stricmp( pChunk->m_pChunkName, "entity" ) == 0 )
	{
		if( pChunk->CompareKey( "classname", "keyframe_rope" ) ||
			pChunk->CompareKey( "classname", "move_rope" ) )
		{
			CKeyValue *pKey = pChunk->FindKey( "slack" );
			if( pKey )
			{
				// Subtract 100 from all the Slack properties.
				float flCur = (float)atof( pKey->m_pValue );
				char str[256];
				sprintf( str, "%f", flCur + 100 );
				pKey->m_pValue = CopyString( str );
			}
		}
	}
}

char *GetSoundName( bool bMoveOrStop, int iValue )
{

// set the door's "in-motion" sound

	if ( bMoveOrStop == false )
	{
		switch (iValue)
		{
		case	0:
			return ("common/null.wav");
		case	1:
			return ("doors/doormove1.wav");
		case	2:
			return ("doors/doormove2.wav");
		case	3:
			return ("doors/doormove3.wav");
		case	4:
			return ("doors/doormove4.wav");
		case	5:
			return ("doors/doormove5.wav");
		case	6:
			return ("doors/doormove6.wav");
		case	7:
			return ("doors/doormove7.wav");
		case	8:
			return ("doors/doormove8.wav");
		case	9:
			return ("doors/doormove9.wav");
		case	10:
			return ("doors/doormove10.wav");
		default:
			return ("common/null.wav");
		}
	}
	else
	{
		// set the door's 'reached destination' stop sound
		switch (iValue)
		{
		case	0:
			return ("common/null.wav");
		case	1:
			return ("doors/doorstop1.wav");
		case	2:
			return ("doors/doorstop2.wav");
		case	3:
			return ("doors/doorstop3.wav");
		case	4:
			return ("doors/doorstop4.wav");
		case	5:
			return ("doors/doorstop5.wav");
		case	6:
			return ("doors/doorstop6.wav");
		case	7:
			return ("doors/doorstop7.wav");
		case	8:
			return ("doors/doorstop8.wav");
		default:
			return ("common/null.wav");
		}
	}
}

char *GetTrainSoundName( bool bMoveOrStop, int iValue )
{
// set the plat's "in-motion" sound
	if ( bMoveOrStop == false )
	{
		switch (iValue)
		{
		case	0:
			return ("common/null.wav");
			break;
		case	1:
			return ("plats/bigmove1.wav");
			break;
		case	2:
			return ("plats/bigmove2.wav");
			break;
		case	3:
			return ("plats/elevmove1.wav");
			break;
		case	4:
			return ("plats/elevmove2.wav");
			break;
		case	5:
			return ("plats/elevmove3.wav");
			break;
		case	6:
			return ("plats/freightmove1.wav");
			break;
		case	7:
			return ("plats/freightmove2.wav");
			break;
		case	8:
			return ("plats/heavymove1.wav");
			break;
		case	9:
			return ("plats/rackmove1.wav");
			break;
		case	10:
			return ("plats/railmove1.wav");
			break;
		case	11:
			return ("plats/squeekmove1.wav");
			break;
		case	12:
			return ("plats/talkmove1.wav");
			break;
		case	13:
			return ("plats/talkmove2.wav");
			break;
		default:
			return ("common/null.wav");
			break;
		}
	}

	else
	{
		// set the plat's 'reached destination' stop sound
		switch ( iValue)
		{
		case	0:
			return ("common/null.wav");
			break;
		case	1:
			return ("plats/bigstop1.wav");
			break;
		case	2:
			return ("plats/bigstop2.wav");
			break;
		case	3:
			return ("plats/freightstop1.wav");
			break;
		case	4:
			return ("plats/heavystop2.wav");
			break;
		case	5:
			return ("plats/rackstop1.wav");
			break;
		case	6:
			return ("plats/railstop1.wav");
			break;
		case	7:
			return ("plats/squeekstop1.wav");
			break;
		case	8:
			return ("plats/talkstop1.wav");
			break;

		default:
			return("common/null.wav");
			break;
		}
	}
}

char *GetTrainMoveSound( int iValue )
{
	switch( iValue )
	{
		case 1: return("plats/ttrain1.wav");
		case 2: return("plats/ttrain2.wav");
		case 3: return("plats/ttrain3.wav");
		case 4: return("plats/ttrain4.wav");
		case 5: return("plats/ttrain6.wav");
		case 6: return("plats/ttrain7.wav");
	}

	return NULL;
}

void ScanHL1( CChunk *pChunk )
{
	if( stricmp( pChunk->m_pChunkName, "entity" ) == 0 )
	{
		/*if( pChunk->CompareKey( "classname", "env_beam" ) )
		{
			CKeyValue *pKey = pChunk->FindKey( "BoltWidth" );
			if( pKey )
			{
				//scale width by 0.1
				float flCur = (float)atof( pKey->m_pValue );
				char str[256];
				sprintf( str, "%.1f", flCur * 0.1 );
				pKey->m_pValue = CopyString( str );
			}
			pKey = pChunk->FindKey( "NoiseAmplitude" );
			if( pKey )
			{
				//scale amplitude by 0.16
				float flCur = (float)atof( pKey->m_pValue );
				char str[256];
				flCur = fmod(flCur,255);
				sprintf( str, "%.1f", flCur * 0.16 );
				pKey->m_pValue = CopyString( str );
			}
		}

		//Find info_player_start and move them down 36 units.
		if( pChunk->CompareKey( "classname", "info_player_start" ) )
		{
			CKeyValue *pKey = pChunk->FindKey( "origin" );
			if( pKey )
			{
				//scale width by 0.1
				const char *pOrigin = pKey->m_pValue;
				double	v1, v2, v3;

				v1 = v2 = v3 = 0;
				sscanf ( pOrigin, "%lf %lf %lf", &v1, &v2, &v3);
			
				char str[256];
				sprintf( str, "%lf %lf %lf", v1, v2, ( v3 - 36 ) );
				pKey->m_pValue = CopyString( str );
			}
		}*/

		if( pChunk->CompareKey( "classname", "env_laser" ) )
		{/*
			CKeyValue *pKey = pChunk->FindKey( "BoltWidth" );
			if( pKey )
			{
				//scale width by 0.1
				float flCur = (float)atof( pKey->m_pValue );
				char str[256];
				sprintf( str, "%.1f", flCur * 0.1 );
				pKey->m_pValue = CopyString( str );
			}
			pKey = pChunk->FindKey( "NoiseAmplitude" );
			if( pKey )
			{
				//scale amplitude by 0.16
				float flCur = (float)atof( pKey->m_pValue );
				char str[256];
				flCur = fmod(flCur,255);
				sprintf( str, "%.1f", flCur * 0.16 );
				pKey->m_pValue = CopyString( str );
			}*/
		}

		if( pChunk->CompareKey( "classname", "env_sound" ) )
		{
			CKeyValue *pKey = pChunk->FindKey( "roomtype" );
		
			if( pKey )
			{
				// Find the original numeric roomtype value
				int iCur = atoi( pKey->m_pValue );

				// Change the key name from "roomtype" to "soundscape"
				delete pKey->m_pKey;
				pKey->m_pKey = CopyString( "soundscape" );

				// Delete the old value
				delete pKey->m_pValue;

				// Throw in the converted value
				switch ( iCur )
				{
				case 0:
					pKey->m_pValue = CopyString( "Normal (off)" );		break;
				case 1:
					pKey->m_pValue = CopyString( "Generic" );			break;
				case 2:
					pKey->m_pValue = CopyString( "Metal Small" );		break;
				case 3:
					pKey->m_pValue = CopyString( "Metal Medium" );		break;
				case 4:
					pKey->m_pValue = CopyString( "Metal Large" );		break;
				case 5:
					pKey->m_pValue = CopyString( "Tunnel Small" );		break;
				case 6:
					pKey->m_pValue = CopyString( "Tunnel Medium" );		break;
				case 7:
					pKey->m_pValue = CopyString( "Tunnel Large" );		break;
				case 8:
					pKey->m_pValue = CopyString( "Chamber Small" );		break;
				case 9:
					pKey->m_pValue = CopyString( "Chamber Medium" );		break;
				case 10:
					pKey->m_pValue = CopyString( "Chamber Large" );		break;
				case 11:
					pKey->m_pValue = CopyString( "Bright Small" );		break;
				case 12:
					pKey->m_pValue = CopyString( "Bright Medium" );		break;
				case 13:
					pKey->m_pValue = CopyString( "Bright Large" );		break;
				case 14:
					pKey->m_pValue = CopyString( "Water 1" );			break;
				case 15:
					pKey->m_pValue = CopyString( "Water 2" );			break;
				case 16:
					pKey->m_pValue = CopyString( "Water 3" );			break;
				case 17:
					pKey->m_pValue = CopyString( "Concrete Small" );	break;
				case 18:
					pKey->m_pValue = CopyString( "Concrete Medium" );	break;
				case 19:
					pKey->m_pValue = CopyString( "Concrete Large" );	break;
				case 20:
					pKey->m_pValue = CopyString( "Big 1" );				break;
				case 21:
					pKey->m_pValue = CopyString( "Big 2" );				break;
				case 22:
					pKey->m_pValue = CopyString( "Big 3" );				break;
				case 23:
					pKey->m_pValue = CopyString( "Cavern Small" );		break;
				case 24:
					pKey->m_pValue = CopyString( "Cavern Medium" );		break;
				case 25:
					pKey->m_pValue = CopyString( "Cavern Large" );		break;
				case 26:
					pKey->m_pValue = CopyString( "Weirdo 1" );			break;
				case 27:
					pKey->m_pValue = CopyString( "Weirdo 2" );			break;
				case 28:
					pKey->m_pValue = CopyString( "Weirdo 3" );			break;
				default:
					pKey->m_pValue = CopyString( "Normal (off)" );		break;
				}
			}
		}

		if( pChunk->CompareKey( "classname", "func_tracktrain" ) )
		{
/*			CKeyValue *pKey = pChunk->FindKey( "spawnflags" );
		
			if( pKey )
			{
				int iCur = atoi( pKey->m_pValue );

				iCur |= 128;

				char str[256];
				sprintf( str, "%d", iCur );
				pKey->m_pValue = CopyString( str );
			}

			pKey = pChunk->FindKey( "movesnd" );

			if( pKey )
			{
				//scale width by 0.1
				int iCur = atoi( pKey->m_pValue );

				char str[256];
				sprintf( str, "%s", GetTrainSoundName( false, iCur ) );
				pKey->m_pValue = CopyString( str );

				delete pKey->m_pKey;

				pKey->m_pKey = CopyString( "StartSound" );
			}

			pKey = pChunk->FindKey( "stopsnd" );

			if( pKey )
			{
				//scale width by 0.1
				int iCur = atoi( pKey->m_pValue );

				char str[256];
				sprintf( str, "%s", GetTrainSoundName( true, iCur ) );
				pKey->m_pValue = CopyString( str );

				delete pKey->m_pKey;

				pKey->m_pKey = CopyString( "StopSound" );
			}

			pKey = pChunk->FindKey( "sounds" );

			if( pKey )
			{
				//scale width by 0.1
				int iCur = atoi( pKey->m_pValue );

				char str[256];
				sprintf( str, "%s", GetTrainMoveSound( iCur ) );
				pKey->m_pValue = CopyString( str );

				delete pKey->m_pKey;

				pKey->m_pKey = CopyString( "MoveSound" );
			}
*/		}

		if( pChunk->CompareKey( "classname", "func_door" ) || pChunk->CompareKey( "classname", "func_door_rotating" ))
		{
/*			CKeyValue *pKey = pChunk->FindKey( "movesnd" );

			if( pKey )
			{
				int iType = atoi( pKey->m_pValue );

				if ( iType == 0 )
				{
					pKey = pChunk->FindKey( "stopsnd" );

					if( pKey )
					{
						iType = atoi( pKey->m_pValue );

						if ( iType == 0 )
						{
							pKey = pChunk->FindKey( "spawnflags" );

							if( pKey )
							{
								int iCur = atoi( pKey->m_pValue );

								iCur |= 4096;

								char str[256];
								sprintf( str, "%d", iCur );
								pKey->m_pValue = CopyString( str );
							}
						}
					}

				}
			}

			pKey = pChunk->FindKey( "movesnd" );

			if( pKey )
			{
				//scale width by 0.1
				int iCur = atoi( pKey->m_pValue );

				char str[256];
				sprintf( str, "%s", GetSoundName( false, iCur ) );
				pKey->m_pValue = CopyString( str );

				strcpy( pKey->m_pKey, "noise1" );
			}

			pKey = pChunk->FindKey( "stopsnd" );

			if( pKey )
			{
				//scale width by 0.1
				int iCur = atoi( pKey->m_pValue );

				char str[256];
				sprintf( str, "%s", GetSoundName( true, iCur ) );
				pKey->m_pValue = CopyString( str );

				strcpy( pKey->m_pKey, "noise2" );
			}
*/		}
	}
}


void ScanChunks( CChunk *pChunk, void (*fn)(CChunk *) )
{
	fn( pChunk );

	// Recurse into the children.
	for( unsigned short i=pChunk->m_Chunks.Head(); i != pChunk->m_Chunks.InvalidIndex(); i = pChunk->m_Chunks.Next( i ) )
	{
		ScanChunks( pChunk->m_Chunks[i], fn );
	}
}


int main(int argc, char* argv[])
{
	if( argc < 3 )
	{
		printf( "vmf_tweak <input file> <output file>\n" );
		return 1;
	}

	char const *pInFilename = argv[1];
	char const *pOutFilename = argv[2];

	CChunk *pRoot = ReadChunkFile( pInFilename );
	if( !pRoot )
		return 2;

	//
	//
	//
	// This is where you can put code to modify the VMF.
	//
	//
	//
	//ScanChunks( pRoot, ScanMoveDir );
	//ScanChunks( pRoot, ScanFuncWaterMoveDir );
	//ScanChunks( pRoot, ScanMomentaryRotButton );

	char const *pHL1Port = argv[3];

	if ( pHL1Port && !stricmp( pHL1Port, "-hl1port" ) )
		 ScanChunks( pRoot, ScanHL1 );
	
	if( !WriteChunkFile( pOutFilename, pRoot ) )
		return 3;

	return 0;
}

