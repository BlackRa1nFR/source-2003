# Microsoft Developer Studio Project File - Name="tfstats" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=tfstats - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tfstats.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tfstats.mak" CFG="tfstats - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tfstats - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "tfstats - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tfstats - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O1 /I "regexp\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"Release/tfstatsRT.exe" /libpath:"regexp\lib"
# Begin Custom Build - copying  $(InputPath) to $(ProjDir)
ProjDir=.
TargetName=tfstatsRT
InputPath=.\Release\tfstatsRT.exe
SOURCE="$(InputPath)"

"$(ProjDir)\$(TargetName)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(InputPath) $(ProjDir)

# End Custom Build

!ELSEIF  "$(CFG)" == "tfstats - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "regexp\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/tfstatsRT.exe" /pdbtype:sept /libpath:"regexp\lib"

!ENDIF 

# Begin Target

# Name "tfstats - Win32 Release"
# Name "tfstats - Win32 Debug"
# Begin Group "Parsing"

# PROP Default_Filter ""
# Begin Group "Parsing Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Argument.cpp
# End Source File
# Begin Source File

SOURCE=.\EventList.cpp
# End Source File
# Begin Source File

SOURCE=.\LogEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\LogEventIOStreams.cpp
# End Source File
# End Group
# Begin Group "Parsing Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Argument.h
# End Source File
# Begin Source File

SOURCE=.\EventList.h
# End Source File
# Begin Source File

SOURCE=.\LogEvent.h
# End Source File
# End Group
# Begin Group "Reports"

# PROP Default_Filter ""
# Begin Group "Awards"

# PROP Default_Filter ""
# Begin Group "Award Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CureAward.cpp
# End Source File
# Begin Source File

SOURCE=.\KamikazeAward.cpp
# End Source File
# Begin Source File

SOURCE=.\SentryRebuildAward.cpp
# End Source File
# Begin Source File

SOURCE=.\SharpshooterAward.cpp
# End Source File
# Begin Source File

SOURCE=.\SurvivalistAward.cpp
# End Source File
# Begin Source File

SOURCE=.\TalkativeAward.cpp
# End Source File
# Begin Source File

SOURCE=.\TeamKillAward.cpp
# End Source File
# Begin Source File

SOURCE=.\WeaponAwards.cpp
# End Source File
# End Group
# Begin Group "Award Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CureAward.h
# End Source File
# Begin Source File

SOURCE=.\KamikazeAward.h
# End Source File
# Begin Source File

SOURCE=.\SentryRebuildAward.h
# End Source File
# Begin Source File

SOURCE=.\SharpshooterAward.h
# End Source File
# Begin Source File

SOURCE=.\SurvivalistAward.h
# End Source File
# Begin Source File

SOURCE=.\TalkativeAward.h
# End Source File
# Begin Source File

SOURCE=.\TeamKillAward.h
# End Source File
# Begin Source File

SOURCE=.\WeaponAwards.h
# End Source File
# End Group
# Begin Group "CustomAwards"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CustomAward.cpp
# End Source File
# Begin Source File

SOURCE=.\CustomAward.h
# End Source File
# Begin Source File

SOURCE=.\CustomAwardList.cpp
# End Source File
# Begin Source File

SOURCE=.\CustomAwardList.h
# End Source File
# Begin Source File

SOURCE=.\CustomAwardTriggers.cpp
# End Source File
# Begin Source File

SOURCE=.\CustomAwardTriggers.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Award.cpp
# End Source File
# Begin Source File

SOURCE=.\Award.h
# End Source File
# Begin Source File

SOURCE=.\awards.h
# End Source File
# End Group
# Begin Group "Report Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AllPlayersStats.cpp
# End Source File
# Begin Source File

SOURCE=.\CVars.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogueReadout.cpp
# End Source File
# Begin Source File

SOURCE=.\MatchResults.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerReport.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerSpecifics.cpp
# End Source File
# Begin Source File

SOURCE=.\scoreboard.cpp
# End Source File
# Begin Source File

SOURCE=.\WhoKilledWho.cpp
# End Source File
# End Group
# Begin Group "Report Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AllPlayersStats.h
# End Source File
# Begin Source File

SOURCE=.\Cvars.h
# End Source File
# Begin Source File

SOURCE=.\DialogueReadout.h
# End Source File
# Begin Source File

SOURCE=.\MatchResults.h
# End Source File
# Begin Source File

SOURCE=.\PlayerReport.h
# End Source File
# Begin Source File

SOURCE=.\PlayerSpecifics.h
# End Source File
# Begin Source File

SOURCE=.\scoreboard.h
# End Source File
# Begin Source File

SOURCE=.\WhoKilledWho.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Report.cpp
# End Source File
# Begin Source File

SOURCE=.\report.h
# End Source File
# End Group
# End Group
# Begin Group "Logfiles"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\testsuite\testlog1.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog10.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog11.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog12.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog13.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog14.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog15.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog16.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog2.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog3.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog4.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog5.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog50.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog51.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog52.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog53.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog54.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog55.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog56.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog57.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog58.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog59.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog6.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog60.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog7.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog8.txt
# End Source File
# Begin Source File

SOURCE=.\testsuite\testlog9.txt
# End Source File
# End Group
# Begin Group "Util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BinaryResource.h
# End Source File
# Begin Source File

SOURCE=.\binResources.cpp
# End Source File
# Begin Source File

SOURCE=.\HTML.cpp
# End Source File
# Begin Source File

SOURCE=.\HTML.h
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\memdbg.cpp
# End Source File
# Begin Source File

SOURCE=.\memdbg.h
# End Source File
# Begin Source File

SOURCE=.\pid.cpp
# End Source File
# Begin Source File

SOURCE=.\pid.h
# End Source File
# Begin Source File

SOURCE=.\StaticOutputFiles.cpp
# End Source File
# Begin Source File

SOURCE=.\TextFile.cpp
# End Source File
# Begin Source File

SOURCE=.\TextFile.h
# End Source File
# Begin Source File

SOURCE=.\TimeIndexedList.h
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# Begin Group "rule files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tfc.2fort.rul
# End Source File
# Begin Source File

SOURCE=.\tfc.cz2.rul
# End Source File
# Begin Source File

SOURCE=.\tfc.rock2.rul
# End Source File
# Begin Source File

SOURCE=.\tfc.rul
# End Source File
# Begin Source File

SOURCE=.\tfc.well.rul
# End Source File
# End Group
# Begin Group "Documentation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# Begin Source File

SOURCE=.\manual\regexp.html
# End Source File
# Begin Source File

SOURCE=.\manual\TFstats.htm
# End Source File
# Begin Source File

SOURCE=.\tfstats.txt
# End Source File
# End Group
# Begin Source File

SOURCE=.\MatchInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\MatchInfo.h
# End Source File
# Begin Source File

SOURCE=.\Player.cpp
# End Source File
# Begin Source File

SOURCE=.\Player.h
# End Source File
# Begin Source File

SOURCE=.\PlrPersist.cpp
# End Source File
# Begin Source File

SOURCE=.\PlrPersist.h
# End Source File
# Begin Source File

SOURCE=.\TFStatsApplication.cpp
# End Source File
# Begin Source File

SOURCE=.\TFStatsApplication.h
# End Source File
# Begin Source File

SOURCE=.\TFStatsOSInterface.cpp
# End Source File
# Begin Source File

SOURCE=.\TFStatsOSInterface.h
# End Source File
# Begin Source File

SOURCE=.\TFStatsReport.cpp
# End Source File
# Begin Source File

SOURCE=.\TFStatsReport.h
# End Source File
# End Target
# End Project
