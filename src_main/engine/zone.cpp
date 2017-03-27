//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//						ZONE MEMORY ALLOCATION
//
// There is never any space between memblocks, and there will never be two
// contiguous free memblocks.
//
// The rover can be left pointing at a non-empty block
//
// The zone calls are pretty much only used for small strings and structures,
// all big things are allocated on the hunk.
//=============================================================================	   

#include "zone.h"

#include "quakedef.h"
#include <studio.h>
#include "filesystem.h"
#include "filesystem_engine.h"
#include "sound.h" // just to get at MAX_SFX
#include "vstdlib/strtools.h"
#include "vstdlib/ICommandLine.h"


void Cache_FreeLow (int new_low_hunk);
void Cache_FreeHigh (int new_high_hunk);

ConVar mem_dbgfile( "mem_dbgfile",".\\mem.txt" );

//============================================================================

#define	HUNK_SENTINAL	0x1df001ed

#define HUNK_NAME_LEN 64
#ifdef _WIN32
typedef __declspec(align(16)) struct
#elif _LINUX
typedef __attribute__((aligned(16))) struct
#else
#error
#endif
{
	int		sentinal;
	int		size;		// including sizeof(hunk_t), -1 = not allocated
	char	name[HUNK_NAME_LEN];
} hunk_t;

byte	*hunk_base;
int		hunk_size;

int		hunk_low_used;
int		hunk_high_used;

qboolean	hunk_tempactive;
int		hunk_tempmark;

void R_FreeTextures (void);

/*
==============
Hunk_Check

Run consistancy and sentinal trahing checks
==============
*/
void Hunk_Check (void)
{
	hunk_t	*h;
	
	for (h = (hunk_t *)hunk_base ; (byte *)h != hunk_base + hunk_low_used ; )
	{
		if (h->sentinal != HUNK_SENTINAL)
			Sys_Error ("Hunk_Check: trashed sentinal");
		if (h->size < 16 || h->size + (byte *)h - hunk_base > hunk_size)
			Sys_Error ("Hunk_Check: bad size");
		h = (hunk_t *)((byte *)h+h->size);
	}
}

/*
==============
Hunk_Print

If "all" is specified, every single allocation is printed.
Otherwise, allocations with the same name will be totaled up before printing.
==============
*/
void Hunk_Print (qboolean all)
{
	hunk_t	*h, *next, *endlow, *starthigh, *endhigh;
	int		count, sum;
	int		totalblocks;
	char	name[HUNK_NAME_LEN+1];

	name[HUNK_NAME_LEN] = 0;
	count = 0;
	sum = 0;
	totalblocks = 0;
	
	FileHandle_t file = g_pFileSystem->Open(mem_dbgfile.GetString(), "a");
	if (!file)
		return;

	h = (hunk_t *)hunk_base;
	endlow = (hunk_t *)(hunk_base + hunk_low_used);
	starthigh = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);
	endhigh = (hunk_t *)(hunk_base + hunk_size);

	//Con_Printf ("          :%8i total hunk size\n", hunk_size);
	//Con_Printf ("-------------------------\n");
	g_pFileSystem->FPrintf(file, "          :%16.16s total hunk size\n", Q_pretifymem(hunk_size));
	g_pFileSystem->FPrintf(file, "-------------------------\n");

	while (1)
	{
	//
	// skip to the high hunk if done with low hunk
	//
		if ( h == endlow )
		{
			g_pFileSystem->FPrintf(file, "-------------------------\n");
			g_pFileSystem->FPrintf(file, "          :%16.16s REMAINING\n", Q_pretifymem(hunk_size - hunk_low_used - hunk_high_used));
			g_pFileSystem->FPrintf(file, "-------------------------\n");
			h = starthigh;
		}
		
	//
	// if totally done, break
	//
		if ( h == endhigh )
			break;

	//
	// run consistancy checks
	//
		if (h->sentinal != HUNK_SENTINAL)
			Sys_Error ("Hunk_Check: trahsed sentinal");
		if (h->size < 16 || h->size + (byte *)h - hunk_base > hunk_size)
			Sys_Error ("Hunk_Check: bad size");
			
		next = (hunk_t *)((byte *)h+h->size);
		count++;
		totalblocks++;
		sum += h->size;

	//
	// print the single block
	//
		memcpy (name, h->name, HUNK_NAME_LEN);
		if (all)
			g_pFileSystem->FPrintf(file, "%8p :%16.16s %16s\n",h, Q_pretifymem(h->size), name);
			
	//
	// print the total
	//
		if (next == endlow || next == endhigh || 
		strncmp (h->name, next->name, HUNK_NAME_LEN) )
		{
			if (!all)
				g_pFileSystem->FPrintf(file, "          :%16.16s %16s (TOTAL)\n",Q_pretifymem(sum), name);
			count = 0;
			sum = 0;
		}

		h = next;
	}

	g_pFileSystem->FPrintf(file, "-------------------------\n");
	g_pFileSystem->FPrintf(file, "%8i total blocks\n", totalblocks);
	g_pFileSystem->Close(file);
}


