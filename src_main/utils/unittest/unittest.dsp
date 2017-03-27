# Microsoft Developer Studio Project File - Name="unittest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=unittest - Win32 ReleaseWithSymbols
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "unittest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "unittest.mak" CFG="unittest - Win32 ReleaseWithSymbols"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "unittest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "unittest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "unittest - Win32 ReleaseWithSymbols" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/utils/unittest", QOYDAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "unittest - Win32 Release"

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
# ADD BASE CPP /nologo /G6 /MT /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# ADD CPP /nologo /G6 /MT /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WIN32" /D "_CONSOLE" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /machine:I386 /libpath:"..\..\..\lib\common\Release" /libpath:"..\..\..\lib\public\Release"
# ADD LINK32 unitlib.lib /nologo /subsystem:console /machine:I386 /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# Begin Custom Build - Publishing to target directory...
TargetDir=.\Release
TargetPath=.\Release\unittest.exe
InputPath=.\Release\unittest.exe
SOURCE="$(InputPath)"

"..\..\..\bin\unittest.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\unittest.exe attrib -r ..\..\..\bin\unittest.exe 
	copy $(TargetPath) ..\..\..\bin\unittest.exe 
	if exist $(TargetDir)\unittest.map copy $(TargetDir)\unittest.map ..\..\..\bin\unittest.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "unittest - Win32 Debug"

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
# ADD BASE CPP /nologo /G6 /MTd /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /GZ /GZ /c
# ADD CPP /nologo /G6 /MTd /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WIN32" /D "_CONSOLE" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /FR /FD /GZ /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\lib\common\Debug" /libpath:"..\..\..\lib\public\Debug"
# ADD LINK32 unitlib.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# Begin Custom Build - Publishing to target directory...
TargetDir=.\Debug
TargetPath=.\Debug\unittest.exe
InputPath=.\Debug\unittest.exe
SOURCE="$(InputPath)"

"..\..\..\bin\unittest.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\unittest.exe attrib -r ..\..\..\bin\unittest.exe 
	copy $(TargetPath) ..\..\..\bin\unittest.exe 
	if exist $(TargetDir)\unittest.map copy $(TargetDir)\unittest.map ..\..\..\bin\unittest.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "unittest - Win32 ReleaseWithSymbols"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "unittest___Win32_ReleaseWithSymbols"
# PROP BASE Intermediate_Dir "unittest___Win32_ReleaseWithSymbols"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "unittest___Win32_ReleaseWithSymbols"
# PROP Intermediate_Dir "unittest___Win32_ReleaseWithSymbols"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MT /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "NDEBUG" /FR /FD /c
# ADD CPP /nologo /G6 /MT /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WIN32" /D "_CONSOLE" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /FR /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\lib\common\ReleaseWithSymbols" /libpath:"..\..\..\lib\public\ReleaseWithSymbols"
# ADD LINK32 unitlib.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# Begin Custom Build - Publishing to target directory...
TargetDir=.\unittest___Win32_ReleaseWithSymbols
TargetPath=.\unittest___Win32_ReleaseWithSymbols\unittest.exe
InputPath=.\unittest___Win32_ReleaseWithSymbols\unittest.exe
SOURCE="$(InputPath)"

"..\..\..\bin\unittest.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\unittest.exe attrib -r ..\..\..\bin\unittest.exe 
	copy $(TargetPath) ..\..\..\bin\unittest.exe 
	if exist $(TargetDir)\unittest.map copy $(TargetDir)\unittest.map ..\..\..\bin\unittest.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "unittest - Win32 Release"
# Name "unittest - Win32 Debug"
# Name "unittest - Win32 ReleaseWithSymbols"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\unittest.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
