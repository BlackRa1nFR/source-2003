# Microsoft Developer Studio Project File - Name="vgui_surfacelib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vgui_surfacelib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vgui_surfacelib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vgui_surfacelib.mak" CFG="vgui_surfacelib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vgui_surfacelib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "vgui_surfacelib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "vgui_surfacelib"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vgui_surfacelib - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W4 /GX /Z7 /O2 /I "..\..\public" /I "..\..\common\\" /I "..\include" /D "NDEBUG" /D "_WIN32" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Custom Build
TargetDir=.\Release
TargetPath=.\Release\vgui_surfacelib.lib
InputPath=.\Release\vgui_surfacelib.lib
SOURCE="$(InputPath)"

"..\..\lib\public\vgui_surfacelib.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\lib\public\vgui_surfacelib.lib attrib -r ..\..\lib\public\vgui_surfacelib.lib 
	copy $(TargetPath) ..\..\lib\public\vgui_surfacelib.lib 
	if exist $(TargetDir)\vgui_surfacelib.map copy $(TargetDir)\vgui_surfacelib.map ..\..\lib\public\vgui_surfacelib.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vgui_surfacelib - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W4 /Gm /GR /GX /ZI /Od /I "..\..\public" /I "..\..\common\\" /I "..\include" /D "_DEBUG" /D "_WIN32" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Custom Build
TargetDir=.\Debug
TargetPath=.\Debug\vgui_surfacelib.lib
InputPath=.\Debug\vgui_surfacelib.lib
SOURCE="$(InputPath)"

"..\..\lib\public\vgui_surfacelib.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\lib\public\vgui_surfacelib.lib attrib -r ..\..\lib\public\vgui_surfacelib.lib 
	copy $(TargetPath) ..\..\lib\public\vgui_surfacelib.lib 
	if exist $(TargetDir)\vgui_surfacelib.map copy $(TargetDir)\vgui_surfacelib.map ..\..\lib\public\vgui_surfacelib.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "vgui_surfacelib - Win32 Release"
# Name "vgui_surfacelib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\FontAmalgam.cpp
# End Source File
# Begin Source File

SOURCE=.\FontManager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\utlbuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32Font.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\..\public\dbg\dbg.h
# End Source File
# Begin Source File

SOURCE=..\..\public\platform\fasttimer.h
# End Source File
# Begin Source File

SOURCE=..\..\common\vgui_surfacelib\FontAmalgam.h
# End Source File
# Begin Source File

SOURCE=..\..\common\vgui_surfacelib\FontManager.h
# End Source File
# Begin Source File

SOURCE=..\..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=..\..\public\platform\platform.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=..\..\public\utlbuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlMemory.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlVector.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vector2d.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\vstdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\common\vgui_surfacelib\Win32Font.h
# End Source File
# End Group
# End Target
# End Project
