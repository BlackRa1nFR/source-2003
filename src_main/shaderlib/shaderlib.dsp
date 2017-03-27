# Microsoft Developer Studio Project File - Name="shaderlib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=shaderlib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "shaderlib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "shaderlib.mak" CFG="shaderlib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "shaderlib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "shaderlib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "shaderlib"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "shaderlib - Win32 Release"

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
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\common" /I "..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D fopen=dont_use_fopen /D "_WIN32" /FD /c
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
TargetPath=.\Release\shaderlib.lib
InputPath=.\Release\shaderlib.lib
SOURCE="$(InputPath)"

"..\lib\public\shaderlib.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\lib\public\shaderlib.lib attrib -r ..\lib\public\shaderlib.lib 
	copy $(TargetPath) ..\lib\public\shaderlib.lib 
	if exist $(TargetDir)\shaderlib.map copy $(TargetDir)\shaderlib.map ..\lib\public\shaderlib.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "shaderlib - Win32 Debug"

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
# ADD BASE CPP /nologo /G6 /W4 /Gm /Zi /Od /Op /I "..\common" /I "..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D fopen=dont_use_fopen /D "_WIN32" /FR /FD /GZ /c
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
TargetPath=.\Debug\shaderlib.lib
InputPath=.\Debug\shaderlib.lib
SOURCE="$(InputPath)"

"..\lib\public\shaderlib.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\lib\public\shaderlib.lib attrib -r ..\lib\public\shaderlib.lib 
	copy $(TargetPath) ..\lib\public\shaderlib.lib 
	if exist $(TargetDir)\shaderlib.map copy $(TargetDir)\shaderlib.map ..\lib\public\shaderlib.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "shaderlib - Win32 Release"
# Name "shaderlib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BaseShader.cpp
# End Source File
# Begin Source File

SOURCE=..\public\convar.cpp
# End Source File
# Begin Source File

SOURCE=..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\ShaderDLL.cpp
# End Source File
# Begin Source File

SOURCE=.\shaderDLL_Global.h
# End Source File
# Begin Source File

SOURCE=.\shaderlib_cvar.cpp
# End Source File
# Begin Source File

SOURCE=.\shaderlib_cvar.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\public\shaderlib\BaseShader.h
# End Source File
# Begin Source File

SOURCE=..\public\shaderlib\cshader.h
# End Source File
# Begin Source File

SOURCE=..\public\shaderlib\ShaderDLL.h
# End Source File
# End Group
# End Target
# End Project
