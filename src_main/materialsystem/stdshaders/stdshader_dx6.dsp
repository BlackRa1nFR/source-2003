# Microsoft Developer Studio Project File - Name="stdshader_dx6" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=stdshader_dx6 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "stdshader_dx6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "stdshader_dx6.mak" CFG="stdshader_dx6 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "stdshader_dx6 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "stdshader_dx6 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "stdshader_dx6"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "stdshader_dx6 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_dx6"
# PROP BASE Intermediate_Dir "Release_dx6"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_dx6"
# PROP Intermediate_Dir "Release_dx6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX6_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX6_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Release_dx6
TargetPath=.\Release_dx6\stdshader_dx6.dll
InputPath=.\Release_dx6\stdshader_dx6.dll
SOURCE="$(InputPath)"

"..\..\..\bin\stdshader_dx6.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\stdshader_dx6.dll attrib -r ..\..\..\bin\stdshader_dx6.dll 
	copy $(TargetPath) ..\..\..\bin\stdshader_dx6.dll 
	if exist $(TargetDir)\stdshader_dx6.map copy $(TargetDir)\stdshader_dx6.map ..\..\..\bin\stdshader_dx6.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "stdshader_dx6 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_dx6"
# PROP BASE Intermediate_Dir "Debug_dx6"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_dx6"
# PROP Intermediate_Dir "Debug_dx6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX6_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX6_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Debug_dx6
TargetPath=.\Debug_dx6\stdshader_dx6.dll
InputPath=.\Debug_dx6\stdshader_dx6.dll
SOURCE="$(InputPath)"

"..\..\..\bin\stdshader_dx6.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\stdshader_dx6.dll attrib -r ..\..\..\bin\stdshader_dx6.dll 
	copy $(TargetPath) ..\..\..\bin\stdshader_dx6.dll 
	if exist $(TargetDir)\stdshader_dx6.map copy $(TargetDir)\stdshader_dx6.map ..\..\..\bin\stdshader_dx6.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "stdshader_dx6 - Win32 Release"
# Name "stdshader_dx6 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\basetimesmod2xenvmap.cpp
# End Source File
# Begin Source File

SOURCE=.\cloud.cpp
# End Source File
# Begin Source File

SOURCE=.\decalbasetimeslightmapalphablendselfillum.cpp
# End Source File
# Begin Source File

SOURCE=.\decalmodulate.cpp
# End Source File
# Begin Source File

SOURCE=.\eyeball.cpp
# End Source File
# Begin Source File

SOURCE=.\fogsurface.cpp
# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\lightmappedtwotexture.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=.\modulate_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\shadow_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\shadowbuild_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\sky_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\skyfog.cpp
# End Source File
# Begin Source File

SOURCE=.\sprite_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\unlittwotexture_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\viewalpha.cpp
# End Source File
# Begin Source File

SOURCE=.\volumetricfog.cpp
# End Source File
# Begin Source File

SOURCE=.\water_dx60.cpp
# End Source File
# Begin Source File

SOURCE=.\worldtwotextureblend.cpp
# End Source File
# Begin Source File

SOURCE=.\worldvertextransition_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\writez_dx6.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\shaderlib.lib
# End Source File
# End Target
# End Project