/*
===================
Hunk_AllocName
===================
*/
void *Hunk_AllocName (int size, const char *name)
{
	hunk_t	*h;
	
#ifdef PARANOID
	Hunk_Check ();
#endif

	if (size < 0)
		Sys_Error ("Hunk_Alloc: bad size: %i", size);
		
	size = sizeof(hunk_t) + ((size+15)&~15);
	
	if (hunk_size - hunk_low_used - hunk_high_used < size)
		Sys_Error ("Hunk_Alloc: failed on %i bytes",size);
	
	h = (hunk_t *)(hunk_base + hunk_low_used);
	hunk_low_used += size;

	Cache_FreeLow (hunk_low_used);

	memset (h, 0, size);
	
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	Q_strncpy(h->name, name, HUNK_NAME_LEN);
	
	return (void *)(h+1);
}

/*
===================
Hunk_Alloc
===================
*/
void *Hunk_Alloc (int size)
{
	return Hunk_AllocName (size, "unknown");
}

int	Hunk_LowMark (void)
{
	return hunk_low_used;
}

void Hunk_FreeToLowMark (int mark)
{
	if (mark < 0 || mark > hunk_low_used)
		Sys_Error ("Hunk_FreeToLowMark: bad mark %i", mark);
#if 0
	memset (hunk_base + mark, 0, hunk_low_used - mark);
#else
	// Fill the memory with junk (debug only) or don't bother in non-debug
	#if _DEBUG
	memset (hunk_base + mark, 0xDD, hunk_low_used - mark);
	#endif
#endif
	hunk_low_used = mark;
}

int	Hunk_HighMark (void)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark (hunk_tempmark);
	}

	return hunk_high_used;
}

void Hunk_FreeToHighMark (int mark)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark (hunk_tempmark);
	}
	if (mark < 0 || mark > hunk_high_used)
		Sys_Error ("Hunk_FreeToHighMark: bad mark %i", mark);

#if 0
	memset (hunk_base + hunk_size - hunk_high_used, 0, hunk_high_used - mark);
#else
	// Fill the memory with junk (debug only) or don't bother in non-debug
	#if _DEBUG
	memset (hunk_base + hunk_size - hunk_high_used, 0xDD, hunk_high_used - mark);
	#endif
#endif
	hunk_high_used = mark;
}


/*
===================
Hunk_HighAllocName
===================
*/
void *Hunk_HighAllocName (int size, char *name)
{
	hunk_t	*h;

	if (size < 0)
		Sys_Error ("Hunk_HighAllocName: bad size: %i", size);

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}

#ifdef PARANOID
	Hunk_Check ();
#endif

	size = sizeof(hunk_t) + ((size+15)&~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
	{
		Con_Printf ("Hunk_HighAlloc: failed on %i bytes\n",size);
		return NULL;
	}

	hunk_high_used += size;
	Cache_FreeHigh (hunk_high_used);

	h = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);

	memset (h, 0, size);
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	Q_strncpy(h->name, name, HUNK_NAME_LEN);

	return (void *)(h+1);
}


/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
void *Hunk_TempAlloc (int size)
{
	void	*buf;

	size = (size+15)&~15;
	
	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark (hunk_tempmark);
		hunk_tempactive = false;
	}
	
	hunk_tempmark = Hunk_HighMark ();

	buf = Hunk_HighAllocName (size, "temp");

	hunk_tempactive = true;

	return buf;
}

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/

#define CACHE_NAME_LEN 64
typedef struct cache_system_s
{
	int						size;		// including this header
	cache_user_t			*user;
	char					name[CACHE_NAME_LEN];
	struct cache_system_s	*prev, *next;
	struct cache_system_s	*lru_prev, *lru_next;	// for LRU flushing	
	int						locked;
} cache_system_t;

