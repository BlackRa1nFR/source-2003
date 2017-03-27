//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Voice / Sentence streaming & parsing code
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

//===============================================================================
// VOX. Algorithms to load and play spoken text sentences from a file:
//
// In ambient sounds or entity sounds, precache the 
// name of the sentence instead of the wave name, ie: !C1A2S4
//
// During sound system init, the 'sentences.txt' is read.
// This file has the format:
//
//		C1A2S4 agrunt/vox/You will be exterminated, surrender NOW.
//      C1A2s5 hgrunt/vox/Radio check, over.
//		...
//
//		There must be at least one space between the sentence name and the sentence.
//		Sentences may be separated by one or more lines
//		There may be tabs or spaces preceding the sentence name
//		The sentence must end in a /n or /r
//		Lines beginning with // are ignored as comments
//
//		Period or comma will insert a pause in the wave unless
//		the period or comma is the last character in the string.
//
//		If first 2 chars of a word are upper case, word volume increased by 25%
// 
//		If last char of a word is a number from 0 to 9
//		then word will be pitch-shifted up by 0 to 9, where 0 is a small shift
//		and 9 is a very high pitch shift.
//
// We alloc heap space to contain this data, and track total 
// sentences read.  A pointer to each sentence is maintained in g_Sentences.
//
// When sound is played back in S_StartDynamicSound or s_startstaticsound, we detect the !name
// format and lookup the actual sentence in the sentences array
//
// To play, we parse each word in the sentence, chain the words, and play the sentence
// each word's data is loaded directy from disk and freed right after playback.
//===============================================================================

#include <stdio.h>
#include <ctype.h>
#include "tier0/dbg.h"
#include "convar.h"
#include "basetypes.h"
#include "commonmacros.h"
#include "const.h"
#include "sound_private.h"
#include "vox_private.h"
#include "snd_channels.h"
#include "characterset.h"
#include "snd_device.h"
#include "snd_audio_source.h"
#include "vstdlib/random.h"
#include "soundservice.h"
#include "common.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// In other C files.
// Globals

// This is the initial capacity for sentences, the array will grow if necessary
#define MAX_EXPECTED_SENTENCES	2048

CUtlVector<sentence_t>	g_Sentences;

// Module Locals
static char		*rgpparseword[CVOXWORDMAX];	// array of pointers to parsed words
static char		voxperiod[] = "_period";				// vocal pause
static char		voxcomma[] = "_comma";				// vocal pause


// Sentence file list management
static void			VOX_ListClear( void );
static int			VOX_ListFileIsLoaded( const char *psentenceFileName );
static void			VOX_ListMarkFileLoaded( const char *psentenceFileName, byte *pFileData );

// This module depends on these engine calls:
// DevMsg
// S_FreeChannel
// S_LoadSound
// S_FindName
// COM_LoadFileForMe() 
// COM_FreeFile
// It also depends on vstdlib/RandomInt (all other random calls go through g_pSoundServices)

void VOX_Init( void ) 
{
	g_Sentences.RemoveAll();
	g_Sentences.EnsureCapacity( MAX_EXPECTED_SENTENCES );

	VOX_ListClear();
	VOX_ReadSentenceFile( "scripts/sentences.txt" );
}


void VOX_Shutdown( void ) 
{
	g_Sentences.RemoveAll();
	VOX_ListClear();
}

//-----------------------------------------------------------------------------
// Purpose: This is kind of like strchr(), but we get the actual pointer to the
//			end of the string when it fails rather than NULL.  This is useful
//			for parsing buffers containing multiple strings
// Input  : *string - 
//			scan - 
// Output : char
//-----------------------------------------------------------------------------
char *ScanForwardUntil( char *string, char scan )
{
	while( string[0] )
	{
		if ( string[0] == scan )
			return string;

		string++;
	}
	return string;
}

// parse a null terminated string of text into component words, with
// pointers to each word stored in rgpparseword
// note: this code actually alters the passed in string!

