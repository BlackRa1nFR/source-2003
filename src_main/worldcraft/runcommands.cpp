//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Handles running the OS commands for map compilation.
//
// $Workfile: $
// $Date: 2000/07/25 04:33:08 $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include <afxtempl.h>
#include "RunCommands.h"
#include "Options.h"
#include <process.h>
#include "ProcessWnd.h"
#include <io.h>
#include <direct.h>
#include "GlobalFunctions.h"
#include "Worldcraft.h"


static char *pszDocPath, *pszDocName, *pszDocExt;

CProcessWnd procWnd;

void FixGameVars(char *pszSrc, char *pszDst, BOOL bUseQuotes)
{
	// run through the parms list and substitute $variable strings for
	//  the real thing
	char *pSrc = pszSrc, *pDst = pszDst;
	BOOL bInQuote = FALSE;
	while(pSrc[0])
	{	
		if(pSrc[0] == '$')	// found a parm
		{
			if(pSrc[1] == '$')	// nope, it's a single symbol
			{
				*pDst++ = '$';
				++pSrc;
			}
			else
			{
				// figure out which parm it is .. 
				++pSrc;
				
				if(!bInQuote && bUseQuotes)
				{
					// not in quote, and subbing a variable.. start quote
					*pDst++ = '\"';
					bInQuote = TRUE;
				}

				if(!strnicmp(pSrc, "file", 4))
				{
					pSrc += 4;
					strcpy(pDst, pszDocName);
					pDst += strlen(pDst);
				}
				else if(!strnicmp(pSrc, "ext", 3))
				{
					pSrc += 3;
					strcpy(pDst, pszDocExt);
					pDst += strlen(pDst);
				}
				else if(!strnicmp(pSrc, "path", 4))
				{
					pSrc += 4;
					strcpy(pDst, pszDocPath);
					pDst += strlen(pDst);
				}
				else if(!strnicmp(pSrc, "exedir", 6))
				{
					pSrc += 6;
					strcpy(pDst, g_pGameConfig->m_szGameExeDir);
					pDst += strlen(pDst);
				}
				else if(!strnicmp(pSrc, "bspdir", 6))
				{
					pSrc += 6;
					strcpy(pDst, g_pGameConfig->szBSPDir);
					pDst += strlen(pDst);
				}
				else if(!strnicmp(pSrc, "csg_exe", 7))
				{
					pSrc += 7;
					strcpy(pDst, g_pGameConfig->m_szCSG);
					pDst += strlen(pDst);
				}
				else if(!strnicmp(pSrc, "bsp_exe", 7))
				{
					pSrc += 7;
					strcpy(pDst, g_pGameConfig->szBSP);
					pDst += strlen(pDst);
				}
				else if(!strnicmp(pSrc, "vis_exe", 7))
				{
					pSrc += 7;
					strcpy(pDst, g_pGameConfig->szVIS);
					pDst += strlen(pDst);
				}
				else if(!strnicmp(pSrc, "light_exe", 9))
				{
					pSrc += 9;
					strcpy(pDst, g_pGameConfig->szLIGHT);
					pDst += strlen(pDst);
				}
				else if(!strnicmp(pSrc, "game_exe", 8))
				{
					pSrc += 8;
					strcpy(pDst, g_pGameConfig->szExecutable);
					pDst += strlen(pDst);
				}
				else if (!strnicmp(pSrc, "gamedir", 7))
				{
					pSrc += 7;
					strcpy(pDst, g_pGameConfig->m_szGameDir);
					pDst += strlen(pDst);
				}
				else if (!strnicmp(pSrc, "moddir", 7))
				{
					pSrc += 7;
					strcpy(pDst, g_pGameConfig->m_szModDir);
					pDst += strlen(pDst);
				}
			}
		}
		else
		{
			if(*pSrc == ' ' && bInQuote)
			{
				bInQuote = FALSE;
				*pDst++ = '\"';	// close quotes
			}

			// just copy the char into the destination buffer
			*pDst++ = *pSrc++;
		}
	}

	if(bInQuote)
	{
		bInQuote = FALSE;
		*pDst++ = '\"';	// close quotes
	}

	pDst[0] = 0;
}

static void RemoveQuotes(char *pBuf)
{
	if(pBuf[0] == '\"')
		strcpy(pBuf, pBuf+1);
	if(pBuf[strlen(pBuf)-1] == '\"')
		pBuf[strlen(pBuf)-1] = 0;
}

LPCTSTR GetErrorString()
{
	static char szBuf[200];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, 
		szBuf, 200, NULL);
	char *p = strchr(szBuf, '\r');	// get rid of \r\n
	if(p) p[0] = 0;
	return szBuf;
}