cache_system_t *Cache_TryAlloc (int size, qboolean nobottom);

cache_system_t	cache_head;

int	cache_critical_section;

/*
===========
Cache_Move
===========
*/
void Cache_Move ( cache_system_t *c)
{
	cache_system_t		*newmem;

// we are clearing up space at the bottom, so only allocate it late
	newmem = Cache_TryAlloc (c->size, true);
	if (newmem)
	{
//		Con_Printf ("cache_move ok\n");

		Q_memcpy ( newmem+1, c+1, c->size - sizeof(cache_system_t) );
		newmem->user = c->user;
		Q_memcpy (newmem->name, c->name, sizeof(newmem->name));
		Cache_Free (c->user);
		newmem->user->data = (void *)(newmem+1);
	}
	else
	{
//		Con_Printf ("cache_move failed\n");

		Cache_Free (c->user);		// tough luck...
	}
}

/*
============
Cache_FreeLow

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeLow (int new_low_hunk)
{
	cache_system_t	*c;
	
	while (1)
	{
		c = cache_head.next;
		if (c == &cache_head)
			return;		// nothing in cache at all
		if ((byte *)c >= hunk_base + new_low_hunk)
			return;		// there is space to grow the hunk
		Cache_Move ( c );	// reclaim the space
	}
}

/*
============
Cache_FreeHigh

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeHigh (int new_high_hunk)
{
	cache_system_t	*c, *prev;
	
	prev = NULL;
	while (1)
	{
		c = cache_head.prev;
		if (c == &cache_head)
			return;		// nothing in cache at all
		if ( (byte *)c + c->size <= hunk_base + hunk_size - new_high_hunk)
			return;		// there is space to grow the hunk
		if (c == prev)
			Cache_Free (c->user);	// didn't move out of the way
		else
		{
			Cache_Move (c);	// try to move it
			prev = c;
		}
	}
}

void Cache_UnlinkLRU (cache_system_t *cs)
{
	if (!cs->lru_next || !cs->lru_prev)
		Sys_Error ("Cache_UnlinkLRU: NULL link");

	cs->lru_next->lru_prev = cs->lru_prev;
	cs->lru_prev->lru_next = cs->lru_next;
	
	cs->lru_prev = cs->lru_next = NULL;

	// consider it unneeded.
	cs->locked = 0;
}

void Cache_MakeLRU (cache_system_t *cs)
{
	if (cs->lru_next || cs->lru_prev)
		Sys_Error ("Cache_MakeLRU: active link");

	cache_head.lru_next->lru_prev = cs;
	cs->lru_next = cache_head.lru_next;
	cs->lru_prev = &cache_head;
	cache_head.lru_next = cs;

	// if it's wanted it must be important.
	cs->locked = cache_critical_section;
}

/*
============
Cache_TryAlloc

Looks for a free block of memory between the high and low hunk marks
Size should already include the header and padding
============
*/
cache_system_t *Cache_TryAlloc (int size, qboolean nobottom)
{
	cache_system_t	*cs, *newmem;
	
// is the cache completely empty?

	if (!nobottom && cache_head.prev == &cache_head)
	{
		if (hunk_size - hunk_high_used - hunk_low_used < size)
			Sys_Error ("Cache_TryAlloc: %i is greater then free hunk", size);

		newmem = (cache_system_t *) (hunk_base + hunk_low_used);
		memset (newmem, 0, sizeof(*newmem));
		newmem->size = size;

		cache_head.prev = cache_head.next = newmem;
		newmem->prev = newmem->next = &cache_head;
		
		Cache_MakeLRU (newmem);
		return newmem;
	}
	
// search from the bottom up for space

	newmem = (cache_system_t *) (hunk_base + hunk_low_used);
	cs = cache_head.next;
	
	do
	{
		if (!nobottom || cs != cache_head.next)
		{
			if ( (byte *)cs - (byte *)newmem >= size)
			{	// found space
				memset (newmem, 0, sizeof(*newmem));
				newmem->size = size;
				
				newmem->next = cs;
				newmem->prev = cs->prev;
				cs->prev->next = newmem;
				cs->prev = newmem;
				
				Cache_MakeLRU (newmem);
	
				return newmem;
			}
		}

	// continue looking		
		newmem = (cache_system_t *)((byte *)cs + cs->size);
		cs = cs->next;

	} while (cs != &cache_head);
	
// try to allocate one at the very end
	if ( hunk_base + hunk_size - hunk_high_used - (byte *)newmem >= size)
	{
		memset (newmem, 0, sizeof(*newmem));
		newmem->size = size;
		
		newmem->next = &cache_head;
		newmem->prev = cache_head.prev;
		cache_head.prev->next = newmem;
		cache_head.prev = newmem;
		
		Cache_MakeLRU (newmem);

		return newmem;
	}
	
	return NULL;		// couldn't allocate
}

