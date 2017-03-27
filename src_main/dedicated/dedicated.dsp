# Microsoft Developer Studio Project File - Name="Dedicated" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Dedicated - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dedicated.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dedicated.mak" CFG="Dedicated - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Dedicated - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Dedicated - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/dedicated", DJEBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Dedicated - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /MT /W4 /Zi /O2 /I "..\engine" /I "..\public" /I "..\common" /D "_WIN32" /D "NDEBUG" /D "_WINDOWS" /D "DEDICATED" /D "LAUNCHERONLY" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /map /debug /machine:I386 /libpath:"..\lib\common" /libpath:"..\lib\public"
# Begin Custom Build - Copying to root & building .DAT file
TargetDir=.\Release
TargetPath=.\Release\dedicated.exe
InputPath=.\Release\dedicated.exe
SOURCE="$(InputPath)"

"..\..\bin\hlds.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\bin\hlds.exe attrib -r ..\..\bin\hlds.exe 
	if exist ..\..\bin\hlds.dat attrib -r ..\..\bin\hlds.dat 
	copy $(TargetPath) ..\..\bin\hlds.exe 
	if exist $(TargetDir)\hlds.map copy $(TargetDir)\hlds.map ..\..\bin\hlds.map 
	..\..\bin\newdat ..\..\bin\hlds.exe 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "Dedicated - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /MTd /W4 /Gm /ZI /Od /I "..\engine" /I "..\public" /I "..\common" /D "_WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DEDICATED" /D "LAUNCHERONLY" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\lib\common" /libpath:"..\lib\public"
# SUBTRACT LINK32 /map
# Begin Custom Build
TargetDir=.\Debug
TargetPath=.\Debug\dedicated.exe
InputPath=.\Debug\dedicated.exe
SOURCE="$(InputPath)"

"..\..\bin\hlds.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\bin\hlds.exe attrib -r ..\..\bin\hlds.exe 
	if exist ..\..\bin\hlds.dat attrib -r ..\..\bin\hlds.dat 
	copy $(TargetPath) ..\..\bin\hlds.exe 
	if exist $(TargetDir)\hlds.map copy $(TargetDir)\hlds.map ..\..\bin\hlds.map 
	..\..\bin\newdat ..\..\bin\hlds.exe 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "Dedicated - Win32 Release"
# Name "Dedicated - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\Public\checksum_md5.cpp
# End Source File
# Begin Source File

SOURCE=.\conproc.cpp
# End Source File
# Begin Source File

SOURCE=.\dedicated.rc
# End Source File
# Begin Source File

SOURCE=.\filesystem.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\launcher_int.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\Mathlib.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_common.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_linux.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\sys_windows.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=.\callbacks.h
# End Source File
# Begin Source File

SOURCE=..\Public\checksum_md5.h
# End Source File
# Begin Source File

SOURCE=..\Public\commonmacros.h
# End Source File
# Begin Source File

SOURCE=.\conproc.h
# End Source File
# Begin Source File

SOURCE=..\Public\convar.h
# End Source File
# Begin Source File

SOURCE=.\dedicated.h
# End Source File
# Begin Source File

SOURCE=..\Public\dll_state.h
# End Source File
# Begin Source File

SOURCE=..\Public\engine_launcher_api.h
# End Source File
# Begin Source File

SOURCE=..\Public\exefuncs.h
# End Source File
# Begin Source File

SOURCE=.\exports.h
# End Source File
# Begin Source File

SOURCE=..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\Public\icvar.h
# End Source File
# Begin Source File

SOURCE=.\ifilesystem.h
# End Source File
# Begin Source File

SOURCE=..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=.\isys.h
# End Source File
# Begin Source File

SOURCE=..\Public\launcher_int.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\public\platform\vcrmode.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\NetGame.ico
# End Source File
# End Group
# Begin Source File

SOURCE=..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
