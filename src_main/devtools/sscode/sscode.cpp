#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cmdline.h"
#include "sys.h"

// for STL - generates really long identifiers!
#pragma warning(disable:4786)

#include <string>
#include <vector>

const char *GetUsageString( void )
{
	return "SSCODE -user <User> [-ssdir <ssdir>] [-r] [-vssdate <VSS Date Range> | -weeks <number of weeks> | -addcurrent] <$project>\n"
		"\n----------\n"
		"-user <VSS User name>\te.g. -user Jay\n"
		"-ssdir <VSS dir>\te.g. -ssdir \\\\jeeves\\vsscode (default)\n"
		"-r \t\t\t(optional) recurse subprojects\n"
		"-vssdate <vssdate>\t(optional), a string in the VSS date range format\n\t\t\te.g. -vssdate 11/1/99:3:30p~10/15/99:2:11p\n\n"
		"-weeks <n>\t\te.g. number of weeks (from the most recent sunday) to report\n"
		"-addcurrent\t\t(optional), adds the current, incomplete week to the report\n"
		"<$project>\t\t(required), this is the VSS project name\n\t\t\tmust be the last argument, e.g. $/src/engine\n";
}

typedef std::basic_string<char> string;
struct historyList
{
	historyList( string name, int ver ) : file(name), version(ver) {}
	string	file;
	int		version;
};

typedef std::vector<historyList> STRING_LIST;

void SubtractTime( const tm *last, tm *first, int time_t_time );

// time_t units are seconds
#define TIME_T_MINUTE		60
#define TIME_T_HOUR			(TIME_T_MINUTE*60)
#define TIME_T_DAY			(TIME_T_HOUR*24)
#define TIME_T_WEEK			(TIME_T_DAY*7)

//-----------------------------------------------------------------------------
// Purpose: Set "date" to now
// Input  : *date - 
//-----------------------------------------------------------------------------
void SetToday( struct tm *date )
{
	time_t today;

	time( &today );
	tm *newTime = localtime( &today );
	memcpy( date, newTime, sizeof(*date) );
}

//-----------------------------------------------------------------------------
// Purpose: Set to the most recently past Sunday
// Input  : *date - 
//-----------------------------------------------------------------------------
void SetSunday( struct tm *date )
{
	int days = date->tm_wday;

	tm newDate;
	SubtractTime( date, &newDate, TIME_T_DAY * days );

	memcpy( date, &newDate, sizeof(*date) );
}


//-----------------------------------------------------------------------------
// Purpose: Subtract seconds from a time and fixup calendar date
// Input  : *source - source time
//			*destination - output time
//			deltaSeconds - number of seconds
//-----------------------------------------------------------------------------
void SubtractTime( const tm *source, tm *destination, int deltaSeconds )
{
	time_t tempTime = mktime( (tm *)source );
	tempTime -= deltaSeconds;

	tm *newTime = localtime( &tempTime );
	memcpy( destination, newTime, sizeof(*destination) );
}


//-----------------------------------------------------------------------------
// Purpose: Subtract n weeks from the date in "last" to compute the date stored in "first"
// Input  : *last - end of the range
//			*first - last - "weeks"
//			weeks - number of weeks to subtract
//-----------------------------------------------------------------------------
void SubtractWeeks( struct tm *last, struct tm *first, int weeks )
{
	SubtractTime( last, first, TIME_T_WEEK * weeks );
}


//-----------------------------------------------------------------------------
// Purpose: formatted output of the input date
// Input  : &date - 
// Output : string in vss date/time format
//-----------------------------------------------------------------------------
string TmToString( struct tm &date )
{
	char buf[128];
	int hour = date.tm_hour;

	const char *ampm = "a";
	if ( hour >= 12 )
	{
		if ( hour > 12 )
			hour -= 12;
		ampm = "p";
	}
	else if ( hour == 0 )
	{
		hour = 12;
	}

//	sprintf( buf, "%d/%d/%d;%d:%02d%s", date.tm_mon+1, date.tm_mday, date.tm_year+1900, hour, date.tm_min, ampm );

	// skip the time for now, it seems to cause VSS to go into a different mode
	sprintf( buf, "%d/%d/%d", date.tm_mon+1, date.tm_mday, date.tm_year+1900 );

	return string(buf);
}


char *StringSkipTo( char *pStr, char pSkip )
{
	int done = 0;
	while ( *pStr && !done )
	{
		if ( *pStr == pSkip )
			done = 1;

		pStr++;
	}

	return pStr;
}

