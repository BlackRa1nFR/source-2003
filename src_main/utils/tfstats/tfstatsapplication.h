//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of the TFStatsApplication class. 
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef TFSTATSAPPLICATION_H
#define TFSTATSAPPLICATION_H
#ifdef WIN32
#pragma once
#endif
#include <string>
using std::string;
#include "util.h"
#include "HTML.h"
#include "TFStatsOSInterface.h"

//------------------------------------------------------------------------------------------------------
// Purpose:  Instances of this class contain information that is specific to one run
//of TFStats.  This serves as the main entry point for the program as well. 
//------------------------------------------------------------------------------------------------------
class CTFStatsApplication
{
public:
	CTFStatsOSInterface* os;
	string outputDirectory;
	string inputDirectory;
	string ruleDirectory;
	string supportDirectory;
	string supportHTTPPath;
	string playerDirectory;
	string playerHTTPPath;

	string logFileName;

	bool eliminateOldPlayers;
	
	int elimDays;
	time_t getCutoffSeconds();	
	
	void makeAndSaveDirectory(string& dir);
	void makeDirectory(string& dir);

	//command line switches
	//stored here with the name of the switch as the index
	//and the value of the switch as the data
	std::map<string,string> cmdLineSwitches;
	void parseCmdLineArg(const char* in, char* var, char* val);
	void ParseCommandLine(int argc, const char* argv[]);
	


	void fatalError(char* fmt,...);
	void warning(char* fmt,...);

	void DoAwards(CHTMLFile& MatchResultsPage);
	void DoMatchResults();
	
	void printUsage();
	void main(int argc, const char* argv[]);

	int majorVer;
	int minorVer;
};

extern CTFStatsApplication* g_pApp;
#endif // TFSTATSAPPLICATION_H
