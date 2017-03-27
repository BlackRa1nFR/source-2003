//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "testscriptmgr.h"
#include "tier0/dbg.h"
#include "filesystem_engine.h"
#include "vstdlib/strtools.h"
#include "cmd.h"
#include "convar.h"
#include "vstdlib/random.h"
#include <stdlib.h>


CTestScriptMgr g_TestScriptMgr;


// --------------------------------------------------------------------------------------------------- //
// Global console commands the test script manager implements.
// --------------------------------------------------------------------------------------------------- //

void Test_Wait()
{
	if ( Cmd_Argc() < 2 )
	{
		Error( "Test_Wait: requires seconds parameter." );
		return;
	}

	float flSeconds = atof( Cmd_Argv( 1 ) );
	GetTestScriptMgr()->SetWaitTime( flSeconds );
}

void Test_RunFrame()
{
	GetTestScriptMgr()->SetWaitCheckPoint( "frame_end" );
}

void Test_WaitForCheckPoint()
{
	if ( Cmd_Argc() < 2 )
	{
		Error( "Test_WaitForCheckPoint: requires checkpoint name." );
		return;
	}

	GetTestScriptMgr()->SetWaitCheckPoint( Cmd_Argv( 1 ) );
}

void Test_StartLoop()
{
	if ( Cmd_Argc() < 2 )
	{
		Error( "Test_StartLoop: requires a loop name." );
		return;
	}

	GetTestScriptMgr()->StartLoop( Cmd_Argv( 1 ) );
}

void Test_LoopCount()
{
	if ( Cmd_Argc() < 3 )
	{
		Error( "Test_LoopCount: requires a loop name and number of times to loop." );
		return;
	}

	GetTestScriptMgr()->LoopCount( Cmd_Argv( 1 ), atoi( Cmd_Argv( 2 ) ) );
}

void Test_LoopForNumSeconds()
{
	if ( Cmd_Argc() < 3 )
	{
		Error( "Test_LoopLoopForNumSeconds: requires a loop name and number of seconds to loop." );
		return;
	}

	GetTestScriptMgr()->LoopForNumSeconds( Cmd_Argv( 1 ), atof( Cmd_Argv( 2 ) ) );
}

void Test_RandomChance()
{
	if ( Cmd_Argc() < 3 )
	{
		Error( "Test_RandomChance: requires percentage chance parameter (0-100) followed by command to execute." );
	}

	float flPercent = atof( Cmd_Argv ( 1 ) );
	if ( RandomFloat( 0, 100 ) < flPercent )
	{
		char newString[1024];
		newString[0] = 0;

		for ( int i=2; i < Cmd_Argc(); i++ )
		{
			Q_strncat( newString, Cmd_Argv( i ), sizeof( newString ) );
			Q_strncat( newString, " ", sizeof( newString ) );
		}

		Cmd_ExecuteString( newString, src_command );
	}
}

ConCommand cc_Test_Wait( "Test_Wait", Test_Wait );

ConCommand cc_Test_StartLoop( "Test_StartLoop", Test_StartLoop, "Test_StartLoop <loop name> - Denote the start of a loop. Really just defines a named point you can jump to." );
ConCommand cc_Test_LoopCount( "Test_LoopCount", Test_LoopCount, "Test_LoopCount <loop name> <count> - loop back to the specified loop start point the specified # of times." );
ConCommand cc_Test_LoopForNumSeconds( "Test_LoopForNumSeconds", Test_LoopForNumSeconds, "Test_LoopForNumSeconds <loop name> <time> - loop back to the specified start point for the specified # of seconds." );

ConCommand cc_Test_RandomChance( "Test_RandomChance", Test_RandomChance, "Test_RandomChance <percent chance, 0-100> <token1> <token2...> - Roll the dice and maybe run the command following the percentage chance." );

ConCommand cc_Test_RunFrame( "Test_RunFrame", Test_RunFrame );
ConCommand cc_Test_WaitForCheckPoint( "Test_WaitForCheckPoint", Test_WaitForCheckPoint );


// --------------------------------------------------------------------------------------------------- //
// CTestScriptMgr implementation.
// --------------------------------------------------------------------------------------------------- //

CTestScriptMgr::CTestScriptMgr()
{
	m_hFile = FILESYSTEM_INVALID_HANDLE;
	m_NextCheckPoint[0] = 0;
	m_WaitUntil = Sys_FloatTime();
}


CTestScriptMgr::~CTestScriptMgr()
{
	Term();
}


bool CTestScriptMgr::StartTestScript( const char *pFilename )
{
	Term();

	char fullName[512];
	Q_snprintf( fullName, sizeof( fullName ), "TestScripts\\%s", pFilename );

	m_hFile = g_pFileSystem->Open( fullName, "rt" );
	if ( m_hFile == FILESYSTEM_INVALID_HANDLE )
		return false;

	RunCommands();	
	return true;
}


void CTestScriptMgr::Term()
{
	if ( m_hFile != FILESYSTEM_INVALID_HANDLE )
	{
		g_pFileSystem->Close( m_hFile );
		m_hFile = FILESYSTEM_INVALID_HANDLE;
	}
}


bool CTestScriptMgr::IsInitted() const
{
	return m_hFile != FILESYSTEM_INVALID_HANDLE;
}


