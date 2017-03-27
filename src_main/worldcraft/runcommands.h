
#ifndef _RUNCOMMANDS_H
#define _RUNCOMMANDS_H

#include <afxtempl.h>

//
// RunCommands functions
//

enum
{
	CCChangeDir = 0x100,
	CCCopyFile,
	CCDelFile,
	CCRenameFile
};

// command:
typedef struct
{
	BOOL bEnable;	// run this command?

	int iSpecialCmd;	// non-0 if special command exists
	char szRun[MAX_PATH];
	char szParms[MAX_PATH];
	BOOL bLongFilenames;
	BOOL bEnsureCheck;
	char szEnsureFn[MAX_PATH];
	BOOL bUseProcessWnd;
	BOOL bNoWait;

} CCOMMAND, *PCCOMMAND;

// list of commands:
typedef CArray<CCOMMAND, CCOMMAND&> CCommandArray;

// run a list of commands:
BOOL RunCommands(CCommandArray& Commands, LPCTSTR pszDocName);
void FixGameVars(char *pszSrc, char *pszDst, BOOL bUseQuotes = TRUE);

#endif // _RUNCOMMANDS_H