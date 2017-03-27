# Microsoft Developer Studio Project File - Name="appframework" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=appframework - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "appframework.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "appframework.mak" CFG="appframework - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "appframework - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "appframework - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "appframework"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "appframework - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\src\common" /I "..\..\src\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D fopen=dont_use_fopen /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Z7 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\common" /I "..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D fopen=dont_use_fopen /D "_WIN32" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Custom Build - Publishing to target directory (..\lib\public\)...
TargetDir=.\Release
TargetPath=.\Release\appframework.lib
InputPath=.\Release\appframework.lib
SOURCE="$(InputPath)"

"..\lib\public\appframework.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\lib\public\appframework.lib attrib -r ..\lib\public\appframework.lib 
	copy $(TargetPath) ..\lib\public\appframework.lib 
	if exist $(TargetDir)\appframework.map copy $(TargetDir)\appframework.map ..\lib\public\appframework.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "appframework - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /Zi /Od /Op /I "..\..\src\common" /I "..\..\src\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D fopen=dont_use_fopen /D "_WIN32" /FR /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Z7 /Od /Op /I "..\common" /I "..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D fopen=dont_use_fopen /D "_WIN32" /FR /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Custom Build - Publishing to target directory (..\lib\public\)...
TargetDir=.\Debug
TargetPath=.\Debug\appframework.lib
InputPath=.\Debug\appframework.lib
SOURCE="$(InputPath)"

"..\lib\public\appframework.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\lib\public\appframework.lib attrib -r ..\lib\public\appframework.lib 
	copy $(TargetPath) ..\lib\public\appframework.lib 
	if exist $(TargetDir)\appframework.map copy $(TargetDir)\appframework.map ..\lib\public\appframework.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "appframework - Win32 Release"
# Name "appframework - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AppSystemGroup.cpp
# End Source File
# Begin Source File

SOURCE=..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\public\utlsymbol.cpp
# End Source File
# Begin Source File

SOURCE=.\WinApp.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AppSystemGroup.h
# End Source File
# End Group
# Begin Group "Interface"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\public\appframework\AppFramework.h
# End Source File
# Begin Source File

SOURCE=..\public\appframework\IAppSystem.h
# End Source File
# End Group
# End Target
# End Project