void CTestScriptMgr::CheckPoint( const char *pName )
{
	if ( !IsInitted() || IsTimerWaiting() )
		return;

	if ( m_NextCheckPoint[0] )
	{
		if ( Q_stricmp( m_NextCheckPoint, pName ) != 0 )
		{
			// This isn't the checkpoint you're looking for.
			return;
		}
	}

	// Either the timer expired, or we hit the checkpoint we were waiting for. Run some more commands.
	m_NextCheckPoint[0] = 0;
	RunCommands();
}


void CTestScriptMgr::RunCommands()
{
	Assert( IsInitted() );

	while ( !IsTimerWaiting() && !IsCheckPointWaiting() )
	{
		// Parse out the next command.
		char curCommand[512];
		int iCurPos = 0;

		bool bInSlash = false;
		while ( 1 )
		{
			g_pFileSystem->Read( &curCommand[iCurPos], 1, m_hFile );
			if ( curCommand[iCurPos] == '/' )
			{
				if ( bInSlash )
				{
					// Ok, the rest of this line is a comment.
					char tempVal = !'\n';
					while ( tempVal != '\n' && !g_pFileSystem->EndOfFile( m_hFile ) )
					{
						g_pFileSystem->Read( &tempVal, 1, m_hFile );
					}

					--iCurPos;
					break;
				}
				else
				{
					bInSlash = true;
				}
			}
			else
			{
				bInSlash = false;

				if ( curCommand[iCurPos] == ';' || curCommand[iCurPos] == '\n' || g_pFileSystem->EndOfFile( m_hFile ) )
				{
					// End of this command.
					break;
				}
			}

			++iCurPos;
		}

		curCommand[iCurPos] = 0;
		
		// Did we hit the end of the file?
		if ( curCommand[0] == 0 )
		{
			if ( g_pFileSystem->EndOfFile( m_hFile ) )
			{
				Term();
				break;
			}
			else
			{
				continue;
			}			
		}

		Cmd_ExecuteString( curCommand, src_command );
	}
}


bool CTestScriptMgr::IsTimerWaiting() const
{
	if ( Sys_FloatTime() < m_WaitUntil )
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool CTestScriptMgr::IsCheckPointWaiting() const
{
	return m_NextCheckPoint[0] != 0;
}


void CTestScriptMgr::SetWaitTime( float flSeconds )
{
	m_WaitUntil = Sys_FloatTime() + flSeconds;
}


CLoopInfo* CTestScriptMgr::FindLoop( const char *pLoopName )
{
	FOR_EACH_LL( m_Loops, i )
	{
		if ( Q_stricmp( pLoopName, m_Loops[i]->m_Name ) == 0 )
			return m_Loops[i];
	}
	return NULL;
}


void CTestScriptMgr::StartLoop( const char *pLoopName )
{
	ErrorIfNotInitted();

	if ( FindLoop( pLoopName ) )
	{
		Error( "CTestScriptMgr::StartLoop( %s ): loop already exists.", pLoopName );
	}

	CLoopInfo *pLoop = new CLoopInfo;
	Q_strncpy( pLoop->m_Name, pLoopName, sizeof( pLoop->m_Name ) );
	pLoop->m_nCount = 0;
	pLoop->m_flStartTime = Sys_FloatTime();
	pLoop->m_iNextCommandPos = g_pFileSystem->Tell( m_hFile );
	pLoop->m_ListIndex = m_Loops.AddToTail( pLoop );
}


void CTestScriptMgr::LoopCount( const char *pLoopName, int nTimes )
{
	ErrorIfNotInitted();

	CLoopInfo *pLoop = FindLoop( pLoopName );
	if ( !pLoop )
	{
		Error( "CTestScriptMgr::LoopCount( %s ): no loop with this name exists.", pLoopName );
	}

	++pLoop->m_nCount;
	if ( pLoop->m_nCount < nTimes )
	{
		g_pFileSystem->Seek( m_hFile, pLoop->m_iNextCommandPos, FILESYSTEM_SEEK_HEAD );
	}
	else
	{
		m_Loops.Remove( pLoop->m_ListIndex );
		delete pLoop;
	}
}


void CTestScriptMgr::LoopForNumSeconds( const char *pLoopName, double nSeconds )
{
	ErrorIfNotInitted();

	CLoopInfo *pLoop = FindLoop( pLoopName );
	if ( !pLoop )
	{
		Error( "CTestScriptMgr::LoopForNumSeconds( %s ): no loop with this name exists.", pLoopName );
	}

	if ( Sys_FloatTime() - pLoop->m_flStartTime < nSeconds )
	{
		g_pFileSystem->Seek( m_hFile, pLoop->m_iNextCommandPos, FILESYSTEM_SEEK_HEAD );
	}
	else
	{
		m_Loops.Remove( pLoop->m_ListIndex );
		delete pLoop;
	}
}


void CTestScriptMgr::ErrorIfNotInitted()
{
	if ( !IsInitted() )
	{
		Error( "CTestScriptMgr: not initialized." );
	}
}


void CTestScriptMgr::SetWaitCheckPoint( const char *pCheckPointName )
{
	Q_strncpy( m_NextCheckPoint, pCheckPointName, sizeof( m_NextCheckPoint ) );
}