char **VOX_ParseString(char *psz) 
{
	int i;
	int fdone = 0;
	char *pszscan = psz;
	char c;
	characterset_t nextWord, skip;

	memset(rgpparseword, 0, sizeof(char *) * CVOXWORDMAX);

	if (!psz)
		return NULL;

	i = 0;
	rgpparseword[i++] = psz;

	CharacterSetBuild( &nextWord, " ,.({" );
	CharacterSetBuild( &skip, "., " );
	while (!fdone && i < CVOXWORDMAX)
	{
		// scan up to next word
		c = *pszscan;
		while (c && !IN_CHARACTERSET(nextWord,c) )
			c = *(++pszscan);
			
		// if '(' then scan for matching ')'
		if ( c == '(' || c=='{' )
		{
			if ( c == '(' )
				pszscan = ScanForwardUntil( pszscan, ')' );
			else if ( c == '{' )
				pszscan = ScanForwardUntil( pszscan, '}' );

			c = *(++pszscan);
			if (!c)
				fdone = 1;
		}

		if (fdone || !c)
			fdone = 1;
		else
		{	
			// if . or , insert pause into rgpparseword,
			// unless this is the last character
			if ((c == '.' || c == ',') && *(pszscan+1) != '\n' && *(pszscan+1) != '\r'
					&& *(pszscan+1) != 0)
			{
				if (c == '.')
					rgpparseword[i++] = voxperiod;
				else
					rgpparseword[i++] = voxcomma;

				if (i >= CVOXWORDMAX)
					break;
			}

			// null terminate substring
			*pszscan++ = 0;

			// skip whitespace
			c = *pszscan;
			while (c && IN_CHARACTERSET(skip, c))
				c = *(++pszscan);

			if (!c)
				fdone = 1;
			else
				rgpparseword[i++] = pszscan;
		}
	}
	return rgpparseword;
}

// backwards scan psz for last '/'
// return substring in szpath null terminated
// if '/' not found, return 'vox/'

char *VOX_GetDirectory(char *szpath, char *psz)
{
	char c;
	int cb = 0;
	char *pszscan = psz + strlen(psz) - 1;

	// scan backwards until first '/' or start of string
	c = *pszscan;
	while (pszscan > psz && c != '/')
	{
		c = *(--pszscan);
		cb++;
	}

	if (c != '/')
	{
		// didn't find '/', return default directory
		strcpy(szpath, "vox/");
		return psz;
	}

	cb = strlen(psz) - cb;
	memcpy(szpath, psz, cb);
	szpath[cb] = 0;
	return pszscan + 1;
}

// set channel volume based on volume of current word
#ifndef SWDS
void VOX_SetChanVol(channel_t *ch)
{
	if ( !ch->pMixer )
		return;

	float scale = ch->pMixer->GetVolumeScale();
	if ( scale == 1.0 )
		return;

	ch->rightvol = (int) (ch->rightvol * scale);
	ch->leftvol = (int) (ch->leftvol * scale);

	if ( g_AudioDevice->Should3DMix() )
	{
		ch->rrightvol = (int) (ch->rrightvol * scale);
		ch->rleftvol = (int) (ch->rleftvol * scale);
	}
	else
	{
		ch->rrightvol = 0;
		ch->rleftvol = 0;
	}
}
#endif

//===============================================================================
//  Get any pitch, volume, start, end params into voxword
//  and null out trailing format characters
//  Format: 
//		someword(v100 p110 s10 e20)
//		
//		v is volume, 0% to n%
//		p is pitch shift up 0% to n%
//		s is start wave offset %
//		e is end wave offset %
//		t is timecompression %
//
//	pass fFirst == 1 if this is the first string in sentence
//  returns 1 if valid string, 0 if parameter block only.
//
//  If a ( xxx ) parameter block does not directly follow a word, 
//  then that 'default' parameter block will be used as the default value
//  for all following words.  Default parameter values are reset
//  by another 'default' parameter block.  Default parameter values
//  for a single word are overridden for that word if it has a parameter block.
// 
//===============================================================================

