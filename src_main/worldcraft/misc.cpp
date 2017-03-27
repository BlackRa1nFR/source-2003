//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Miscellaneous utility functions.
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include <direct.h>
#include <time.h>
#include "MapSolid.h"


static DWORD holdrand;


void randomize()
{
	holdrand = DWORD(time(NULL));
}


DWORD random()
{
	return(holdrand = holdrand * 214013L + 2531011L);
}


// MapCheckDlg.cpp:
BOOL DoesContainDuplicates(CMapSolid *pSolid);
static BOOL bCheckDupes = FALSE;


void NotifyDuplicates(CMapSolid *pSolid)
{
	if(!bCheckDupes)
		return;	// stop that

	if(DoesContainDuplicates(pSolid))
	{
		if(IDNO == AfxMessageBox("Duplicate Plane! Do you want more messages?", 
			MB_YESNO))
		{
			bCheckDupes = FALSE;
		}
	}
}


void NotifyDuplicates(CMapObjectList *pList)
{
	if(!bCheckDupes)
		return;	// stop that

	POSITION p = pList->GetHeadPosition();
	while(p)
	{
		CMapClass *pobj = pList->GetNext(p);
		if(!pobj->IsMapClass(MAPCLASS_TYPE(CMapSolid)))
			continue;	// not a solid
		NotifyDuplicates((CMapSolid*) pobj);
	}
}


int mychdir(LPCTSTR pszDir)
{
	int curdrive = _getdrive();

	// changes to drive/directory
	if(pszDir[1] == ':' && _chdrive(toupper(pszDir[0]) - 'A' + 1) == -1)
		return -1;
	if(_chdir(pszDir) == -1)
	{
		// change back to original disk
		_chdrive(curdrive);
		return -1;
	}

	return 0;
}


void WriteDebug(char *pszStr)
{
#if 0
	static BOOL bFirst = TRUE;
	
	if(bFirst)
		remove("wcdebug.txt");

	bFirst = FALSE;

	FILE *fp = fopen("wcdebug.txt", "ab");
	fprintf(fp, "%s\r\n", pszStr);
	fclose(fp);
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Case-insensitive search for a substring within a string.
// Input  : str1 - string to search in.
//			str2 - substring to search for.
// Output : Returns a pointer to the location within str1 where str2 occurs, NULL if none.
//-----------------------------------------------------------------------------
const char *stristr(const char *str1, const char *str2)
{
	if ((str1 != NULL) && (str2 != NULL))
	{
		while (*str1 != '\0')
		{
			const char *p1 = str1;
			const char *p2 = str2;

			while ((*p2 != '\0') && (tolower(*p1) == tolower(*p2)))
			{
				p1++;
				p2++;
			}

			if (*p2 == '\0')
			{
				return(str1);
			}

			str1++;
		}
	}
	
	return(NULL);
}