BOOL RunCommands(CCommandArray& Commands, LPCTSTR pszOrigDocName)
{
	int iDisk = _getdrive();
	char szCurDir[MAX_PATH];
	_getcwd(szCurDir, MAX_PATH);

	procWnd.GetReady();

	// cut up document name into file and extension components.
	//  create two sets of buffers - one set with the long filename
	//  and one set with the 8.3 format.

	char szDocLongPath[MAX_PATH], szDocLongName[MAX_PATH], 
		szDocLongExt[MAX_PATH];
	char szDocShortPath[MAX_PATH], szDocShortName[MAX_PATH], 
		szDocShortExt[MAX_PATH];

	GetFullPathName(pszOrigDocName, MAX_PATH, szDocLongPath, NULL);
	GetShortPathName(pszOrigDocName, szDocShortPath, MAX_PATH);

	// split them up
	char *p = strrchr(szDocLongPath, '.');
	if(p)
	{
		// got the extension
		strcpy(szDocLongExt, p+1);
		p[0] = 0;
	}

	p = strrchr(szDocLongPath, '\\');
	if(!p)
		p = strrchr(szDocLongPath, '/');
	if(p)
	{
		// got the filepart
		strcpy(szDocLongName, p+1);
		p[0] = 0;
	}

	// split the short part up
	p = strrchr(szDocShortPath, '.');
	if(p)
	{
		// got the extension
		strcpy(szDocShortExt, p+1);
		p[0] = 0;
	}

	p = strrchr(szDocShortPath, '\\');
	if(!p)
		p = strrchr(szDocShortPath, '/');
	if(p)
	{
		// got the filepart
		strcpy(szDocShortName, p+1);
		p[0] = 0;
	}

	int iSize = Commands.GetSize(), i = 0;
	char *ppParms[32];
	while(iSize--)
	{
		CCOMMAND &cmd = Commands[i++];

		// anything there?
		if((!cmd.szRun[0] && !cmd.iSpecialCmd) || !cmd.bEnable)
			continue;

		// set name pointers based on cmd.bLongFilenames
		if(cmd.bLongFilenames)
		{
			pszDocExt = szDocLongExt;
			pszDocName = szDocLongName;
			pszDocPath = szDocLongPath;
		}
		else
		{
			pszDocExt = szDocShortExt;
			pszDocName = szDocShortName;
			pszDocPath = szDocShortPath;
		}
		
		char szNewParms[MAX_PATH], szNewRun[MAX_PATH];

		FixGameVars(cmd.szRun, szNewRun, FALSE);
		FixGameVars(cmd.szParms, szNewParms, cmd.bLongFilenames);

		CString strTmp;
		strTmp.Format("\r\n"
			"** Executing...\r\n"
			"** Command: %s\r\n"
			"** Parameters: %s\r\n\r\n", szNewRun, szNewParms);
		procWnd.Append(strTmp);
		
		// create a parameter list (not always required)
		if(!cmd.bUseProcessWnd || cmd.iSpecialCmd)
		{
			char *p = szNewParms;
			ppParms[0] = szNewRun;
			int iArg = 1;
			BOOL bDone = FALSE;
			while(p[0])
			{
				ppParms[iArg++] = p;
				while(p[0])
				{
					if(p[0] == ' ')
					{
						// found a space-separator
						p[0] = 0;

						p++;

						// skip remaining white space
						while (*p == ' ')
							p++;

						break;
					}

					// found the beginning of a quoted parameters
					if(p[0] == '\"')
					{
						while(1)
						{
							p++;
							if(p[0] == '\"')
							{
								// found the end
								if(p[1] == 0)
									bDone = TRUE;
								p[1] = 0;	// kick its ass
								p += 2;

								// skip remaining white space
								while (*p == ' ')
									p++;

								break;
							}
						}
						break;
					}

					// else advance p
					++p;
				}

				if(!p[0] || bDone)
					break;	// done.
			}

			ppParms[iArg] = NULL;

			if(cmd.iSpecialCmd)
			{
				BOOL bError = FALSE;
				LPCTSTR pszError;

				if(cmd.iSpecialCmd == CCCopyFile && iArg == 3)
				{
					RemoveQuotes(ppParms[1]);
					RemoveQuotes(ppParms[2]);
					
					// don't copy if we're already there
					if (stricmp(ppParms[1], ppParms[2]) && 					
							(!CopyFile(ppParms[1], ppParms[2], FALSE)))
					{
						bError = TRUE;
						pszError = GetErrorString();
					}
				}
				else if(cmd.iSpecialCmd == CCDelFile && iArg == 2)
				{
					RemoveQuotes(ppParms[1]);
					if(!DeleteFile(ppParms[1]))
					{
						bError = TRUE;
						pszError = GetErrorString();
					}
				}
				else if(cmd.iSpecialCmd == CCRenameFile && iArg == 3)
				{
					RemoveQuotes(ppParms[1]);
					RemoveQuotes(ppParms[2]);
					if(rename(ppParms[1], ppParms[2]))
					{
						bError = TRUE;
						pszError = strerror(errno);
					}
				}
				else if(cmd.iSpecialCmd == CCChangeDir && iArg == 2)
				{
					RemoveQuotes(ppParms[1]);
					if(mychdir(ppParms[1]) == -1)
					{
						bError = TRUE;
						pszError = strerror(errno);
					}
				}

				if(bError)
				{
					CString str;
					str.Format("The command failed. Windows reported the error:\r\n"
						"  \"%s\"\r\n", pszError);
					procWnd.Append(str);
					procWnd.SetForegroundWindow();
					str += "\r\nDo you want to continue?";
					if(AfxMessageBox(str, MB_YESNO) == IDNO)
						break;
				}
			}
			else
			{
				// YWB Force asynchronous operation so that engine doesn't hang on
				//  exit???  Seems to work.
				_spawnv(/*cmd.bNoWait ?*/ P_NOWAIT /*: P_WAIT*/, szNewRun, 
					(const char *const *)ppParms);
			}
		}
		else
		{
			procWnd.Execute(szNewRun, szNewParms);
		}

		// check for existence?
		if(cmd.bEnsureCheck)
		{
			char szFile[MAX_PATH];
			FixGameVars(cmd.szEnsureFn, szFile, FALSE);
			if(GetFileAttributes(szFile) == 0xFFFFFFFF)
			{
				// not there!
				CString str;
				str.Format("The file '%s' was not built.\n"
					"Do you want to continue?", szFile);
				procWnd.SetForegroundWindow();
				if(AfxMessageBox(str, MB_YESNO) == IDNO)
					break;	// outta here
			}
		}
	}

	mychdir(szCurDir);

	return TRUE;
}