int VOX_ParseWordParams(char *psz, voxword_t *pvoxword, int fFirst) 
{
	char *pszsave = psz;
	char c;
	char ct;
	char sznum[8];
	int i;
	static voxword_t voxwordDefault;
	characterset_t commandSet, delimitSet;

	// List of valid commands
	CharacterSetBuild( &commandSet, "vpset)" );

	// init to defaults if this is the first word in string.
	if (fFirst)
	{
		voxwordDefault.pitch = -1;
		voxwordDefault.volume = 100;
		voxwordDefault.start = 0;
		voxwordDefault.end = 100;
		voxwordDefault.fKeepCached = 0;
		voxwordDefault.timecompress = 0;
	}

	*pvoxword = voxwordDefault;

	// look at next to last char to see if we have a 
	// valid format:

	c = *(psz + strlen(psz) - 1);
	
	if (c != ')')
		return 1;		// no formatting, return

	// scan forward to first '('
	CharacterSetBuild( &delimitSet, "()" );
	c = *psz;
	while ( !IN_CHARACTERSET(delimitSet, c) )
		c = *(++psz);
	
	if ( c == ')' )
		return 0;		// bogus formatting
	
	// null terminate

	*psz = 0;
	ct = *(++psz);

	while (1)
	{
		// scan until we hit a character in the commandSet

		while (ct && !IN_CHARACTERSET(commandSet, ct) )
			ct = *(++psz);
		
		if (ct == ')')
			break;

		memset(sznum, 0, sizeof(sznum));
		i = 0;

		c = *(++psz);
		
		if (!isdigit(c))
			break;

		// read number
		while (isdigit(c) && i < sizeof(sznum) - 1)
		{
			sznum[i++] = c;
			c = *(++psz);
		}

		// get value of number
		i = atoi(sznum);

		switch (ct)
		{
		case 'v': pvoxword->volume = i; break;
		case 'p': pvoxword->pitch = i; break;
		case 's': pvoxword->start = i; break;
		case 'e': pvoxword->end = i; break;
		case 't': pvoxword->timecompress = i; break;
		}

		ct = c;
	}

	// if the string has zero length, this was an isolated
	// parameter block.  Set default voxword to these
	// values

	if (strlen(pszsave) == 0)
	{
		voxwordDefault = *pvoxword;
		return 0;
	}
	else 
		return 1;
}

