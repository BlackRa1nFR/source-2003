# Microsoft Developer Studio Project File - Name="NVTriStripLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=NVTriStripLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "nvtristriplib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nvtristriplib.mak" CFG="NVTriStripLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NVTriStripLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "NVTriStripLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/utils/NvTriStripLib", DFCDAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NVTriStripLib - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Z7 /O2 /I "..\..\lib\common" /I "..\..\lib\public" /D "_WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\NvTriStrip.lib"
# Begin Custom Build
TargetDir=.\Release
InputPath=.\Release\NvTriStrip.lib
SOURCE="$(InputPath)"

"..\..\lib\public\nvtristrip.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\lib\public\nvtristrip.lib attrib -r ..\..\lib\public\nvtristrip.lib 
	if exist $(TargetDir)\nvtristrip.lib copy $(TargetDir)\nvtristrip.lib ..\..\lib\public\nvtristrip.lib 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=attrib -r release/nvtristrip.lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "NVTriStripLib - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\..\lib\common" /I "..\..\lib\public" /D "_WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\NvTriStrip.lib"
# Begin Custom Build
TargetDir=.\Debug
InputPath=.\Debug\NvTriStrip.lib
SOURCE="$(InputPath)"

"..\..\lib\public\nvtristrip.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\lib\public\nvtristrip.lib attrib -r ..\..\lib\public\nvtristrip.lib 
	if exist $(TargetDir)\nvtristrip.lib copy $(TargetDir)\nvtristrip.lib ..\..\lib\public\nvtristrip.lib 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=attrib -r debug/nvtristrip.lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "NVTriStripLib - Win32 Release"
# Name "NVTriStripLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\NvTriStrip.cpp
# End Source File
# Begin Source File

SOURCE=.\NvTriStripObjects.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\NvTriStrip.h
# End Source File
# Begin Source File

SOURCE=.\NvTriStripObjects.h
# End Source File
# Begin Source File

SOURCE=.\VertexCache.h
# End Source File
# End Group
# End Target
# End Project
