# Microsoft Developer Studio Project File - Name="TrackerSRV" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TrackerSRV - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TrackerSRV.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TrackerSRV.mak" CFG="TrackerSRV - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TrackerSRV - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TrackerSRV - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GoldSrc/Tracker/TrackerSRV", ASACAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERSRV_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /WX /GR /GX /O2 /I "..\..\public" /I "..\public" /I "..\..\common" /I "..\common" /I "..\..\utils\mysql\include" /I "..\..\utils\mysql\mysql\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERSRV_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib /nologo /dll /machine:I386 /nodefaultlib:"libc.lib"
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Custom Build
TargetPath=.\Release\TrackerSRV.dll
InputPath=.\Release\TrackerSRV.dll
SOURCE="$(InputPath)"

"..\..\..\TrackerServer\TrackerSRV.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\TrackerServer\TrackerSRV.dll attrib -r ..\..\..\TrackerServer\TrackerSRV.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\TrackerServer\TrackerSRV.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERSRV_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /WX /Gm /GR /GX /ZI /Od /I "..\..\public" /I "..\public" /I "..\..\common" /I "..\common" /I "..\..\utils\mysql\include" /I "..\..\utils\mysql\mysql\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERSRV_EXPORTS" /FR /Yu"PrecompHeader.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib /nologo /dll /map /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept
# Begin Custom Build
TargetPath=.\Debug\TrackerSRV.dll
InputPath=.\Debug\TrackerSRV.dll
SOURCE="$(InputPath)"

"..\..\..\TrackerServer\TrackerSRV.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\TrackerServer\TrackerSRV.dll attrib -r ..\..\..\TrackerServer\TrackerSRV.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\TrackerServer\TrackerSRV.dll 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "TrackerSRV - Win32 Release"
# Name "TrackerSRV - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\common\CompletionEvent.cpp

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DebugTimer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LocalDebugConsole.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\common\MemPool.cpp

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\TrackerNET\NetAddress.cpp

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PrecompHeader.cpp

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

# ADD CPP /Yc"PrecompHeader.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\Random.cpp

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SessionManager.cpp

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TopologyManager.cpp

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TrackerDatabaseManager.cpp
# End Source File
# Begin Source File

SOURCE=.\TrackerSRV.cpp

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\UserSession.cpp

!IF  "$(CFG)" == "TrackerSRV - Win32 Release"

!ELSEIF  "$(CFG)" == "TrackerSRV - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\common\CompletionEvent.h
# End Source File
# Begin Source File

SOURCE=..\common\DebugConsole_Interface.h
# End Source File
# Begin Source File

SOURCE=.\DebugTimer.h
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.h
# End Source File
# Begin Source File

SOURCE=..\Public\ITrackerDistroDatabase.h
# End Source File
# Begin Source File

SOURCE=..\Public\ITrackerUserDatabase.h
# End Source File
# Begin Source File

SOURCE=.\LocalDebugConsole.h
# End Source File
# Begin Source File

SOURCE=.\PrecompHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\common\Random.h
# End Source File
# Begin Source File

SOURCE=.\SessionManager.h
# End Source File
# Begin Source File

SOURCE=.\TopologyManager.h
# End Source File
# Begin Source File

SOURCE=..\Public\TrackerCmdID.h
# End Source File
# Begin Source File

SOURCE=.\TrackerDatabaseManager.h
# End Source File
# Begin Source File

SOURCE=..\common\TrackerProtocol.h
# End Source File
# Begin Source File

SOURCE=.\TrackerSRV_Interface.h
# End Source File
# Begin Source File

SOURCE=.\UserSession.h
# End Source File
# Begin Source File

SOURCE=..\common\UtlMsgBuffer.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