// link all sounds in sentence, start playing first word.
void VOX_LoadSound( channel_t *pchan, const char *pszin )
{
#ifndef SWDS
	char buffer[512];
	int i, cword;
	char	pathbuffer[64];
	char	szpath[32];
	voxword_t rgvoxword[CVOXWORDMAX];
	char *psz;

	if (!pszin)
		return;

	memset(rgvoxword, 0, sizeof (voxword_t) * CVOXWORDMAX);
	memset(buffer, 0, sizeof(buffer));

	// lookup actual string in g_Sentences, 
	// set pointer to string data

	psz = VOX_LookupString(pszin, NULL);

	if (!psz)
	{
		DevMsg ("VOX_LoadSound: no sentence named %s\n",pszin);
		return;
	}

	// get directory from string, advance psz
	psz = VOX_GetDirectory(szpath, psz);

	if (strlen(psz) > sizeof(buffer) - 1)
	{
		DevMsg ("VOX_LoadSound: sentence is too long %s\n",psz);
		return;
	}

	// copy into buffer
	strcpy(buffer, psz);
	psz = buffer;

	// parse sentence (also inserts null terminators between words)
	
	VOX_ParseString(psz);

	// for each word in the sentence, construct the filename,
	// lookup the sfx and save each pointer in a temp array	

	i = 0;
	cword = 0;
	while (rgpparseword[i])
	{
		// Get any pitch, volume, start, end params into voxword

		if (VOX_ParseWordParams(rgpparseword[i], &rgvoxword[cword], i == 0))
		{
			// this is a valid word (as opposed to a parameter block)
			strcpy(pathbuffer, szpath);
			Q_strncat(pathbuffer, rgpparseword[i], sizeof( pathbuffer ) );
			Q_strncat(pathbuffer, ".wav", sizeof( pathbuffer ));

			// find name, if already in cache, mark voxword
			// so we don't discard when word is done playing
			rgvoxword[cword].sfx = S_FindName(pathbuffer, 
					&(rgvoxword[cword].fKeepCached));
			cword++;
		}
		i++;
	}
	pchan->pMixer = CreateSentenceMixer( rgvoxword );
	if ( !pchan->pMixer )
		return;

	pchan->isSentence = true;
	pchan->sfx = rgvoxword[0].sfx;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Take a NULL terminated sentence, and parse any commands contained in
//			{}.  The string is rewritten in place with those commands removed.
//
// Input  : *pSentenceData - sentence data to be modified in place
//			sentenceIndex - global sentence table index for any data that is 
//							parsed out
//-----------------------------------------------------------------------------
void VOX_ParseLineCommands( char *pSentenceData, int sentenceIndex )
{
	char tempBuffer[512];
	char *pNext, *pStart;
	int  length, tempBufferPos = 0;

	if ( !pSentenceData )
		return;

	pStart = pSentenceData;

	while ( *pSentenceData )
	{
		pNext = ScanForwardUntil( pSentenceData, '{' );

		// Find length of "good" portion of the string (not a {} command)
		length = pNext - pSentenceData;
		if ( tempBufferPos + length > sizeof(tempBuffer) )
		{
			DevMsg("Error! sentence too long!\n" );
			return;
		}

		// Copy good string to temp buffer
		memcpy( tempBuffer + tempBufferPos, pSentenceData, length );
		
		// Move the copy position
		tempBufferPos += length;

		pSentenceData = pNext;
		
		// Skip ahead of the opening brace
		if ( *pSentenceData )
			pSentenceData++;
		
		// Skip whitespace
		while ( *pSentenceData && *pSentenceData <= 32 )
			pSentenceData++;

		// Simple comparison of string commands:
		switch( tolower(*pSentenceData) )
		{
		case 'l':
			// All commands starting with the letter 'l' here
			if ( !strnicmp( pSentenceData, "len", 3 ) )
			{
				g_Sentences[sentenceIndex].length = atof( pSentenceData + 3 ) ;
			}
			break;

		case 0:
		default:
			break;
		}

		pSentenceData = ScanForwardUntil( pSentenceData, '}' );
		
		// Skip the closing brace
		if ( *pSentenceData )
			pSentenceData++;

		// Skip trailing whitespace
		while ( *pSentenceData && *pSentenceData <= 32 )
			pSentenceData++;
	}

	if ( tempBufferPos < sizeof(tempBuffer) )
	{
		// terminate cleaned up copy
		tempBuffer[ tempBufferPos ] = 0;
		
		// Copy it over the original data
		strcpy( pStart, tempBuffer );
	}
}

#define CSENTENCE_LRU_MAX		32

struct sentencegroup_t
{
	char szgroupname[CVOXSENTENCEMAX];
	int count;
	unsigned char rgblru[CSENTENCE_LRU_MAX];

};

CUtlVector<sentencegroup_t> g_SentenceGroups;

//-----------------------------------------------------------------------------
// Purpose: Add a new group or increment count of the existing one
// Input  : *pSentenceName - text of the sentence name
//-----------------------------------------------------------------------------
void VOX_GroupAdd( const char *pSentenceName )
{
	int len = strlen( pSentenceName ) - 1;
	char groupName[CVOXSENTENCEMAX];

	// group members end in a number
	if ( len <= 0 || !isdigit(pSentenceName[len]) )
		return;

	// truncate away the index
	while ( len > 0 && isdigit(pSentenceName[len]) )
		len--;

	if ( len > sizeof(groupName)-1 )
	{
		Msg( "Group %s name too long (%d chars max)\n", pSentenceName, sizeof(groupName)-1 );
		return;
	}

	// make a copy of the actual group name
	Q_strncpy( groupName, pSentenceName, len+2 );

	// check for it in the list
	int i;
	sentencegroup_t *pGroup;

	int groupCount = g_SentenceGroups.Size();
	for ( i = 0; i < groupCount; i++ )
	{
		int groupIndex = (i + groupCount-1) % groupCount;
		// Start at the last group a loop around
		pGroup = &g_SentenceGroups[groupIndex];
		if ( !strcmp( pGroup->szgroupname, groupName ) )
		{
			// Matches previous group, bump count
			pGroup->count++;
			return;
		}
	}

	// new group
	int addIndex = g_SentenceGroups.AddToTail();
	sentencegroup_t *group = &g_SentenceGroups[addIndex];
	strcpy( group->szgroupname, groupName );
	group->count = 1;
}



#if DEAD
//-----------------------------------------------------------------------------
// Purpose: clear the sentence groups
//-----------------------------------------------------------------------------
void VOX_GroupClear( void )
{
	g_SentenceGroups.RemoveAll();
}
#endif


void VOX_LRUInit( sentencegroup_t *pGroup )
{
	int i, n1, n2, temp;

	if ( pGroup->count )
	{
		if (pGroup->count > CSENTENCE_LRU_MAX)
			pGroup->count = CSENTENCE_LRU_MAX;

		for (i = 0; i < pGroup->count; i++)
			pGroup->rgblru[i] = (unsigned char) i;

		// randomize array by swapping random elements
		for (i = 0; i < (pGroup->count * 4); i++)
		{
			// FIXME: This should probably call through g_pSoundServices
			// or some other such call?
			n1 = RandomInt(0,pGroup->count-1);
			n2 = RandomInt(0,pGroup->count-1);
			temp = pGroup->rgblru[n1];
			pGroup->rgblru[n1] = pGroup->rgblru[n2];
			pGroup->rgblru[n2] = temp;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Init the LRU for each sentence group
//-----------------------------------------------------------------------------
void VOX_GroupInitAllLRUs( void )
{
	int i;

	for ( i = 0; i < g_SentenceGroups.Size(); i++ )
	{
		VOX_LRUInit( &g_SentenceGroups[i] );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Given a group name, return that group's index
// Input  : *pGroupName - name of the group
// Output : int - index in group table, returns -1 if no matching group is found
//-----------------------------------------------------------------------------
int VOX_GroupIndexFromName( const char *pGroupName )
{
	int i;

	if ( pGroupName )
	{
		// search rgsentenceg for match on szgroupname
		for ( i = 0; i < g_SentenceGroups.Size(); i++ )
		{
			if ( !strcmp( g_SentenceGroups[i].szgroupname, pGroupName ) )
				return i;
		}
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: return the group's name
// Input  : groupIndex - index of the group
// Output : const char * - name pointer
//-----------------------------------------------------------------------------
const char *VOX_GroupNameFromIndex( int groupIndex )
{
	if ( groupIndex >= 0 && groupIndex < g_SentenceGroups.Size() )
		return g_SentenceGroups[groupIndex].szgroupname;

	return NULL;
}

// ignore lru. pick next sentence from sentence group. Go in order until we hit the last sentence, 
// then repeat list if freset is true.  If freset is false, then repeat last sentence.
// ipick is passed in as the requested sentence ordinal.
// ipick 'next' is returned.  
// return of -1 indicates an error.

int VOX_GroupPickSequential( int isentenceg, char *szfound, int szfoundLen, int ipick, int freset )
{
	char *szgroupname;
	unsigned char count;
	
	if (isentenceg < 0 || isentenceg > g_SentenceGroups.Size())
		return -1;

	szgroupname = g_SentenceGroups[isentenceg].szgroupname;
	count = g_SentenceGroups[isentenceg].count;
	
	if (count == 0)
		return -1;

	if (ipick >= count)
		ipick = count-1;

	Q_snprintf( szfound, szfoundLen, "!%s%d", szgroupname, ipick );
	
	if (ipick >= count)
	{
		if (freset)
			// reset at end of list
			return 0;
		else
			return count;
	}

	return ipick + 1;
}



// pick a random sentence from rootname0 to rootnameX.
// picks from the rgsentenceg[isentenceg] least
// recently used, modifies lru array. returns the sentencename.
// note, lru must be seeded with 0-n randomized sentence numbers, with the
// rest of the lru filled with -1. The first integer in the lru is
// actually the size of the list.  Returns ipick, the ordinal
// of the picked sentence within the group.

int VOX_GroupPick( int isentenceg, char *szfound, int strLen )
{
	char *szgroupname;
	unsigned char *plru;
	unsigned char i;
	unsigned char count;
	unsigned char ipick=0;
	int ffound = FALSE;
	
	if (isentenceg < 0 || isentenceg > g_SentenceGroups.Size())
		return -1;

	szgroupname = g_SentenceGroups[isentenceg].szgroupname;
	count = g_SentenceGroups[isentenceg].count;
	plru = g_SentenceGroups[isentenceg].rgblru;

	while (!ffound)
	{
		for (i = 0; i < count; i++)
			if (plru[i] != 0xFF)
			{
				ipick = plru[i];
				plru[i] = 0xFF;
				ffound = TRUE;
				break;
			}

		if (!ffound)
		{
			VOX_LRUInit( &g_SentenceGroups[isentenceg] );
		}
		else
		{
			Q_snprintf( szfound, strLen, "!%s%d", szgroupname, ipick );
			return ipick;
		}
	}
	return -1;
}


typedef struct filelist_s filelist_t;
struct filelist_s
{
	const char	*pFileName;
	byte		*pFileData;
	filelist_t	*pNext;
};

static filelist_t *g_pSentenceFileList = NULL;

//-----------------------------------------------------------------------------
// Purpose: clear / reinitialize the vox list
//-----------------------------------------------------------------------------
void VOX_ListClear( void )
{
	filelist_t *pList, *pNext;
	
	pList = g_pSentenceFileList;
	
	while ( pList )
	{
		pNext = pList->pNext;

		COM_FreeFile( pList->pFileData );
		free( pList );

		pList = pNext;
	}

	g_pSentenceFileList = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if this file is in the list
// Input  : *psentenceFileName - 
// Output : int, true if the file is in the list, false if not
//-----------------------------------------------------------------------------
int VOX_ListFileIsLoaded( const char *psentenceFileName )
{
	filelist_t *pList = g_pSentenceFileList;
	while ( pList )
	{
		if ( !strcmp( psentenceFileName, pList->pFileName ) )
			return true;

		pList = pList->pNext;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Add this file name to the sentence list
// Input  : *psentenceFileName - 
//-----------------------------------------------------------------------------
void VOX_ListMarkFileLoaded( const char *psentenceFileName, byte *pFileData )
{
	filelist_t	*pEntry;
	char		*pName;

	pEntry = (filelist_t *)malloc( sizeof(filelist_t) + strlen( psentenceFileName ) + 1);

	if ( pEntry )
	{
		pName = (char *)(pEntry+1);
		strcpy( pName, psentenceFileName );

		pEntry->pFileName = pName;
		pEntry->pFileData = pFileData;
		pEntry->pNext = g_pSentenceFileList;

		g_pSentenceFileList = pEntry;
	}
}

// Load sentence file into memory, insert null terminators to
// delimit sentence name/sentence pairs.  Keep pointer to each
// sentence name so we can search later.

void VOX_ReadSentenceFile( const char *psentenceFileName )
{
	char *pch, *pFileData;
	int fileSize;
	char c;
	char *pchlast, *pSentenceData;
	characterset_t whitespace;


	// Have we already loaded this file?
	if ( VOX_ListFileIsLoaded( psentenceFileName ) )
		return;

	// load file

	// USES malloc()
	pFileData = (char*)COM_LoadFileForMe( psentenceFileName, &fileSize );

	if (!pFileData)
	{
		DevMsg ("Couldn't load %s\n", psentenceFileName);
		return;
	} 

	pch = pFileData;
	pchlast = pch + fileSize;
	CharacterSetBuild( &whitespace, "\n\r\t " );
	while (pch < pchlast)
	{
		// Only process this pass on sentences
		pSentenceData = NULL;

		// skip newline, cr, tab, space

		c = *pch;
		while (pch < pchlast && IN_CHARACTERSET( whitespace, c ))
			c = *(++pch);

		// skip entire line if first char is /
		if (*pch != '/')
		{
			int addIndex = g_Sentences.AddToTail();
			sentence_t *pSentence = &g_Sentences[addIndex];
			pSentence->pName = pch;
			pSentence->length = 0;

			// scan forward to first space, insert null terminator
			// after sentence name

			c = *pch;
			while (pch < pchlast && c != ' ')
				c = *(++pch);

			if (pch < pchlast)
				*pch++ = 0;

			// A sentence may have some line commands, make an extra pass
			pSentenceData = pch;
		}
		// scan forward to end of sentence or eof
		while (pch < pchlast && pch[0] != '\n' && pch[0] != '\r')
			pch++;
	
		// insert null terminator
		if (pch < pchlast)
			*pch++ = 0;

		// If we have some sentence data, parse out any line commands
		if ( pSentenceData && pSentenceData < pchlast )
		{
			int index = g_Sentences.Size()-1;
			// The current sentence has an index of count-1
			VOX_ParseLineCommands( pSentenceData, index );

			// Add a new group or increment count of the existing one
			VOX_GroupAdd( g_Sentences[index].pName );
		}
	}
	VOX_GroupInitAllLRUs();
	VOX_ListMarkFileLoaded( psentenceFileName, (unsigned char*)pFileData );
}


//-----------------------------------------------------------------------------
// Purpose: Get the current number of sentences in the database
// Output : int
//-----------------------------------------------------------------------------
int VOX_SentenceCount( void )
{
	return g_Sentences.Size();
}


float VOX_SentenceLength( int sentence_num )
{
	if ( sentence_num < 0 || sentence_num > g_Sentences.Size()-1 )
		return 0.0f;
	
	return g_Sentences[ sentence_num ].length;
}

// scan g_Sentences, looking for pszin sentence name
// return pointer to sentence data if found, null if not
// CONSIDER: if we have a large number of sentences, should
// CONSIDER: sort strings in g_Sentences and do binary search.
char *VOX_LookupString(const char *pSentenceName, int *psentencenum)
{
	int i;
	for (i = 0; i < g_Sentences.Size(); i++)
	{
		if (!stricmp(pSentenceName, g_Sentences[i].pName))
		{
			if (psentencenum)
				*psentencenum = i;
			return (g_Sentences[i].pName + strlen(g_Sentences[i].pName) + 1);
		}
	}
	return NULL;
}


// Abstraction for sentence name array
const char *VOX_SentenceNameFromIndex( int sentencenum )
{
	if ( sentencenum < g_Sentences.Size() )
		return g_Sentences[sentencenum].pName;
	return NULL;
}