/*
============
Cache_Flush

Throw everything out, so newmem data will be demand cached
============
*/
void Cache_Flush (void)
{
	while (cache_head.next != &cache_head)
		Cache_Free ( cache_head.next->user );	// reclaim the space
}




/*
============
CacheSystemCompare

Compares the names of two cache_system_t structs.
Used with qsort()
============
*/

//needed for OutputDebugString
//#include <windows.h>
static int CacheSystemCompare(const void* ppcs1,const void* ppcs2)
{
	cache_system_t* pcs1=*(cache_system_t**)ppcs1;
	cache_system_t* pcs2=*(cache_system_t**)ppcs2;
//	char buf[400];
//	sprintf(buf,"Comparing \"%s\" and \"%s\"\n",pcs1->name,pcs2->name);
//	OutputDebugString(buf);
	

	return stricmp(pcs1->name,pcs2->name);
}	



/*
============
Cache_Print

============
*/
void Cache_Print (void)
{
	cache_system_t	*cd;
	cache_system_t *sortarray[512];
	int i=0,j=0;

	FileHandle_t file = g_pFileSystem->Open(mem_dbgfile.GetString(), "a");
	if (!file)
		return;

	
	memset(sortarray,sizeof(cache_system_t*)*512,0);
	
	g_pFileSystem->FPrintf(file,"\nCACHE:\n");

	for (cd = cache_head.next ; cd != &cache_head ; cd = cd->next)
	{
		sortarray[i++]=cd;
	}

	//Sort the array alphabetically
	qsort(sortarray,i,sizeof(cache_system_t*),CacheSystemCompare);
	
	for(j=0;j<i;j++)
		g_pFileSystem->FPrintf(file, "%16.16s : %-16s\n", Q_pretifymem(sortarray[j]->size), sortarray[j]->name);

	g_pFileSystem->Close(file);
}


/*
============
ComparePath1

compares the first directory of two paths...
(so  "foo/bar" will match "foo/fred"
============
*/
int ComparePath1(char* path1,char* path2)
{
	while (*path1!='/' && *path1!='\\' && *path1)
	{
		if (*path1 != *path2)
			return 0;
		else
		{
			path1++;
			path2++;
		}
	}
	return 1;
}

/*
============
Q_pretifymem

takes a number, and creates a string of that with commas in the 
appropriate places.
============
*/
char* Q_pretifymem(int num, char* pout)
{

	//this is probably more complex than it needs to be.
	int len=0;
	int i;
	char outbuf[50];
	memset(outbuf,0,50);
	while (num)
	{
		char tempbuf[50];
		int temp=num%1000;
		num=num/1000;
		strcpy(tempbuf,outbuf);

		Q_snprintf(outbuf, sizeof( outbuf ), ",%03i%s",temp,tempbuf);
	}

	len=strlen(outbuf);
	
	for (i=0;i<len;i++)				//find first significant digit
		if (outbuf[i]!='0' && outbuf[i]!=',')
			break;

	if (i==len)
		strcpy(pout,"0");
	else
		strcpy(pout,&outbuf[i]);	//copy from i to get rid of the first comma and leading zeros

	return pout;
}


/*
============
Cache_Report

============
*/
void Cache_Report (void)
{
	Con_DPrintf ("%4.1f megabyte data cache\n", (hunk_size - hunk_high_used - hunk_low_used) / (float)(1024*1024) );
}

/*
============
Cache_Compact

============
*/
void Cache_Compact (void)
{
}

/*
============
Cache_Init

============
*/
static ConCommand flush( "flush", Cache_Flush, "Flush cache memory." );

void Cache_Init (void)
{
	cache_head.next = cache_head.prev = &cache_head;
	cache_head.lru_next = cache_head.lru_prev = &cache_head;
}