//-----------------------------------------------------------------------------
// Purpose: Parse the VSS history command and generate a list of files & versions
// Input  : *pFileName - 
// Output : STRING_LIST
//-----------------------------------------------------------------------------
STRING_LIST &ParseHistory( const char *pProjectName, const char *pFileName )
{
	int nameLen = strlen( pProjectName );
	STRING_LIST *list = new STRING_LIST;

	FILE *fp = fopen( pFileName, "r" );
	if ( fp )
	{
		char buf[1024];
		string currentFile;
		string project;
		int currentVersion;

		while ( !feof(fp) )
		{
			fgets( buf, 1024, fp );
			if ( !strncmp( buf, "*****", 5 ) )
			{
				char *pStr = buf + 7;
				strtok( pStr, " \n" );
				currentFile = pStr;
			}
			else if ( !strncmp( buf, "Version", 7 ) )
			{
				currentVersion = atoi( buf + 8 );
			}
			else if ( !strncmp( buf, "Checked in", 10 ) )
			{
				char *pStr = buf + 11;
				strtok( pStr, " \n" );

				if ( strncmp( pProjectName, pStr, nameLen ) )
				{
					project = pProjectName;
					pStr = StringSkipTo( pStr, '/' ); // skip $/
					pStr = StringSkipTo( pStr, '/' );	// skip xxx/
					project += "/";
					project += pStr;
				}
				else
				{
					project = pStr;
				}

				project += "/";
				project += currentFile;
				historyList tmp( project, currentVersion );
				list->push_back( tmp );
			}
		}
		fclose( fp );
	}
	return *list;
}


//-----------------------------------------------------------------------------
// Purpose: counts the lines inserted or changed in the diff output
// Input  : *pFileName - name of file containing the output of a diff
// Output : int - count of lines added/changed
//-----------------------------------------------------------------------------
int CountDiffs( const char *pFileName )
{
	int count = 0;

	FILE *fp = fopen( pFileName, "r" );
	if ( fp )
	{
		char buf[1024];
		while ( !feof(fp) )
		{
			fgets( buf, 1024, fp );
			if ( buf[0] == '>' )
			{
				count++;
			}
		}
		fclose( fp );
	}

	return count;
}


//-----------------------------------------------------------------------------
// Purpose: wrapper to run source safe
// Input  : *pCmdLine - 
//-----------------------------------------------------------------------------
void RunVSS( const char *pCmdLine )
{
	Sys_Exec( "ss.EXE", pCmdLine );
}

//-----------------------------------------------------------------------------
// Purpose: runs diff on all changed files in the date range
// Input  : argc - 
//			*argv[] - 
// Output : int
//-----------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	char tmpBuf[1024];

	if ( argc < 2 )
	{
		Error( "Incorrect usage\n" );
	}

	ArgsInit( argc, argv );
	const char *pSSDir = ArgsGet( "-ssdir", "\\\\jeeves\\vsscode" );

	sprintf( tmpBuf, "ssdir=%s", pSSDir );
	putenv( tmpBuf );

	const char *pSSProject = argv[argc-1];
	const char *pSSUser = ArgsGet( "-user", "Jay" );

	printf("Changes for %s\n", pSSUser );
	const char *pSSTmpFile = _tempnam( ".", "ssc" );

	const char *pRecurse = "";
	if ( ArgsExist( "-r" ) || ArgsExist( "-R" ) )
	{
		pRecurse = "-R";
	}

	const char *pDateRange = ArgsGet( "-vssdate", NULL );
	char dateBuf[128];

	if ( !pDateRange )
	{
		pDateRange = ArgsGet( "-weeks", "1" );
		int weeks = atoi( pDateRange );

		struct tm last, first;
		SetToday( &last );
		SetSunday( &last );
		SubtractWeeks( &last, &first, weeks );
		if ( ArgsExist( "-addcurrent" ) )
		{
			SetToday( &last );
		}
		string end = TmToString(last);
		string start = TmToString( first );
		sprintf( dateBuf, "%s~%s", &end[0], &start[0] );
		pDateRange = dateBuf;
		printf("From %s to %s\n", &start[0], &end[0] );
	}
	else
	{
		printf("Using custom vss date range: %s\n", pDateRange );
	}

	sprintf( tmpBuf, "history %s %s -vd%s -u%s >%s", pRecurse, pSSProject, pDateRange, pSSUser, pSSTmpFile );
	RunVSS( tmpBuf );

	STRING_LIST list = ParseHistory( pSSProject, pSSTmpFile );

	int totalLines = 0;
	for ( int i = 0; i < list.size(); i++ )
	{
		const char *pFileName = (const char *)&((list[i].file)[0]);
		sprintf( tmpBuf, "Diff -IEW %s -V%d~%d -DU >%s", pFileName, list[i].version-1, list[i].version, pSSTmpFile );
		RunVSS( tmpBuf );
		int linesChanged = CountDiffs( pSSTmpFile );
		printf( "Changed %-32s (ver %3d) \t%4d lines\n", pFileName, list[i].version, linesChanged );
		totalLines += linesChanged;
	}

	unlink( pSSTmpFile );
	printf( "\nTotal Files changed: %d\n", list.size() );
	printf( "Total Lines changed: %d\n\n", totalLines );
	return 0;
}
