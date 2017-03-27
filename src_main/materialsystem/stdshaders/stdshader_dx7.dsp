# Microsoft Developer Studio Project File - Name="stdshader_dx7" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=stdshader_dx7 - Win32 ReleaseWithSymbols
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "stdshader_dx7.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "stdshader_dx7.mak" CFG="stdshader_dx7 - Win32 ReleaseWithSymbols"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "stdshader_dx7 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "stdshader_dx7 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "stdshader_dx7"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "stdshader_dx7 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_dx7"
# PROP BASE Intermediate_Dir "Release_dx7"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_dx7"
# PROP Intermediate_Dir "Release_dx7"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX7_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX7_DLL_EXPORT" /D "_WIN32" /Fo"Release_dx7/" /Fd"Release_dx7/" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Release_dx7/stdshader_dx7.bsc"
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /pdb:"Release_dx7/stdshader_dx7.pdb" /machine:I386 /out:"Release_dx7/stdshader_dx7.dll" /implib:"Release_dx7/stdshader_dx7.lib" /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Release_dx7
TargetPath=.\Release_dx7\stdshader_dx7.dll
InputPath=.\Release_dx7\stdshader_dx7.dll
SOURCE="$(InputPath)"

"..\..\..\bin\stdshader_dx7.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\stdshader_dx7.dll attrib -r ..\..\..\bin\stdshader_dx7.dll 
	copy $(TargetPath) ..\..\..\bin\stdshader_dx7.dll 
	if exist $(TargetDir)\stdshader_dx7.map copy $(TargetDir)\stdshader_dx7.map ..\..\..\bin\stdshader_dx7.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "stdshader_dx7 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_dx7"
# PROP BASE Intermediate_Dir "Debug_dx7"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_dx7"
# PROP Intermediate_Dir "Debug_dx7"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX7_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX7_DLL_EXPORT" /D "_WIN32" /FR"Debug_dx7/" /Fo"Debug_dx7/" /Fd"Debug_dx7/" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug_dx7/stdshader_dx7.bsc"
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /pdb:"Debug_dx7/stdshader_dx7.pdb" /debug /machine:I386 /out:"Debug_dx7/stdshader_dx7.dll" /implib:"Debug_dx7/stdshader_dx7.lib" /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Debug_dx7
TargetPath=.\Debug_dx7\stdshader_dx7.dll
InputPath=.\Debug_dx7\stdshader_dx7.dll
SOURCE="$(InputPath)"

"..\..\..\bin\stdshader_dx7.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\stdshader_dx7.dll attrib -r ..\..\..\bin\stdshader_dx7.dll 
	copy $(TargetPath) ..\..\..\bin\stdshader_dx7.dll 
	if exist $(TargetDir)\stdshader_dx7.map copy $(TargetDir)\stdshader_dx7.map ..\..\..\bin\stdshader_dx7.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "stdshader_dx7 - Win32 Release"
# Name "stdshader_dx7 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=.\shatteredglass_dx7.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_dx7.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\shaderlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