/*
==============
Cache_Free

Frees the memory and removes it from the LRU list
==============
*/
void Cache_Free (cache_user_t *c)
{
	cache_system_t	*cs;

	if (!c->data)
		Sys_Error ("Cache_Free: not allocated");

	cs = ((cache_system_t *)c->data) - 1;

	cs->prev->next = cs->next;
	cs->next->prev = cs->prev;
	cs->next = cs->prev = NULL;

	c->data = NULL;

	Cache_UnlinkLRU (cs);
}

int Cache_TotalUsed(void)
{
	cache_system_t	*cd;
	int Total=0;
	for (cd = cache_head.next ; cd != &cache_head ; cd = cd->next)
			Total+=cd->size;

	return Total;
}
	



/*
==============
Cache_Check
==============
*/
void *Cache_Check (cache_user_t *c)
{
	cache_system_t	*cs;

	if (!c->data)
		return NULL;

	cs = ((cache_system_t *)c->data) - 1;

// move to head of LRU
	Cache_UnlinkLRU (cs);
	Cache_MakeLRU (cs);
	
	return c->data;
}


/*
==============
Cache_Alloc
==============
*/
void *Cache_Alloc (cache_user_t *c, int size, const char *name)
{
	cache_system_t	*cs;

	if (c->data)
		Sys_Error ("Cache_Alloc: already allocated");
	
	if (size <= 0)
		Sys_Error ("Cache_Alloc: size %i", size);

	size = (size + sizeof(cache_system_t) + 15) & ~15;

// find memory for it	
	while (1)
	{
		cs = Cache_TryAlloc (size, false);
		if (cs)
		{
			Q_strncpy(cs->name, name, sizeof(cs->name));
			c->data = (void *)(cs+1);
			cs->user = c;
			break;
		}

	// free the least recently used entry
		// assume if it's a locked entry that we've run out.
		if (cache_head.lru_prev == &cache_head || cache_head.lru_prev->locked)
			Sys_Error ("Cache_Alloc: out of memory");
		
		Cache_Free ( cache_head.lru_prev->user );
	}
	
	return Cache_Check (c);
}


void Cache_EnterCriticalSection( )
{
	cache_critical_section = 1;
}


void Cache_ExitCriticalSection( void )
{
	cache_critical_section = 0;

	cache_system_t	*cs = &cache_head;

	// clear all existing locked entries
	do
	{
		cs->locked = 0;
		cs = cs->lru_next;
	} while (cs != &cache_head && cs->locked);
}

//============================================================================

#ifdef _DEBUG
/*
=========================
MEM_Print

  Print out some information about memory
=========================
*/

typedef struct SMemoryBin
{
	char binName[ HUNK_NAME_LEN+1 ];
	long size;

	struct SMemoryBin *pNext;
} SMemoryBin;


void MEM_PrintHunk( void ) 
{ 
	SMemoryBin *pHunkBins = NULL, *pCur = NULL;
	FileHandle_t file = g_pFileSystem->Open(mem_dbgfile.GetString(), "a");

	hunk_t *h = (hunk_t *)hunk_base;

	hunk_t *endlow = (hunk_t *)(hunk_base + hunk_low_used);
	hunk_t *starthigh = (hunk_t *)(hunk_base + hunk_size - hunk_high_used);
	hunk_t *endhigh = (hunk_t *)(hunk_base + hunk_size);

	if (!file)
		return;

	g_pFileSystem->FPrintf(file, "\n\nHunk:\n\n" ); 

	while (1)
	{
		//
		// skip to the high hunk if done with low hunk
		//
		if ( h == endlow )
			h = starthigh;
		
		//
		// if totally done, break
		//
		if ( h == endhigh )
			break;

		// total up info
		pCur = pHunkBins;
		while( pCur )
		{
			if( !strncmp( pCur->binName, h->name, sizeof( h->name ) ) )
			{
				pCur->size += h->size;
				break;
			}

			pCur = pCur->pNext;
		}

		// If we ran out of bins, make a newmem one.
		if( !pCur )
		{
			pCur = (SMemoryBin *) malloc( sizeof( SMemoryBin ) );
			Q_strncpy( pCur->binName, h->name, sizeof(pCur->binName) );

			pCur->size = h->size;

			pCur->pNext = pHunkBins;
			pHunkBins = pCur;
		}

		// next block
		h = (hunk_t *)((byte *)h + h->size);
	}

	pCur = pHunkBins;
	while( pCur )
	{
		SMemoryBin *pLast = pCur;

		g_pFileSystem->FPrintf(file, "%16.16s : %16s \n", Q_pretifymem(pCur->size), pCur->binName);

		pCur = pCur->pNext;
		free( pLast );
	}

	pHunkBins = NULL;
	g_pFileSystem->Close(file);
//	Hunk_Print( 1 ); 
}


