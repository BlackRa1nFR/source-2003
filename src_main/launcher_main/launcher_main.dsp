# Microsoft Developer Studio Project File - Name="launcher_main" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=launcher_main - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "launcher_main.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "launcher_main.mak" CFG="launcher_main - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "launcher_main - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "launcher_main - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/launcher_main", COYDAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "launcher_main - Win32 Release"

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
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\common" /I "..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D fopen=dont_use_fopen /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\common" /I "..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D fopen=dont_use_fopen /D "_WIN32" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386 /libpath:"..\lib\common\\" /libpath:"..\lib\public\\"
# ADD LINK32 user32.lib /nologo /subsystem:windows /map /debug /machine:I386 /libpath:"..\lib\common\\" /libpath:"..\lib\public\\" /fixed:no
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\bin\)...
TargetDir=.\Release
TargetPath=.\Release\launcher_main.exe
InputPath=.\Release\launcher_main.exe
SOURCE="$(InputPath)"

BuildCmds= \
	if exist ..\..\hl2.exe attrib -r ..\..\hl2.exe \
	copy $(TargetPath) ..\..\hl2.exe \
	if exist ..\..\tf2.exe attrib -r ..\..\tf2.exe \
	copy $(TargetPath) ..\..\tf2.exe \
	if exist $(TargetDir)\launcher_main.map copy $(TargetDir)\launcher_main.map ..\..\tf2.map \
	if exist $(TargetDir)\launcher_main.map copy $(TargetDir)\launcher_main.map ..\..\hl2.map \
	attrib -r ..\..\tf2.dat \
	attrib -r ..\..\hl2.dat \
	..\..\bin\newdat ..\..\tf2.exe \
	..\..\bin\newdat ..\..\hl2.exe \
	

"..\..\hl2.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\..\tf2.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "launcher_main - Win32 Debug"

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
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\common" /I "..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D fopen=dont_use_fopen /D "_WIN32" /FR /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\common" /I "..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D fopen=dont_use_fopen /D "_WIN32" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\lib\common\\" /libpath:"..\lib\public\\"
# ADD LINK32 user32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\lib\common\\" /libpath:"..\lib\public\\"
# Begin Custom Build - Publishing to target directory (..\..\bin\)...
TargetDir=.\Debug
TargetPath=.\Debug\launcher_main.exe
InputPath=.\Debug\launcher_main.exe
SOURCE="$(InputPath)"

BuildCmds= \
	if exist ..\..\hl2.exe attrib -r ..\..\hl2.exe \
	copy $(TargetPath) ..\..\hl2.exe \
	if exist ..\..\tf2.exe attrib -r ..\..\tf2.exe \
	copy $(TargetPath) ..\..\tf2.exe \
	if exist $(TargetDir)\launcher_main.map copy $(TargetDir)\launcher_main.map ..\..\tf2.map \
	if exist $(TargetDir)\launcher_main.map copy $(TargetDir)\launcher_main.map ..\..\hl2.map \
	attrib -r ..\..\tf2.dat \
	attrib -r ..\..\hl2.dat \
	..\..\bin\newdat ..\..\tf2.exe \
	..\..\bin\newdat ..\..\hl2.exe \
	

"..\..\hl2.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\..\tf2.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# Begin Target

# Name "launcher_main - Win32 Release"
# Name "launcher_main - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\launcher_main.rc
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\launcher\res\launcher.ico
# End Source File
# End Target
# End Project
