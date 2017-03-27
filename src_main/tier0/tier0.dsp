# Microsoft Developer Studio Project File - Name="tier0" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TIER0 - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tier0.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tier0.mak" CFG="TIER0 - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tier0 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tier0 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "tier0"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tier0 - Win32 Release"

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
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\common" /I "..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "TIER0_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /MT /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\common" /I "..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TIER0_DLL_EXPORT" /D "_WIN32" /FR /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\lib\common\\" /libpath:"..\lib\public\\"
# ADD LINK32 ws2_32.lib advapi32.lib /nologo /subsystem:windows /dll /incremental:yes /debug /machine:I386 /pdbtype:sept /libpath:"..\lib\common" /libpath:"..\lib\public"
# Begin Custom Build - Publishing to target directory (..\..\bin\)...
TargetDir=.\Release
TargetPath=.\Release\tier0.dll
InputPath=.\Release\tier0.dll
SOURCE="$(InputPath)"

BuildCmds= \
	if exist ..\..\bin\tier0.dll attrib -r ..\..\bin\tier0.dll \
	copy $(TargetPath) ..\..\bin\tier0.dll \
	if exist $(TargetDir)\tier0.map copy $(TargetDir)\tier0.map ..\..\bin\tier0.map \
	if exist ..\lib\public\tier0.lib attrib -r ..\lib\public\tier0.lib \
	if exist $(TargetDir)\tier0.lib copy $(TargetDir)\tier0.lib ..\lib\public\tier0.lib \
	

"..\..\bin\tier0.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\lib\public\tier0.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "tier0 - Win32 Debug"

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
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\common" /I "..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "TIER0_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD CPP /nologo /G6 /MTd /W4 /Gm /ZI /Od /Op /I "..\common" /I "..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TIER0_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\lib\common\\" /libpath:"..\lib\public\\"
# ADD LINK32 ws2_32.lib advapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\lib\common" /libpath:"..\lib\public"
# Begin Custom Build - Publishing to target directory (..\..\bin\)...
TargetDir=.\Debug
TargetPath=.\Debug\tier0.dll
InputPath=.\Debug\tier0.dll
SOURCE="$(InputPath)"

BuildCmds= \
	if exist ..\..\bin\tier0.dll attrib -r ..\..\bin\tier0.dll \
	copy $(TargetPath) ..\..\bin\tier0.dll \
	if exist $(TargetDir)\tier0.map copy $(TargetDir)\tier0.map ..\..\bin\tier0.map \
	if exist ..\lib\public\tier0.lib attrib -r ..\lib\public\tier0.lib \
	if exist $(TargetDir)\tier0.lib copy $(TargetDir)\tier0.lib ..\lib\public\tier0.lib \
	

"..\..\bin\tier0.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\lib\public\tier0.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# Begin Target

# Name "tier0 - Win32 Release"
# Name "tier0 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cpu.cpp
# End Source File
# Begin Source File

SOURCE=.\dbg.cpp
# End Source File
# Begin Source File

SOURCE=.\fasttimer.cpp
# End Source File
# Begin Source File

SOURCE=.\mem.cpp
# End Source File
# Begin Source File

SOURCE=.\memdbg.cpp
# End Source File
# Begin Source File

SOURCE=.\memstd.cpp
# End Source File
# Begin Source File

SOURCE=.\memvalidate.cpp
# End Source File
# Begin Source File

SOURCE=.\platform.cpp
# End Source File
# Begin Source File

SOURCE=.\security.cpp
# End Source File
# Begin Source File

SOURCE=.\thread.cpp
# End Source File
# Begin Source File

SOURCE=.\vcrmode.cpp
# End Source File
# Begin Source File

SOURCE=.\vprof.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Public\tier0\dbg.h
# End Source File
# Begin Source File

SOURCE=..\Public\tier0\fasttimer.h
# End Source File
# Begin Source File

SOURCE=..\Public\tier0\mem.h
# End Source File
# Begin Source File

SOURCE=..\public\tier0\memalloc.h
# End Source File
# Begin Source File

SOURCE=..\Public\tier0\memdbgoff.h
# End Source File
# Begin Source File

SOURCE=..\Public\tier0\memdbgon.h
# End Source File
# Begin Source File

SOURCE=..\Public\tier0\platform.h
# End Source File
# Begin Source File

SOURCE=..\Public\tier0\vcr_shared.h
# End Source File
# Begin Source File

SOURCE=..\Public\tier0\vcrmode.h
# End Source File
# Begin Source File

SOURCE=..\Public\tier0\vprof.h
# End Source File
# End Group
# Begin Group "DESKey"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DESKey\ALGO.H
# End Source File
# Begin Source File

SOURCE=.\DESKey\DK2WIN32.H
# End Source File
# Begin Source File

SOURCE=.\DESKey\DK2WIN32.LIB
# End Source File
# Begin Source File

SOURCE=.\DESKey\ALGO32.LIB
# End Source File
# End Group
# End Target
# End Project