void Cache_Print_Models_And_Totals(void);
void Cache_Print_Sounds_And_Totals(void);

void MEM_PrintCache( void )
{
	Con_Printf( "\n\nCache:\n\n" ); 
	Cache_Print(); 

	Cache_Print_Sounds_And_Totals();
	Cache_Print_Models_And_Totals();
	Cache_Report();
}


void MEM_Summary(void)
{
	FileHandle_t file = g_pFileSystem->Open(mem_dbgfile.GetString(), "a");
	int CacheUsed=Cache_TotalUsed();
	char buf[50];
	if (!file)
		return;


	g_pFileSystem->FPrintf(file, "MEMORY SUMMARY:\n------------------------------------\n");
	g_pFileSystem->FPrintf(file, "\tTotal memory available:    %s\n",Q_pretifymem(hunk_size,buf));
	g_pFileSystem->FPrintf(file, "\tTotal used in low hunk:    %s\n",Q_pretifymem(hunk_low_used,buf));
	g_pFileSystem->FPrintf(file, "\tTotal used in high hunk:   %s\n",Q_pretifymem(hunk_high_used,buf));
	g_pFileSystem->FPrintf(file, "\tTotal cache space:         %s\n",Q_pretifymem(hunk_size - hunk_low_used - hunk_high_used,buf));
	g_pFileSystem->FPrintf(file, "\tTotal cache used:          %s\n",Q_pretifymem(CacheUsed,buf));
	g_pFileSystem->FPrintf(file, "\tTotal memory available:    %s\n",Q_pretifymem(hunk_size - hunk_low_used - hunk_high_used-CacheUsed,buf));
	g_pFileSystem->FPrintf(file, "------------------------------------\n\n");

	g_pFileSystem->Close(file);
}

void MEM_Print(void)
{
	Hunk_Print(0);
}

void MEM_PrintAll(void)
{
	Hunk_Print(1);
}

void MEM_All(void)
{
	MEM_Summary();
	FileHandle_t f = g_pFileSystem->Open(mem_dbgfile.GetString(),"a");

	if (!f)
		return;

	g_pFileSystem->FPrintf(f,"DETAILS:\n------------------------------------\n");
	g_pFileSystem->Close(f);

	Hunk_Print(0);
//	Hunk_Print(1);		//too verbose to be used generally.
	MEM_PrintHunk();
	MEM_PrintCache();
}

void MEM_PrintCacheSound(void)
{
	Cache_Print_Sounds_And_Totals();
}


void MEM_PrintCacheModels(void)
{
	Cache_Print_Models_And_Totals();
}


#endif		// _DEBUG

#ifdef _DEBUG
static ConCommand memhunk( "memhunk", MEM_PrintHunk, "Dump hunk memory info." );
static ConCommand memcache( "memcache", MEM_PrintCache, "Dump cache memory info." );
static ConCommand memprint( "memprint", MEM_Print, "Print memory contents." );
static ConCommand memprintall( "memprintall", MEM_PrintAll, "Print all memory contents." );
static ConCommand memall( "memall", MEM_All );
static ConCommand memsound( "memsound", MEM_PrintCacheSound, "Print sound cache memory." );
static ConCommand memmodels( "memmodels", MEM_PrintCacheModels, "Print model cache memory." );
#endif	_DEBUG

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Memory_Init ( void )
{
	hunk_base = (byte *)host_parms.membase;
	hunk_size = host_parms.memsize;
	hunk_low_used = 0;
	hunk_high_used = 0;
	
	Cache_Init ();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Memory_Shutdown( void )
{
	// TODO: Clear out the hunk?
}

void Cache_Print_Models_And_Totals (void)
{
	char buf[50];
	cache_system_t	*cd;
	cache_system_t *sortarray[512];
	long i=0,j=0;
	long totalbytes=0;
	FileHandle_t file = g_pFileSystem->Open(mem_dbgfile.GetString(), "a");
	int subtot=0;

	if (!file)
		return;



	memset(sortarray,sizeof(cache_system_t*)*512,0);

	//pack names into the array.
	for (cd = cache_head.next ; cd != &cache_head ; cd = cd->next)
	{
		if (strstr(cd->name,".mdl"))
		{
			sortarray[i++]=cd;
		}
	}

	qsort(sortarray,i,sizeof(cache_system_t*),CacheSystemCompare);




	g_pFileSystem->FPrintf(file,"\nCACHED MODELS:\n");

	
	//now process the sorted list.  
	for (j=0;j<i;j++)
	{
		int k;
		mstudiotexture_t	*ptexture;
		studiohdr_t			*phdr=(studiohdr_t*)sortarray[j]->user->data;


		ptexture = (mstudiotexture_t *)( ((char*)phdr) + phdr->textureindex);

		subtot=0;
		for (k = 0; k < phdr->numtextures; k++)
			subtot+=ptexture[k].width * ptexture[k].height+768; // (256*3 for the palette)

		g_pFileSystem->FPrintf(file, "\t%16.16s : %s\n", Q_pretifymem(sortarray[j]->size,buf),sortarray[j]->name);
		totalbytes+=sortarray[j]->size;
	}

	
	g_pFileSystem->FPrintf(file,"Total bytes in cache used by models:  %s\n",Q_pretifymem(totalbytes,buf));
	g_pFileSystem->Close(file);
}



/*
============
Cache_Print_Sounds_And_Totals

Prints out which sounds are in the cache, how much space they take, and the
directories that they're in.
============
*/

void Cache_Print_Sounds_And_Totals (void)
{
	char buf[50];
	cache_system_t	*cd;
	cache_system_t *sortarray[MAX_SFX];
	long i=0,j=0;
	long totalsndbytes=0;
	FileHandle_t file = g_pFileSystem->Open(mem_dbgfile.GetString(), "a");
	int subtot=0;

	if (!file)
		return;

	memset(sortarray,sizeof(cache_system_t*)*MAX_SFX,0);

	//pack names into the array.
	for (cd = cache_head.next ; cd != &cache_head ; cd = cd->next)
	{
		if (Q_stristr(cd->name,".wav"))
		{
			sortarray[i++]=cd;
		}
	}

	qsort(sortarray,i,sizeof(cache_system_t*),CacheSystemCompare);


	if (!file)
		return;
	g_pFileSystem->FPrintf(file,"\nCACHED SOUNDS:\n");

	
	//now process the sorted list.  (totals by directory)
	for (j=0;j<i;j++)
	{
		
		g_pFileSystem->FPrintf(file, "\t%16.16s : %s\n", Q_pretifymem(sortarray[j]->size,buf),sortarray[j]->name);
		totalsndbytes+=sortarray[j]->size;

#ifdef _WIN32
		if (j+1==i || ComparePath1(sortarray[j]->name,sortarray[j+1]->name)==0)
		{
			char pathbuf[512];
			_splitpath(sortarray[j]->name,NULL,pathbuf,NULL,NULL);
			g_pFileSystem->FPrintf(file, "\tTotal Bytes used in \"%s\": %s\n",pathbuf,Q_pretifymem(totalsndbytes-subtot,buf));
			subtot=totalsndbytes;
		}
#endif

	}
		


	g_pFileSystem->FPrintf(file,"Total bytes in cache used by sound:  %s\n",Q_pretifymem(totalsndbytes,buf));
	g_pFileSystem->Close(file);
}

int MEM_Summary_Console( void )
{
	Msg("MEMORY:  Engine hunk, cache, and zone:\n------------------------------------\n");
	Msg("\tTotal memory available:    %s\n",Q_pretifymem(hunk_size));
	Msg("\tTotal used in low hunk:    %s\n",Q_pretifymem(hunk_low_used));
	Msg("\tTotal used in high hunk:   %s\n",Q_pretifymem(hunk_high_used));
	Msg("\tTotal cache space:         %s\n",Q_pretifymem(hunk_size - hunk_low_used - hunk_high_used));
	Msg("\tTotal cache used:          %s\n",Q_pretifymem(Cache_TotalUsed()));
	Msg("\tTotal memory available:    %s\n",Q_pretifymem(hunk_size - hunk_low_used - hunk_high_used-Cache_TotalUsed()));

	Msg("------------------------------------\n");

	return hunk_size;
}
