# Microsoft Developer Studio Project File - Name="hl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=hl - Win32 Debug CounterStrike
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hl.mak" CFG="hl - Win32 Debug CounterStrike"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hl - Win32 Release HL2" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hl - Win32 Debug HL2" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hl - Win32 Debug TF2" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hl - Win32 Release TF2" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hl - Win32 Release HL1" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hl - Win32 Debug HL1" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hl - Win32 Debug CounterStrike" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hl - Win32 Release CounterStrike" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/dlls", ELEBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "hl___Win32_hl2_Release"
# PROP BASE Intermediate_Dir "hl___Win32_hl2_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_hl2"
# PROP Intermediate_Dir "Release_hl2"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MT /W3 /Zi /O2 /I "..\engine" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "QUIVER" /D "VOXEL" /D "QUAKE2" /D "VALVE_DLL" /Fr /YX /FD /c
# ADD CPP /nologo /G6 /W4 /GR /Zi /O2 /I "../game_shared/hl2" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../../dlls" /I "../dlls/hl2_dll" /D "HL2_DLL" /D "USES_SAVERESTORE" /D "NDEBUG" /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /D "PROTECTED_THINGS_ENABLE" /Fr /Yu"cbase.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /def:".\hl.def"
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib:"LIBCD" /out:"Release_hl2/server.dll" /libpath:"..\lib\public"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying to HL2
TargetDir=.\Release_hl2
InputPath=.\Release_hl2\server.dll
SOURCE="$(InputPath)"

"..\..\hl2\bin\server.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\hl2\bin\server.dll attrib -r ..\..\hl2\bin\server.dll 
	if exist $(TargetDir)\server.dll copy $(TargetDir)\server.dll ..\..\hl2\bin\server.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hl___Win32_hl2_Debug"
# PROP BASE Intermediate_Dir "hl___Win32_hl2_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_hl2"
# PROP Intermediate_Dir "Debug_hl2"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MTd /W3 /Gm /ZI /Od /I "..\engine" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "QUIVER" /D "VOXEL" /D "QUAKE2" /D "VALVE_DLL" /FR /YX /FD /c
# ADD CPP /nologo /G6 /W4 /Gm /GR /ZI /Od /I "../game_shared/hl2" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../../dlls" /I "../dlls/hl2_dll" /D "HL2_DLL" /D "USES_SAVERESTORE" /D "_DEBUG" /D fopen=dont_use_fopen /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /D "PROTECTED_THINGS_ENABLE" /FR /Yu"cbase.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\engine" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\engine" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib advapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /def:".\hl.def" /implib:".\Debug\server.lib"
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /debug /machine:I386 /out:"Debug_hl2/server.dll" /implib:".\Debug_hl2\server.lib" /libpath:"..\lib\public"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying to HL2
TargetDir=.\Debug_hl2
InputPath=.\Debug_hl2\server.dll
SOURCE="$(InputPath)"

"..\..\hl2\bin\server.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\hl2\bin\server.dll attrib -r ..\..\hl2\bin\server.dll 
	if exist $(TargetDir)\server.dll copy $(TargetDir)\server.dll ..\..\hl2\bin\server.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hl___Win32_tf2_Debug"
# PROP BASE Intermediate_Dir "hl___Win32_tf2_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_tf2"
# PROP Intermediate_Dir "Debug_tf2"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /W3 /Gm /ZI /Od /I "..\engine" /I "ivp" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "QUIVER" /D "VOXEL" /D "QUAKE2" /D "VALVE_DLL" /FR /YX /FD /c
# ADD CPP /nologo /G6 /W4 /Gm /GR /ZI /Od /I "." /I "../ivp/IVP_Physics" /I "../ivp/IVP_Collision" /I "../ivp/IVP_Controller" /I "../ivp/IVP_Utility" /I "../ivp/IVP_Surface_manager" /I "ivp/" /I "../ivp/IVP_Compact_Builder" /I "../statemachine" /I "../game_shared/tf2" /I "../../game_shared/tf2" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../../dlls" /I "../dlls/tf2_dll" /I "../../dlls/tf2_dll" /D "TF2_DLL" /D "_DEBUG" /D fopen=dont_use_fopen /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /D "PROTECTED_THINGS_ENABLE" /FR /Yu"cbase.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\engine" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\engine" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib advapi32.lib winmm.lib ivp/ivp_lib.lib /nologo /subsystem:windows /dll /debug /machine:I386 /def:".\hl.def" /implib:".\Debug\server.lib"
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /map /debug /machine:I386 /out:"Debug_tf2/server.dll" /implib:".\Debug_hl2\server.lib" /libpath:"..\lib\public"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying to tf2
TargetDir=.\Debug_tf2
InputPath=.\Debug_tf2\server.dll
SOURCE="$(InputPath)"

"..\..\tf2\bin\server.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\tf2\bin\server.dll attrib -r ..\..\tf2\bin\server.dll 
	if exist $(TargetDir)\server.dll copy $(TargetDir)\server.dll ..\..\tf2\bin\server.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "hl___Win32_tf2_Release"
# PROP BASE Intermediate_Dir "hl___Win32_tf2_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_tf2"
# PROP Intermediate_Dir "Release_tf2"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /W3 /Zi /O2 /I "..\engine" /I "ivp" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "QUIVER" /D "VOXEL" /D "QUAKE2" /D "VALVE_DLL" /Fr /YX /FD /c
# ADD CPP /nologo /G6 /W4 /GR /Zi /O2 /I "../ivp/IVP_Physics" /I "../ivp/IVP_Collision" /I "../ivp/IVP_Controller" /I "../ivp/IVP_Utility" /I "../ivp/IVP_Surface_manager" /I "ivp/" /I "../ivp/IVP_Compact_Builder" /I "../statemachine" /I "../game_shared/tf2" /I "../../game_shared/tf2" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../../dlls" /I "../dlls/tf2_dll" /I "../../dlls/tf2_dll" /D "TF2_DLL" /D "NDEBUG" /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /D "PROTECTED_THINGS_ENABLE" /Fr /Yu"cbase.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib ivp/ivp_lib.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /def:".\hl.def"
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib:"LIBCD" /out:"Release_tf2/server.dll" /libpath:"..\lib\public"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying to tf2
TargetDir=.\Release_tf2
InputPath=.\Release_tf2\server.dll
SOURCE="$(InputPath)"

"..\..\tf2\bin\server.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\tf2\bin\server.dll attrib -r ..\..\tf2\bin\server.dll 
	if exist $(TargetDir)\server.dll copy $(TargetDir)\server.dll ..\..\tf2\bin\server.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "hl___Win32_Release_HL1"
# PROP BASE Intermediate_Dir "hl___Win32_Release_HL1"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_hl1"
# PROP Intermediate_Dir "Release_hl1"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /GR /Zi /O2 /I "../game_shared/hl2" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../../dlls" /I "../dlls/hl2_dll" /D "HL1_DLL" /D "USES_SAVERESTORE" /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "NDEBUG" /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /Fr /Yu"cbase.h" /FD /c
# ADD CPP /nologo /G6 /W4 /GR /Zi /O2 /I "../game_shared/hl1" /I "../game_shared/hl2" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../../dlls" /I "../dlls/hl1_dll" /I "../dlls/hl2_dll" /D "HL1_DLL" /D "USES_SAVERESTORE" /D "NDEBUG" /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /D "PROTECTED_THINGS_ENABLE" /Fr /Yu"cbase.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib:"LIBCD" /out:"Release_hl1/server.dll" /libpath:"..\lib\public"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib:"LIBCD" /out:"Release_hl1/server.dll" /libpath:"..\lib\public"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying to HL1
TargetDir=.\Release_hl1
InputPath=.\Release_hl1\server.dll
SOURCE="$(InputPath)"

"..\..\hl1\bin\server.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\hl1\bin\server.dll attrib -r ..\..\hl1\bin\server.dll 
	if exist $(TargetDir)\server.dll copy $(TargetDir)\server.dll ..\..\hl1\bin\server.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hl___Win32_Debug_HL1"
# PROP BASE Intermediate_Dir "hl___Win32_Debug_HL1"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_hl1"
# PROP Intermediate_Dir "Debug_hl1"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /GR /ZI /Od /I "../game_shared/hl2" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../../dlls" /I "../dlls/hl2_dll" /D "HL1_DLL" /D "USES_SAVERESTORE" /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "_DEBUG" /D fopen=dont_use_fopen /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /FR /Yu"cbase.h" /FD /c
# ADD CPP /nologo /G6 /W4 /Gm /GR /ZI /Od /I "../game_shared/hl1" /I "../game_shared/hl2" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../../dlls" /I "../dlls/hl1_dll" /I "../dlls/hl2_dll" /D "HL1_DLL" /D "USES_SAVERESTORE" /D "_DEBUG" /D fopen=dont_use_fopen /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /D "PROTECTED_THINGS_ENABLE" /FR /Yu"cbase.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\engine" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\engine" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /debug /machine:I386 /out:"Debug_hl1/server.dll" /implib:".\Debug_hl1\server.lib" /libpath:"..\lib\public"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /debug /machine:I386 /out:"Debug_hl1/server.dll" /implib:".\Debug_hl1\server.lib" /libpath:"..\lib\public"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying to HL1
TargetDir=.\Debug_hl1
InputPath=.\Debug_hl1\server.dll
SOURCE="$(InputPath)"

"..\..\hl1\bin\server.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\hl1\bin\server.dll attrib -r ..\..\hl1\bin\server.dll 
	if exist $(TargetDir)\server.dll copy $(TargetDir)\server.dll ..\..\hl1\bin\server.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hl___Win32_Debug_CounterStrike"
# PROP BASE Intermediate_Dir "hl___Win32_Debug_CounterStrike"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_CounterStrike"
# PROP Intermediate_Dir "Debug_CounterStrike"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /GR /ZI /Od /I "../game_shared/hl1" /I "../game_shared/hl2" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../../dlls" /I "../dlls/hl1_dll" /I "../dlls/hl2_dll" /D "HL1_DLL" /D "USES_SAVERESTORE" /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "_DEBUG" /D fopen=dont_use_fopen /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /FR /Yu"cbase.h" /FD /c
# ADD CPP /nologo /G6 /W4 /Gm /GR /ZI /Od /I "../game_shared/cstrike" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../dlls/cstrike" /D "_DEBUG" /D fopen=dont_use_fopen /D "CSTRIKE_DLL" /D "USES_SAVERESTORE" /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /D "PROTECTED_THINGS_ENABLE" /FR /Yu"cbase.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\engine" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\engine" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /debug /machine:I386 /out:"Debug_hl1/server.dll" /implib:".\Debug_hl1\server.lib" /libpath:"..\lib\public"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /debug /machine:I386 /out:"Debug_CounterStrike/server.dll" /implib:".\Debug_hl1\server.lib" /libpath:"..\lib\public"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying to HL1
TargetDir=.\Debug_CounterStrike
InputPath=.\Debug_CounterStrike\server.dll
SOURCE="$(InputPath)"

"..\..\cstrike\bin\server.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\cstrike\bin\server.dll attrib -r ..\..\cstrike\bin\server.dll 
	if exist $(TargetDir)\server.dll copy $(TargetDir)\server.dll ..\..\cstrike\bin\server.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "hl___Win32_Release_CounterStrike"
# PROP BASE Intermediate_Dir "hl___Win32_Release_CounterStrike"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_CounterStrike"
# PROP Intermediate_Dir "Release_CounterStrike"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /GR /Zi /O2 /I "../game_shared/hl1" /I "../game_shared/hl2" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../../dlls" /I "../dlls/hl1_dll" /I "../dlls/hl2_dll" /D "HL1_DLL" /D "USES_SAVERESTORE" /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "NDEBUG" /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /Fr /Yu"cbase.h" /FD /c
# ADD CPP /nologo /G6 /W4 /GR /Zi /O2 /I "../game_shared/cstrike" /I "./" /I "../Public" /I "../game_shared" /I "../utils/common" /I "../dlls" /I "../dlls/cstrike" /D "NDEBUG" /D "CSTRIKE_DLL" /D "USES_SAVERESTORE" /D "GAME_DLL" /D sprintf=use_Q_snprintf_instead_of_sprintf /D "_WINDOWS" /D "VECTOR" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WIN32" /D "PROTECTED_THINGS_ENABLE" /Fr /Yu"cbase.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib:"LIBCD" /out:"Release_hl1/server.dll" /libpath:"..\lib\public"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 user32.lib advapi32.lib winmm.lib /nologo /base:"0x22000000" /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib:"LIBCD" /out:"Release_CounterStrike/server.dll" /libpath:"..\lib\public"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying to HL1
TargetDir=.\Release_CounterStrike
InputPath=.\Release_CounterStrike\server.dll
SOURCE="$(InputPath)"

"..\..\cstrike\bin\server.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\cstrike\bin\server.dll attrib -r ..\..\cstrike\bin\server.dll 
	if exist $(TargetDir)\server.dll copy $(TargetDir)\server.dll ..\..\cstrike\bin\server.dll 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "hl - Win32 Release HL2"
# Name "hl - Win32 Debug HL2"
# Name "hl - Win32 Debug TF2"
# Name "hl - Win32 Release TF2"
# Name "hl - Win32 Release HL1"
# Name "hl - Win32 Debug HL1"
# Name "hl - Win32 Debug CounterStrike"
# Name "hl - Win32 Release CounterStrike"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Group "Precompiled Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"cbase.h"
# End Source File
# End Group
# Begin Group "temporary entities"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\hl2_dll\antlion_dust.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\antlion_dust.h
# End Source File
# Begin Source File

SOURCE=.\hl2_dll\assassin_smoke.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\assassin_smoke.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\basetempentity.cpp
# End Source File
# Begin Source File

SOURCE=.\cplane.cpp
# End Source File
# Begin Source File

SOURCE=.\event_tempentity_tester.cpp
# End Source File
# Begin Source File

SOURCE=.\movie_explosion.cpp
# End Source File
# Begin Source File

SOURCE=.\particle_fire.cpp
# End Source File
# Begin Source File

SOURCE=.\particle_smokegrenade.cpp
# End Source File
# Begin Source File

SOURCE=.\plasma.cpp
# End Source File
# Begin Source File

SOURCE=.\plasma.h
# End Source File
# Begin Source File

SOURCE=.\hl2_dll\rotorwash.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\smoke_trail.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\smoke_trail.h
# End Source File
# Begin Source File

SOURCE=.\SmokeStack.cpp
# End Source File
# Begin Source File

SOURCE=.\SmokeStack.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\SpriteTrail.cpp
# End Source File
# Begin Source File

SOURCE=.\steamjet.cpp
# End Source File
# Begin Source File

SOURCE=.\steamjet.h
# End Source File
# Begin Source File

SOURCE=.\te.cpp
# End Source File
# Begin Source File

SOURCE=.\te.h
# End Source File
# Begin Source File

SOURCE=.\te_armorricochet.cpp
# End Source File
# Begin Source File

SOURCE=.\te_basebeam.cpp
# End Source File
# Begin Source File

SOURCE=.\te_basebeam.h
# End Source File
# Begin Source File

SOURCE=.\te_beamentpoint.cpp
# End Source File
# Begin Source File

SOURCE=.\te_beaments.cpp
# End Source File
# Begin Source File

SOURCE=.\te_beamlaser.cpp
# End Source File
# Begin Source File

SOURCE=.\te_beampoints.cpp
# End Source File
# Begin Source File

SOURCE=.\te_beamring.cpp
# End Source File
# Begin Source File

SOURCE=.\te_beamringpoint.cpp
# End Source File
# Begin Source File

SOURCE=.\te_beamspline.cpp
# End Source File
# Begin Source File

SOURCE=.\te_bloodsprite.cpp
# End Source File
# Begin Source File

SOURCE=.\te_bloodstream.cpp
# End Source File
# Begin Source File

SOURCE=.\te_breakmodel.cpp
# End Source File
# Begin Source File

SOURCE=.\te_bspdecal.cpp
# End Source File
# Begin Source File

SOURCE=.\te_bubbles.cpp
# End Source File
# Begin Source File

SOURCE=.\te_bubbletrail.cpp
# End Source File
# Begin Source File

SOURCE=.\te_decal.cpp
# End Source File
# Begin Source File

SOURCE=.\te_dynamiclight.cpp
# End Source File
# Begin Source File

SOURCE=.\te_energysplash.cpp
# End Source File
# Begin Source File

SOURCE=.\te_explosion.cpp
# End Source File
# Begin Source File

SOURCE=.\te_fizz.cpp
# End Source File
# Begin Source File

SOURCE=.\te_fogripple.cpp
# End Source File
# Begin Source File

SOURCE=.\te_footprintdecal.cpp
# End Source File
# Begin Source File

SOURCE=.\te_GlassShatter.cpp
# End Source File
# Begin Source File

SOURCE=.\te_glowsprite.cpp
# End Source File
# Begin Source File

SOURCE=.\te_impact.cpp
# End Source File
# Begin Source File

SOURCE=.\te_killplayerattachments.cpp
# End Source File
# Begin Source File

SOURCE=.\te_largefunnel.cpp
# End Source File
# Begin Source File

SOURCE=.\te_muzzleflash.cpp
# End Source File
# Begin Source File

SOURCE=.\te_particlesystem.cpp
# End Source File
# Begin Source File

SOURCE=.\te_particlesystem.h
# End Source File
# Begin Source File

SOURCE=.\te_playerdecal.cpp
# End Source File
# Begin Source File

SOURCE=.\te_showline.cpp
# End Source File
# Begin Source File

SOURCE=.\te_smoke.cpp
# End Source File
# Begin Source File

SOURCE=.\te_sparks.cpp
# End Source File
# Begin Source File

SOURCE=.\te_sprite.cpp
# End Source File
# Begin Source File

SOURCE=.\te_spritespray.cpp
# End Source File
# Begin Source File

SOURCE=.\te_textmessage.cpp
# End Source File
# Begin Source File

SOURCE=.\te_worlddecal.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\usermessages.cpp
# End Source File
# End Group
# Begin Group "HL2 DLL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\hl2_dll\ai_allymanager.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\AI_Interactions.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\ar2_explosion.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\ar2_explosion.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\BaseBludgeonWeapon.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\BaseBludgeonWeapon.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\basehlcombatweapon.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl2\basehlcombatweapon_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\CBaseHelicopter.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\CBaseHelicopter.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\CBaseSpriteProjectile.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\CBaseSpriteProjectile.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\energy_wave.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\energy_wave.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\env_speaker.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\extinguisherjet.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\extinguisherjet.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\Func_Monitor.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\func_recharge.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\func_tank.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_ar2.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_ar2.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_beam.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_beam.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_brickbat.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_brickbat.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Ignore_Default_Tool 1
# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP BASE Ignore_Default_Tool 1
# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_bugbait.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_bugbait.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_energy.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_energy.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_frag.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_frag.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_homer.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_homer.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_hopwire.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_molotov.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_molotov.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Ignore_Default_Tool 1
# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP BASE Ignore_Default_Tool 1
# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_pathfollower.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_pathfollower.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_satchel.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_satchel.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_scanner.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_scanner.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_spit.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_spit.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_tripmine.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_tripmine.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_tripwire.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_tripwire.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\hl2_client.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\hl2_eventlog.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl2\hl2_gamerules.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl2\hl2_gamerules.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\HL2_Player.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\HL2_Player.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\hl2_playerlocaldata.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\hl2_playerlocaldata.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl2_shareddefs.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl2\hl2_usermessages.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl_gamemovement.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl_movedata.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\hl_playermove.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\item_ammo.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\item_battery.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\item_healthkit.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\item_suit.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\look_door.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\monster_dummy.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_alyx.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_alyx.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_antlion.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_antliongrub.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_antliongrub.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_antlionguard.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_assassin.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_assassin.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_attackchopper.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Barnacle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Barnacle.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Barney.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_BaseZombie.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_BaseZombie.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_breen.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_bullseye.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Bullseye.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_bullsquid.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Bullsquid.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_citizen17.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_citizen17.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Combine.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Combine.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_combinecamera.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_combinedropship.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_CombineE.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_CombineE.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_CombineGuard.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_combinegunship.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_CombineS.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_CombineS.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Conscript.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Conscript.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_crabsynth.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_cranedriver.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_crow.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_eli.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_EnemyFinder.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_fastzombie.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Headcrab.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Headcrab.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_houndeye.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Houndeye.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_hydra.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_hydra.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\NPC_Ichthyosaur.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_kleiner.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_launcher.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\npc_Leader.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\npc_Leader.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_lightstalk.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_lightstalk.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Manhack.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Manhack.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_metropolice.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_metropolice.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_missiledefense.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_monk.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_mortarsynth.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_mortarsynth.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_mossman.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_odell.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_odell.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_PoisonZombie.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\NPC_Roller.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\NPC_Roller.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\NPC_RollerBuddy.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_rollerbuddy.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_rollerbull.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\NPC_RollerDozer.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_rollermine.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\NPC_RollerTurret.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Scanner.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Spotlight.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_SScanner.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_SScanner.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_SScanner_Beam.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_SScanner_Beam.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_stalker.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_stalker.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_strider.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\npc_Talker.cpp
# End Source File
# Begin Source File

SOURCE=.\npc_Talker.h
# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_turret_ceiling.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_turret_floor.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_vehicledriver.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Vortigaunt.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_Vortigaunt.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_wscanner.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_WScanner.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_zombie.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\player_control.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\player_control.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\player_manhack.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\player_missile.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\point_apc_controller.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\Point_Camera.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\proto_sniper.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\rotorwash.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\Scanner_Shield.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\Scanner_Shield.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\SpotlightEnd.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\SpotlightEnd.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\vehicle_airboat.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\vehicle_apc.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\vehicle_base.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vehicle_base.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\vehicle_baseserver.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\Vehicle_Chopper.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\vehicle_crane.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\vehicle_jeep.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\WaterLODControl.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_ar1.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_ar2.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_ar2.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_Binoculars.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_brickbat.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_brickbat.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Ignore_Default_Tool 1
# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP BASE Ignore_Default_Tool 1
# PROP Exclude_From_Build 1
# PROP Ignore_Default_Tool 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_bugbait.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_cguard.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_crowbar.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\weapon_cubemap.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_extinguisher.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_flaregun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_flaregun.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_frag.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_gauss.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_gauss.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_hgm1.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_hopwire.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_iceaxe.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_immolator.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_irifle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_manhack.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_molotov.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_molotov.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_physcannon.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_pistol.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_rollerwand.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_rpg.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_rpg.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_shotgun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_slam.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_slam.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_smg1.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_smg2.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_SniperRifle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_stickylauncher.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_stunstick.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_thumper.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_tripwire.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_tripwire.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "TF2 DLL"

# PROP Default_Filter "h;cpp"
# Begin Group "TF2 Classes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tf2_dll\tf_class_commando.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_commando.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_defender.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_defender.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_escort.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_escort.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_infiltrator.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_infiltrator.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_medic.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_medic.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_pyro.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_pyro.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_recon.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_recon.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_sapper.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_sapper.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_sniper.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_sniper.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_support.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_class_support.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "TF2 Movement"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\game_shared\tf_gamemovement.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf_gamemovement.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_chooser.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_chooser.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_commando.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_commando.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_defender.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_defender.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_escort.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_escort.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_infiltrator.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_infiltrator.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_medic.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_medic.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_pyro.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_pyro.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_recon.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_recon.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_sapper.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_sapper.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_sniper.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_sniper.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_support.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamemovement_support.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf_movedata.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_playermove.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\tf2_dll\basecombatcharacter_tf2.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\basehlcombatweapon.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl2\basehlcombatweapon_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\baseobject_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\baseobject_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\BasePropDoor.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\basetfcombatweapon_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\basetfcombatweapon_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\basetfplayer_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\basetfplayer_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\basetfvehicle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\basetfvehicle.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\bot_base.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\ControlZone.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\controlzone.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cplane.h
# End Source File
# Begin Source File

SOURCE=.\CRagdollMagnet.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\demo_entities.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\entity_burn_effect.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\entity_burn_effect.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\EntityFlame.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\env_fallingrocks.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\env_laserdesignation.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\env_laserdesignation.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\env_meteor.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\env_meteor.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\env_meteor_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\env_meteor_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\envmicrophone.h
# End Source File
# Begin Source File

SOURCE=.\tf2_dll\fire_damage_mgr.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\fire_damage_mgr.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\gasoline_blob.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\gasoline_blob.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\gasoline_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_antipersonnel.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_antipersonnel.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_base_empable.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_base_empable.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_emp.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_emp.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\grenade_hopwire.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_limpetmine.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_limpetmine.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_objectsapper.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_objectsapper.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_rocket.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_rocket.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\grenade_stickybomb.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\h_cycler.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_handgrenade.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\hl2_player_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\ihasbuildpoints.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_act.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_act.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_add_resources.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_buildpoint.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_buildpoint.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_customtech.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_customtech.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_input_playsound.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_input_resetbanks.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_input_resetobjects.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_input_respawnplayers.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_minimappulse.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_output_team.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_vehicle_bay.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\info_vehicle_bay.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\iscorer.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\mapdata_server.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\mapdata_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\menu_base.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\menu_base.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\mortar_round.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\npc_bug_builder.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\npc_bug_builder.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\npc_bug_hole.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\npc_bug_hole.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\npc_bug_warrior.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\npc_bug_warrior.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\npc_vehicledriver.h
# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_assist.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_assist.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_buildsentrygun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_buildsentrygun.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_buildshieldwall.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_buildshieldwall.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_events.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_events.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_heal.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_heal.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_helpers.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_helpers.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_killmortarguy.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_killmortarguy.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_mortar_attack.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_mortar_attack.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_player.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_player.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_repair.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_repair.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_resourcepump.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_resourcepump.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_resupply.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\order_resupply.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\orders.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\orders.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\physics_shared.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\plasmaprojectile.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\plasmaprojectile.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\plasmaprojectile_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\plasmaprojectile_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\ragdoll_shadow.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\ragdoll_shadow.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\ragdoll_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\resource_chunk.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\resource_chunk.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\saverestore_utlsymbol.h
# End Source File
# Begin Source File

SOURCE=.\tf2_dll\sensor_tf_team.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\solidsetdefaults.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\spark.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\SpriteTrail.h
# End Source File
# Begin Source File

SOURCE=.\tf2_dll\team_messages.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\team_messages.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\techtree.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\techtree.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\techtree_parse.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf2_eventlog.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_ai_hint.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_basecombatweapon.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_basecombatweapon.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_basefourwheelvehicle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_basefourwheelvehicle.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_client.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_filters.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_flare.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_flare.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_func_construction_yard.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_func_construction_yard.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_func_mass_teleport.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_func_mass_teleport.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_func_no_build.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_func_no_build.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_func_resource.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_func_resource.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_func_weldable_door.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_func_weldable_door.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamerules.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_gamerules.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_hintmanager.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_hintmanager.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_armor_upgrade.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_armor_upgrade.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_barbed_wire.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_barbed_wire.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_base_manned_gun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_base_manned_gun.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_basedrivergun_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_basedrivergun_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_baseupgrade_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_baseupgrade_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_buff_station.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_buff_station.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_bunker.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_bunker.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_dragonsteeth.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_dragonsteeth.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_driver_machinegun_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_driver_machinegun_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_empgenerator.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_empgenerator.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_explosives.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_manned_missilelauncher.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_manned_missilelauncher.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_manned_plasmagun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_manned_plasmagun.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_manned_plasmagun_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_obj_manned_plasmagun_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_manned_shield.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_mapdefined.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_mapdefined.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_mcv_selection_panel.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_mcv_selection_panel.h
# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_mortar.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_mortar.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_powerpack.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_powerpack.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_rallyflag.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_rallyflag.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_resourcepump.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_resourcepump.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_respawn_station.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_respawn_station.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_resupply.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_resupply.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_sandbag_bunker.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_sandbag_bunker.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_selfheal.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_selfheal.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_sentrygun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_sentrygun.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_shieldwall.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_shieldwall.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_tower.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_tower.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_tunnel.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_vehicleboost.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_obj_vehicleboost.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_player.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_player.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_player_death.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_player_resource.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_player_resource.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_playeranimstate.h
# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_playerclass.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_playerclass.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_playerlocaldata.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_playerlocaldata.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf_reconvars.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf_reconvars.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_shareddefs.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf_shareddefs.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_shield.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_shield.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_shield_flat.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_shield_flat.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_shield_mobile_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_shieldgrenade.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_shieldgrenade.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf_shieldshared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf_shieldshared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_stats.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_stats.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_stressentities.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf_tacticalmap.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_team.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_team.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_teamspawnpoint.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tf_usermessages.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_battering_ram.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_battering_ram.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_flatbed.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_flatbed.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_mortar.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_mortar.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_motorcycle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_siege_tower.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_siege_tower.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_tank.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_tank.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_teleport_station.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_teleport_station.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_wagon.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_vehicle_wagon.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf_vehicleshared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_walker_base.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_walker_base.h
# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_walker_ministrider.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_walker_ministrider.h
# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_walker_strider.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\tf_walker_strider.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\tfclassdata_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\TFClassData_Shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\touchlink.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\trigger_fall.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tf2_dll\trigger_skybox.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\vehicle_crane.h
# End Source File
# Begin Source File

SOURCE=.\vehicle_sounds.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_arcwelder.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_basecombatobject.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_basecombatobject.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_builder.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_builder.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_basegrenade.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_basegrenade.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_burstrifle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_grenade.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_grenade_emp.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_laserrifle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_plasma_grenade_launcher.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_plasmarifle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_shotgun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_usedwithshieldbase.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combat_usedwithshieldbase.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combatshield.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_combatshield.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_drainbeam.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_drainbeam.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_flame_thrower.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_flame_thrower.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_gas_can.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_gas_can.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_grenade_rocket.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_grenade_rocket.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_harpoon.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_limpetmine.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_limpetmine.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_minigun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_obj_empgenerator.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_obj_rallyflag.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_objectselection.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_objectselection.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl2_dll\weapon_physcannon.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_plasmarifle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_repairgun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_repairgun.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_rocketlauncher.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_rocketlauncher.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_shield.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_shield.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_shieldgrenade.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_twohandedcontainer.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\weapon_twohandedcontainer.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "HL1 DLL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\hl1_dll\hl1_ai_basenpc.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_ai_basenpc.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_basecombatweapon.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl1\hl1_basecombatweapon_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl1\hl1_basecombatweapon_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_basegrenade.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_basegrenade.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_CBaseHelicopter.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_client.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_ents.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_ents.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_env_speaker.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_eventlog.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_func_recharge.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_func_tank.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl1\hl1_gamemovement.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl1\hl1_gamemovement.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl1\hl1_gamerules.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl1\hl1_gamerules.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_grenade_mp5.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_grenade_mp5.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_grenade_spit.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_grenade_spit.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_item_ammo.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_item_battery.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_item_healthkit.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_item_longjump.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_item_suit.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_items.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_items.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_monstermaker.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_monstermaker.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_aflock.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_agrunt.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_apache.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_barnacle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_barnacle.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_barney.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_barney.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_bigmomma.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_bloater.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_bullsquid.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_bullsquid.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_controller.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_gargantua.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_gargantua.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_gman.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_hassassin.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_headcrab.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_headcrab.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_hgrunt.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_hgrunt.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_hornet.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_hornet.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_houndeye.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_houndeye.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_ichthyosaur.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_ichthyosaur.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_leech.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_nihilanth.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_osprey.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_roach.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_scientist.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_scientist.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_snark.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_snark.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_talker.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_talker.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_tentacle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_turret.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_vortigaunt.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_vortigaunt.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_zombie.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_npc_zombie.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_player.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_player.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl1\hl1_player_shared.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_te_beamfollow.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_te_boltstick.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\hl1\hl1_usermessages.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_357.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_crossbow.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_crowbar.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_egon.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_gauss.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_glock.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_handgrenade.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_hornetgun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_mp5.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_mp5.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_rpg.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_rpg.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_satchel.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_satchel.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_shotgun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_snark.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weapon_tripmine.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hl1_dll\hl1_weaponbox.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "CounterStrike DLL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cstrike\cs_bot_temp.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\cs_client.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\cs_client.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\cs_eventlog.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_gamemovement.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_gamerules.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_gamerules.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\cs_hostage.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\cs_hostage.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\cs_player.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\cs_player.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_player_shared.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_playeranimstate.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_playeranimstate.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\cs_playermove.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_shareddefs.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_shareddefs.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\cs_team.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\cs_team.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_usermessages.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_weapon_parse.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\cs_weapon_parse.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\flashbang_projectile.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\flashbang_projectile.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\func_bomb_target.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\func_buy_zone.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\hegrenade_projectile.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\hegrenade_projectile.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\item_assaultsuit.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\item_kevlar.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\mapinfo.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrike\mapinfo.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_ak47.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_aug.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_awp.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_basecsgrenade.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_basecsgrenade.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_c4.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_c4.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_csbase.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_csbase.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_csbasegun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_csbasegun.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_deagle.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_famas.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_fiveseven.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_flashbang.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_g3sg1.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_galil.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_glock.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_hegrenade.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_knife.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_m249.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_m3.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_m4a1.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_mac10.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_mp5navy.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_p228.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_p90.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_scout.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_sg550.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_sg552.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_ump45.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\cstrike\weapon_usp.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\actanimating.cpp
# End Source File
# Begin Source File

SOURCE=.\actanimating.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\activitylist.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\activitylist.h
# End Source File
# Begin Source File

SOURCE=.\AI_Activity.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\ai_activity.h
# End Source File
# Begin Source File

SOURCE=.\AI_BaseActor.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_BaseActor.h
# End Source File
# Begin Source File

SOURCE=.\AI_BaseHumanoid.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_BaseHumanoid.h
# End Source File
# Begin Source File

SOURCE=.\AI_BaseHumanoid_Movement.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_BaseHumanoid_Movement.h
# End Source File
# Begin Source File

SOURCE=.\AI_BaseNPC.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_BaseNPC.h
# End Source File
# Begin Source File

SOURCE=.\AI_BaseNPC_Flyer.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_BaseNPC_Flyer.h
# End Source File
# Begin Source File

SOURCE=.\AI_BaseNPC_Flyer_New.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_BaseNPC_Flyer_New.h
# End Source File
# Begin Source File

SOURCE=.\AI_BaseNPC_Movement.cpp
# End Source File
# Begin Source File

SOURCE=.\ai_basenpc_physicsflyer.cpp
# End Source File
# Begin Source File

SOURCE=.\ai_basenpc_physicsflyer.h
# End Source File
# Begin Source File

SOURCE=.\AI_BaseNPC_Schedule.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_BaseNPC_Squad.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Behavior.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Behavior.h
# End Source File
# Begin Source File

SOURCE=.\ai_behavior_assault.cpp
# End Source File
# Begin Source File

SOURCE=.\ai_behavior_assault.h
# End Source File
# Begin Source File

SOURCE=.\AI_Behavior_Follow.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Behavior_Follow.h
# End Source File
# Begin Source File

SOURCE=.\AI_Behavior_Lead.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Behavior_Lead.h
# End Source File
# Begin Source File

SOURCE=.\AI_Behavior_Standoff.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Behavior_Standoff.h
# End Source File
# Begin Source File

SOURCE=.\AI_Component.h
# End Source File
# Begin Source File

SOURCE=.\AI_ConCommands.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Condition.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Condition.h
# End Source File
# Begin Source File

SOURCE=.\AI_Criteria.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Criteria.h
# End Source File
# Begin Source File

SOURCE=.\AI_Debug.h
# End Source File
# Begin Source File

SOURCE=.\AI_Default.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Default.h
# End Source File
# Begin Source File

SOURCE=.\AI_DynamicLink.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_DynamicLink.h
# End Source File
# Begin Source File

SOURCE=.\AI_GoalEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_GoalEntity.h
# End Source File
# Begin Source File

SOURCE=.\AI_Hint.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Hint.h
# End Source File
# Begin Source File

SOURCE=.\AI_Hull.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Hull.h
# End Source File
# Begin Source File

SOURCE=.\AI_InitUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_InitUtils.h
# End Source File
# Begin Source File

SOURCE=.\AI_Interest_Target.h
# End Source File
# Begin Source File

SOURCE=.\AI_Link.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Link.h
# End Source File
# Begin Source File

SOURCE=.\AI_LocalNavigator.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_LocalNavigator.h
# End Source File
# Begin Source File

SOURCE=.\AI_Memory.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Memory.h
# End Source File
# Begin Source File

SOURCE=.\AI_Motor.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Motor.h
# End Source File
# Begin Source File

SOURCE=.\AI_MoveProbe.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_MoveProbe.h
# End Source File
# Begin Source File

SOURCE=.\AI_MoveShoot.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_MoveShoot.h
# End Source File
# Begin Source File

SOURCE=.\AI_MoveSolver.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_MoveSolver.h
# End Source File
# Begin Source File

SOURCE=.\AI_MoveTypes.h
# End Source File
# Begin Source File

SOURCE=.\AI_Namespaces.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Namespaces.h
# End Source File
# Begin Source File

SOURCE=.\AI_Nav_Property.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Nav_Property.h
# End Source File
# Begin Source File

SOURCE=.\AI_NavGoalType.h
# End Source File
# Begin Source File

SOURCE=.\AI_Navigator.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Navigator.h
# End Source File
# Begin Source File

SOURCE=.\AI_NavType.h
# End Source File
# Begin Source File

SOURCE=.\AI_Network.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Network.h
# End Source File
# Begin Source File

SOURCE=.\AI_NetworkManager.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_NetworkManager.h
# End Source File
# Begin Source File

SOURCE=.\AI_Node.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Node.h
# End Source File
# Begin Source File

SOURCE=.\AI_NPCState.h
# End Source File
# Begin Source File

SOURCE=.\AI_Pathfinder.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Pathfinder.h
# End Source File
# Begin Source File

SOURCE=.\AI_PlaneSolver.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_PlaneSolver.h
# End Source File
# Begin Source File

SOURCE=.\AI_ResponseSystem.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_ResponseSystem.h
# End Source File
# Begin Source File

SOURCE=.\AI_Route.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Route.h
# End Source File
# Begin Source File

SOURCE=.\AI_RouteDist.h
# End Source File
# Begin Source File

SOURCE=.\AI_SaveRestore.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_SaveRestore.h
# End Source File
# Begin Source File

SOURCE=.\AI_Schedule.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Schedule.h
# End Source File
# Begin Source File

SOURCE=.\AI_ScriptConditions.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_ScriptConditions.h
# End Source File
# Begin Source File

SOURCE=.\AI_Senses.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Senses.h
# End Source File
# Begin Source File

SOURCE=.\AI_Speech.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Speech.h
# End Source File
# Begin Source File

SOURCE=.\AI_Squad.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Squad.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\AI_SquadSlot.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_SquadSlot.h
# End Source File
# Begin Source File

SOURCE=.\AI_TacticalServices.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_TacticalServices.h
# End Source File
# Begin Source File

SOURCE=.\AI_Task.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Task.h
# End Source File
# Begin Source File

SOURCE=.\ai_trackpather.cpp
# End Source File
# Begin Source File

SOURCE=.\ai_trackpather.h
# End Source File
# Begin Source File

SOURCE=.\ai_utils.cpp
# End Source File
# Begin Source File

SOURCE=.\ai_utils.h
# End Source File
# Begin Source File

SOURCE=.\AI_Waypoint.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Waypoint.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\ammodef.cpp
# End Source File
# Begin Source File

SOURCE=.\animating.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\animation.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\animation.h
# End Source File
# Begin Source File

SOURCE=.\base_transmit_proxy.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseAnimating.h
# End Source File
# Begin Source File

SOURCE=.\BaseAnimatingOverlay.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseAnimatingOverlay.h
# End Source File
# Begin Source File

SOURCE=.\BaseCombatCharacter.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCombatCharacter.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\basecombatcharacter_shared.cpp
# End Source File
# Begin Source File

SOURCE=.\basecombatweapon.cpp
# End Source File
# Begin Source File

SOURCE=.\basecombatweapon.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\basecombatweapon_shared.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\basecombatweapon_shared.h
# End Source File
# Begin Source File

SOURCE=.\BaseEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseEntity.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\baseentity_shared.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\baseentity_shared.h
# End Source File
# Begin Source File

SOURCE=.\BaseFlex.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseFlex.h
# End Source File
# Begin Source File

SOURCE=.\basegrenade_concussion.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\basegrenade_contact.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\game_shared\basegrenade_shared.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\basegrenade_shared.h
# End Source File
# Begin Source File

SOURCE=.\basegrenade_timed.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\basenetworkable.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\baseparticleentity.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\baseparticleentity.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\baseplayer_shared.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\baseplayer_shared.h
# End Source File
# Begin Source File

SOURCE=.\BaseToggle.h
# End Source File
# Begin Source File

SOURCE=.\BaseViewModel.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseViewModel.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\baseviewmodel_shared.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\baseviewmodel_shared.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\beam_shared.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\beam_shared.h
# End Source File
# Begin Source File

SOURCE=..\public\bitbuf.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\BitString.cpp
# End Source File
# Begin Source File

SOURCE=.\BitString.h
# End Source File
# Begin Source File

SOURCE=.\bmodels.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\bone_setup.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\game_shared\bone_setup.h
# End Source File
# Begin Source File

SOURCE=.\buttons.cpp
# End Source File
# Begin Source File

SOURCE=.\buttons.h
# End Source File
# Begin Source File

SOURCE=.\cbase.cpp
# End Source File
# Begin Source File

SOURCE=.\cbase.h
# End Source File
# Begin Source File

SOURCE=..\public\characterset.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\public\checksum_crc.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\checksum_crc.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\choreoactor.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\game_shared\choreoactor.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\choreochannel.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\game_shared\choreochannel.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\choreoevent.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\game_shared\choreoevent.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\choreoscene.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\game_shared\choreoscene.h
# End Source File
# Begin Source File

SOURCE=.\client.cpp
# End Source File
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\collisionproperty.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\collisionproperty.h
# End Source File
# Begin Source File

SOURCE=..\Public\CollisionUtils.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\CollisionUtils.h
# End Source File
# Begin Source File

SOURCE=.\ControlEntities.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\convar.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\convar.h
# End Source File
# Begin Source File

SOURCE=.\CRagdollMagnet.cpp
# End Source File
# Begin Source File

SOURCE=.\CRagdollMagnet.h
# End Source File
# Begin Source File

SOURCE=.\CTerrainMorph.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\damagemodifier.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\decals.cpp
# End Source File
# Begin Source File

SOURCE=.\doors.cpp
# End Source File
# Begin Source File

SOURCE=.\doors.h
# End Source File
# Begin Source File

SOURCE=..\public\dt_send.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\dynamiclight.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\edict.h
# End Source File
# Begin Source File

SOURCE=..\Public\Editor_SendCommand.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\Editor_SendCommand.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\effect_dispatch_data.cpp
# End Source File
# Begin Source File

SOURCE=.\effects.cpp
# End Source File
# Begin Source File

SOURCE=.\EffectsServer.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\ehandle.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\eiface.h
# End Source File
# Begin Source File

SOURCE=.\enginecallback.h
# End Source File
# Begin Source File

SOURCE=.\entityapi.h
# End Source File
# Begin Source File

SOURCE=.\EntityFlame.cpp
# End Source File
# Begin Source File

SOURCE=.\EntityFlame.h
# End Source File
# Begin Source File

SOURCE=.\EntityInput.h
# End Source File
# Begin Source File

SOURCE=.\EntityList.cpp
# End Source File
# Begin Source File

SOURCE=.\EntityList.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\entitylist_base.cpp
# End Source File
# Begin Source File

SOURCE=.\entitymapdata.h
# End Source File
# Begin Source File

SOURCE=.\EntityOutput.h
# End Source File
# Begin Source File

SOURCE=.\env_entity_maker.cpp
# End Source File
# Begin Source File

SOURCE=.\env_screenoverlay.cpp
# End Source File
# Begin Source File

SOURCE=.\env_texturetoggle.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\env_wind_shared.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\env_wind_shared.h
# End Source File
# Begin Source File

SOURCE=.\env_zoom.cpp
# End Source File
# Begin Source File

SOURCE=.\EnvBeam.cpp
# End Source File
# Begin Source File

SOURCE=.\EnvFade.cpp
# End Source File
# Begin Source File

SOURCE=.\EnvLaser.cpp
# End Source File
# Begin Source File

SOURCE=.\EnvLaser.h
# End Source File
# Begin Source File

SOURCE=.\EnvMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\EnvMessage.h
# End Source File
# Begin Source File

SOURCE=.\EnvMicrophone.cpp
# End Source File
# Begin Source File

SOURCE=.\EnvShake.cpp
# End Source File
# Begin Source File

SOURCE=.\EnvSpark.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\event_flags.h
# End Source File
# Begin Source File

SOURCE=.\event_tempentity_tester.h
# End Source File
# Begin Source File

SOURCE=.\EventLog.cpp
# End Source File
# Begin Source File

SOURCE=.\EventQueue.h
# End Source File
# Begin Source File

SOURCE=.\explode.cpp
# End Source File
# Begin Source File

SOURCE=.\explode.h
# End Source File
# Begin Source File

SOURCE=..\public\filesystem_helpers.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\filters.cpp
# End Source File
# Begin Source File

SOURCE=.\filters.h
# End Source File
# Begin Source File

SOURCE=.\fire.cpp
# End Source File
# Begin Source File

SOURCE=.\fire.h
# End Source File
# Begin Source File

SOURCE=.\fire_smoke.cpp
# End Source File
# Begin Source File

SOURCE=.\fire_smoke.h
# End Source File
# Begin Source File

SOURCE=.\fogcontroller.cpp
# End Source File
# Begin Source File

SOURCE=.\fourwheelvehiclephysics.cpp
# End Source File
# Begin Source File

SOURCE=.\fourwheelvehiclephysics.h
# End Source File
# Begin Source File

SOURCE=.\func_areaportal.cpp
# End Source File
# Begin Source File

SOURCE=.\func_areaportalbase.cpp
# End Source File
# Begin Source File

SOURCE=.\func_areaportalbase.h
# End Source File
# Begin Source File

SOURCE=.\func_areaportalwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\func_areaportalwindow.h
# End Source File
# Begin Source File

SOURCE=.\func_break.cpp
# End Source File
# Begin Source File

SOURCE=.\func_break.h
# End Source File
# Begin Source File

SOURCE=.\func_breakablesurf.cpp
# End Source File
# Begin Source File

SOURCE=.\func_breakablesurf.h
# End Source File
# Begin Source File

SOURCE=.\func_dust.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\func_dust_shared.h
# End Source File
# Begin Source File

SOURCE=.\func_lod.cpp
# End Source File
# Begin Source File

SOURCE=.\func_movelinear.cpp
# End Source File
# Begin Source File

SOURCE=.\func_movelinear.h
# End Source File
# Begin Source File

SOURCE=.\func_occluder.cpp
# End Source File
# Begin Source File

SOURCE=.\func_smokevolume.cpp
# End Source File
# Begin Source File

SOURCE=.\game.cpp
# End Source File
# Begin Source File

SOURCE=.\game.h
# End Source File
# Begin Source File

SOURCE=.\game_ui.cpp
# End Source File
# Begin Source File

SOURCE=.\gamehandle.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\GameInterface.cpp
# End Source File
# Begin Source File

SOURCE=.\GameInterface.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\gamemovement.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\gamemovement.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\gamerules.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\gamerules.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\gamestringpool.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\gamestringpool.h
# End Source File
# Begin Source File

SOURCE=.\gametrace_dll.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\gamevars_shared.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\gamevars_shared.h
# End Source File
# Begin Source File

SOURCE=.\genericactor.cpp
# End Source File
# Begin Source File

SOURCE=.\genericmonster.cpp
# End Source File
# Begin Source File

SOURCE=.\Gib.cpp
# End Source File
# Begin Source File

SOURCE=.\Gib.h
# End Source File
# Begin Source File

SOURCE=.\globals.cpp
# End Source File
# Begin Source File

SOURCE=.\globalstate.cpp
# End Source File
# Begin Source File

SOURCE=.\globalstate.h
# End Source File
# Begin Source File

SOURCE=.\globalstate_private.h
# End Source File
# Begin Source File

SOURCE=.\GrenadeThrown.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\GrenadeThrown.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\GunTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\h_ai.cpp
# End Source File
# Begin Source File

SOURCE=.\h_cycler.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\h_export.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\hierarchy.cpp
# End Source File
# Begin Source File

SOURCE=.\hierarchy.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\ichoreoeventcallback.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\igamesystem.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\igamesystem.h
# End Source File
# Begin Source File

SOURCE=.\init_factory.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\init_factory.h
# End Source File
# Begin Source File

SOURCE=..\Public\interface.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=.\Intermission.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\interval.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\interval.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\iscenetokenprocessor.h
# End Source File
# Begin Source File

SOURCE=.\iservervehicle.h
# End Source File
# Begin Source File

SOURCE=.\item_world.cpp
# End Source File
# Begin Source File

SOURCE=.\items.h
# End Source File
# Begin Source File

SOURCE=..\Public\ivoiceserver.h
# End Source File
# Begin Source File

SOURCE=..\common\KeyFrame\keyframe.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\KeyFrame\keyframe.h
# End Source File
# Begin Source File

SOURCE=..\public\KeyValues.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\lightglow.cpp
# End Source File
# Begin Source File

SOURCE=.\lights.cpp
# End Source File
# Begin Source File

SOURCE=.\lights.h
# End Source File
# Begin Source File

SOURCE=.\LockSounds.h
# End Source File
# Begin Source File

SOURCE=.\logic_navigation.cpp
# End Source File
# Begin Source File

SOURCE=.\LogicAuto.cpp
# End Source File
# Begin Source File

SOURCE=.\LogicEntities.cpp
# End Source File
# Begin Source File

SOURCE=.\LogicRelay.cpp
# End Source File
# Begin Source File

SOURCE=.\mapentities.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\maprules.cpp
# End Source File
# Begin Source File

SOURCE=.\maprules.h
# End Source File
# Begin Source File

SOURCE=..\Public\Mathlib.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\MATHLIB.H
# End Source File
# Begin Source File

SOURCE=..\Public\measure_section.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\measure_section.h
# End Source File
# Begin Source File

SOURCE=..\Public\mem_fgets.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\mem_fgets.h
# End Source File
# Begin Source File

SOURCE=..\public\tier0\memoverride.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\MemPool.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\MemPool.h
# End Source File
# Begin Source File

SOURCE=.\message_entity.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\model_types.h
# End Source File
# Begin Source File

SOURCE=.\ModelEntities.cpp
# End Source File
# Begin Source File

SOURCE=.\monstermaker.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\monstermaker.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\movehelper_server.cpp
# End Source File
# Begin Source File

SOURCE=.\movehelper_server.h
# End Source File
# Begin Source File

SOURCE=.\Movement.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\movevars_shared.cpp
# End Source File
# Begin Source File

SOURCE=.\movie_explosion.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\multiplay_gamerules.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\multiplay_gamerules.h
# End Source File
# Begin Source File

SOURCE=.\NDebugOverlay.cpp
# End Source File
# Begin Source File

SOURCE=.\NDebugOverlay.h
# End Source File
# Begin Source File

SOURCE=.\netstatemgr.cpp
# End Source File
# Begin Source File

SOURCE=.\networkstringtable_gamedll.h
# End Source File
# Begin Source File

SOURCE=..\Public\NetworkStringTableDefs.h
# End Source File
# Begin Source File

SOURCE=..\public\networkvar.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\particle_fire.h
# End Source File
# Begin Source File

SOURCE=.\particle_light.cpp
# End Source File
# Begin Source File

SOURCE=.\particle_light.h
# End Source File
# Begin Source File

SOURCE=.\particle_smokegrenade.h
# End Source File
# Begin Source File

SOURCE=.\pathcorner.cpp
# End Source File
# Begin Source File

SOURCE=.\phys_controller.cpp
# End Source File
# Begin Source File

SOURCE=.\phys_controller.h
# End Source File
# Begin Source File

SOURCE=.\physconstraint.cpp
# End Source File
# Begin Source File

SOURCE=.\physgun.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\physics.cpp
# End Source File
# Begin Source File

SOURCE=.\physics.h
# End Source File
# Begin Source File

SOURCE=.\physics_bone_follower.cpp
# End Source File
# Begin Source File

SOURCE=.\physics_bone_follower.h

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\physics_cannister.cpp
# End Source File
# Begin Source File

SOURCE=.\physics_fx.cpp
# End Source File
# Begin Source File

SOURCE=.\physics_impact_damage.cpp
# End Source File
# Begin Source File

SOURCE=.\physics_main.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\physics_main_shared.cpp
# End Source File
# Begin Source File

SOURCE=.\physics_prop_ragdoll.cpp
# End Source File
# Begin Source File

SOURCE=.\physics_prop_ragdoll.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\physics_saverestore.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\physics_saverestore.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\physics_shared.cpp
# End Source File
# Begin Source File

SOURCE=.\physobj.cpp
# End Source File
# Begin Source File

SOURCE=.\physobj.h
# End Source File
# Begin Source File

SOURCE=..\Public\plane.h
# End Source File
# Begin Source File

SOURCE=.\plats.cpp
# End Source File
# Begin Source File

SOURCE=.\player.cpp
# End Source File
# Begin Source File

SOURCE=.\player.h
# End Source File
# Begin Source File

SOURCE=.\player_command.cpp
# End Source File
# Begin Source File

SOURCE=.\player_command.h
# End Source File
# Begin Source File

SOURCE=.\player_lagcompensation.cpp
# End Source File
# Begin Source File

SOURCE=.\player_resource.cpp
# End Source File
# Begin Source File

SOURCE=.\player_resource.h
# End Source File
# Begin Source File

SOURCE=.\playerlocaldata.cpp
# End Source File
# Begin Source File

SOURCE=.\playerlocaldata.h
# End Source File
# Begin Source File

SOURCE=.\point_playermoveconstraint.cpp
# End Source File
# Begin Source File

SOURCE=.\point_template.cpp
# End Source File
# Begin Source File

SOURCE=.\point_template.h
# End Source File
# Begin Source File

SOURCE=.\PointAngleSensor.cpp
# End Source File
# Begin Source File

SOURCE=.\PointAngularVelocitySensor.cpp
# End Source File
# Begin Source File

SOURCE=.\PointHurt.cpp
# End Source File
# Begin Source File

SOURCE=.\PointTeleport.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\precache_register.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\precache_register.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\predictableid.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\predictableid.h
# End Source File
# Begin Source File

SOURCE=.\props.cpp
# End Source File
# Begin Source File

SOURCE=.\props.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\ragdoll_shared.cpp
# End Source File
# Begin Source File

SOURCE=.\recipientfilter.cpp
# End Source File
# Begin Source File

SOURCE=.\recipientfilter.h
# End Source File
# Begin Source File

SOURCE=.\rope.cpp
# End Source File
# Begin Source File

SOURCE=.\rope.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\rope_helpers.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\rope_physics.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\rope_physics.h
# End Source File
# Begin Source File

SOURCE=..\Public\rope_shared.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\saverestore.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\saverestore.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\saverestore_bitstring.h
# End Source File
# Begin Source File

SOURCE=.\saverestore_gamedll.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\saverestore_utlvector.h
# End Source File
# Begin Source File

SOURCE=.\SceneEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\sceneentity.h
# End Source File
# Begin Source File

SOURCE=.\scripted.cpp
# End Source File
# Begin Source File

SOURCE=.\scripted.h
# End Source File
# Begin Source File

SOURCE=.\ScriptedTarget.cpp
# End Source File
# Begin Source File

SOURCE=.\ScriptedTarget.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\scriptevent.h
# End Source File
# Begin Source File

SOURCE=.\sendproxy.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\server_class.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\server_class.h
# End Source File
# Begin Source File

SOURCE=.\shadowcontrol.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\shattersurfacetypes.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\sheetsimulator.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\game_shared\sheetsimulator.h
# End Source File
# Begin Source File

SOURCE=..\Public\simple_physics.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\simple_physics.h
# End Source File
# Begin Source File

SOURCE=.\simtimer.cpp
# End Source File
# Begin Source File

SOURCE=.\simtimer.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\singleplay_gamerules.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\singleplay_gamerules.h
# End Source File
# Begin Source File

SOURCE=.\SkyCamera.cpp
# End Source File
# Begin Source File

SOURCE=.\sound.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\SoundEmitterSystem.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\SoundEmitterSystemBase.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\SoundEmitterSystemBase.h
# End Source File
# Begin Source File

SOURCE=.\soundent.cpp
# End Source File
# Begin Source File

SOURCE=.\soundent.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\soundenvelope.cpp
# End Source File
# Begin Source File

SOURCE=.\soundscape.h
# End Source File
# Begin Source File

SOURCE=.\splash.cpp
# End Source File
# Begin Source File

SOURCE=.\splash.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\Sprite.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\Sprite.h
# End Source File
# Begin Source File

SOURCE=..\Public\StringPool.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\StringRegistry.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\StringRegistry.h
# End Source File
# Begin Source File

SOURCE=..\Public\studio.h
# End Source File
# Begin Source File

SOURCE=.\subs.cpp
# End Source File
# Begin Source File

SOURCE=.\sun.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\takedamageinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\tanktrain.cpp
# End Source File
# Begin Source File

SOURCE=.\te_effect_dispatch.cpp
# End Source File
# Begin Source File

SOURCE=.\team.cpp
# End Source File
# Begin Source File

SOURCE=.\team.h
# End Source File
# Begin Source File

SOURCE=.\team_spawnpoint.cpp
# End Source File
# Begin Source File

SOURCE=.\team_spawnpoint.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\teamplay_gamerules.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\teamplay_gamerules.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\tempentity.h
# End Source File
# Begin Source File

SOURCE=.\TemplateEntities.cpp
# End Source File
# Begin Source File

SOURCE=.\TemplateEntities.h
# End Source File
# Begin Source File

SOURCE=.\tempmonster.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\terrainmod.h
# End Source File
# Begin Source File

SOURCE=.\terrainmodmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\terrainmodmgr.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\terrainmodmgr_shared.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\test_ehandle.cpp
# End Source File
# Begin Source File

SOURCE=.\test_proxytoggle.cpp
# End Source File
# Begin Source File

SOURCE=.\test_stressentities.cpp
# End Source File
# Begin Source File

SOURCE=.\testfunctions.cpp
# End Source File
# Begin Source File

SOURCE=.\TestTraceline.cpp
# End Source File
# Begin Source File

SOURCE=.\textstatsmgr.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\trains.h
# End Source File
# Begin Source File

SOURCE=.\triggers.cpp
# End Source File
# Begin Source File

SOURCE=.\triggers.h
# End Source File
# Begin Source File

SOURCE=..\public\UserCmd.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\util_shared.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\UtlBuffer.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\UtlBuffer.h
# End Source File
# Begin Source File

SOURCE=..\Public\utlsymbol.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\utlsymbol.h
# End Source File
# Begin Source File

SOURCE=.\variant_t.cpp
# End Source File
# Begin Source File

SOURCE=.\vguiscreen.cpp
# End Source File
# Begin Source File

SOURCE=.\vguiscreen.h
# End Source File
# Begin Source File

SOURCE=..\Public\vmatrix.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\vmatrix.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\voice_common.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\voice_gamemgr.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\voice_gamemgr.h
# End Source File
# Begin Source File

SOURCE=.\WCEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\WCEdit.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\weapon_parse.cpp
# End Source File
# Begin Source File

SOURCE=..\game_shared\weapon_parse.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\weapon_parse_default.cpp

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\world.cpp
# End Source File
# Begin Source File

SOURCE=.\world.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\Public\amd3dx.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\ammodef.h
# End Source File
# Begin Source File

SOURCE=.\base_transmit_proxy.h
# End Source File
# Begin Source File

SOURCE=..\public\basehandle.h
# End Source File
# Begin Source File

SOURCE=.\basenetworkable.h
# End Source File
# Begin Source File

SOURCE=.\basetempentity.h
# End Source File
# Begin Source File

SOURCE=..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\beam_flags.h
# End Source File
# Begin Source File

SOURCE=..\Public\bitbuf.h
# End Source File
# Begin Source File

SOURCE=..\Public\bitvec.h
# End Source File
# Begin Source File

SOURCE=.\tf2_dll\bot_base.h
# End Source File
# Begin Source File

SOURCE=..\Public\BSPFILE.H
# End Source File
# Begin Source File

SOURCE=..\Public\bspflags.h
# End Source File
# Begin Source File

SOURCE=..\Public\bumpvects.h
# End Source File
# Begin Source File

SOURCE=..\Public\characterset.h
# End Source File
# Begin Source File

SOURCE=..\Public\client_class.h
# End Source File
# Begin Source File

SOURCE=..\public\client_textmessage.h
# End Source File
# Begin Source File

SOURCE=..\Public\cmodel.h
# End Source File
# Begin Source File

SOURCE=..\public\Color.h
# End Source File
# Begin Source File

SOURCE=..\Public\commonmacros.h
# End Source File
# Begin Source File

SOURCE=..\Public\const.h
# End Source File
# Begin Source File

SOURCE=..\public\vphysics\constraints.h
# End Source File
# Begin Source File

SOURCE=..\Public\coordsize.h
# End Source File
# Begin Source File

SOURCE=..\Public\customentity.h
# End Source File
# Begin Source File

SOURCE=.\damagemodifier.h
# End Source File
# Begin Source File

SOURCE=..\Public\datamap.h
# End Source File
# Begin Source File

SOURCE=..\public\tier0\dbg.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\decals.h
# End Source File
# Begin Source File

SOURCE=..\Public\dlight.h
# End Source File
# Begin Source File

SOURCE=..\public\dt_common.h
# End Source File
# Begin Source File

SOURCE=..\public\dt_recv.h
# End Source File
# Begin Source File

SOURCE=..\public\dt_send.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\effect_dispatch_data.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\ehandle.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\entitylist_base.h
# End Source File
# Begin Source File

SOURCE=.\EventLog.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\ExpressionSample.h
# End Source File
# Begin Source File

SOURCE=..\public\tier0\fasttimer.h
# End Source File
# Begin Source File

SOURCE=..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\public\filesystem_helpers.h
# End Source File
# Begin Source File

SOURCE=..\public\fmtstr.h
# End Source File
# Begin Source File

SOURCE=.\fogcontroller.h
# End Source File
# Begin Source File

SOURCE=.\hl2_dll\Func_Monitor.h
# End Source File
# Begin Source File

SOURCE=..\public\gametrace.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=..\public\globalvars_base.h
# End Source File
# Begin Source File

SOURCE=..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\Public\icliententity.h
# End Source File
# Begin Source File

SOURCE=..\public\iclientnetworkable.h
# End Source File
# Begin Source File

SOURCE=..\Public\iclientrenderable.h
# End Source File
# Begin Source File

SOURCE=..\public\iclientunknown.h
# End Source File
# Begin Source File

SOURCE=..\public\engine\ICollideable.h
# End Source File
# Begin Source File

SOURCE=..\Public\icvar.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\IEffects.h
# End Source File
# Begin Source File

SOURCE=..\Public\engine\IEngineSound.h
# End Source File
# Begin Source File

SOURCE=..\public\engine\IEngineTrace.h
# End Source File
# Begin Source File

SOURCE=..\public\igameevents.h
# End Source File
# Begin Source File

SOURCE=..\Public\igamemovement.h
# End Source File
# Begin Source File

SOURCE=..\public\ihandleentity.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\IKeyValuesSystem.h
# End Source File
# Begin Source File

SOURCE=.\ilagcompensationmanager.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterial.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterialvar.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\imovehelper.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\in_buttons.h
# End Source File
# Begin Source File

SOURCE=..\Public\INetworkStringTableServer.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\ipredictionsystem.h
# End Source File
# Begin Source File

SOURCE=..\public\irecipientfilter.h
# End Source File
# Begin Source File

SOURCE=..\public\isaverestore.h
# End Source File
# Begin Source File

SOURCE=..\Public\iserverentity.h
# End Source File
# Begin Source File

SOURCE=..\public\iservernetworkable.h
# End Source File
# Begin Source File

SOURCE=..\Public\ISpatialPartition.h
# End Source File
# Begin Source File

SOURCE=..\public\engine\IStaticPropMgr.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\itempents.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\IVehicle.h
# End Source File
# Begin Source File

SOURCE=..\public\engine\ivmodelinfo.h
# End Source File
# Begin Source File

SOURCE=..\public\KeyValues.h
# End Source File
# Begin Source File

SOURCE=.\logicrelay.h
# End Source File
# Begin Source File

SOURCE=.\mapentities.h
# End Source File
# Begin Source File

SOURCE=..\public\tier0\mem.h
# End Source File
# Begin Source File

SOURCE=..\public\tier0\memalloc.h
# End Source File
# Begin Source File

SOURCE=..\public\tier0\memdbgoff.h
# End Source File
# Begin Source File

SOURCE=..\public\tier0\memdbgon.h
# End Source File
# Begin Source File

SOURCE=.\modelentities.h
# End Source File
# Begin Source File

SOURCE=.\tf2_dll\mortar_round.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\movevars_shared.h
# End Source File
# Begin Source File

SOURCE=.\netstatemgr.h
# End Source File
# Begin Source File

SOURCE=..\public\networkvar.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\npcevent.h
# End Source File
# Begin Source File

SOURCE=.\physics_cannister.h
# End Source File
# Begin Source File

SOURCE=.\physics_fx.h
# End Source File
# Begin Source File

SOURCE=.\physics_impact_damage.h
# End Source File
# Begin Source File

SOURCE=..\public\tier0\platform.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\playernet_vars.h
# End Source File
# Begin Source File

SOURCE=..\public\PlayerState.h
# End Source File
# Begin Source File

SOURCE=.\hl2_dll\Point_Camera.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\predictable_entity.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\predictioncopy.h
# End Source File
# Begin Source File

SOURCE=..\public\processor_detect.h
# End Source File
# Begin Source File

SOURCE=..\public\protected_things.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\random.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\rope_helpers.h
# End Source File
# Begin Source File

SOURCE=..\public\saverestoretypes.h
# End Source File
# Begin Source File

SOURCE=.\sendproxy.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\serial_entity.h
# End Source File
# Begin Source File

SOURCE=..\Public\shake.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\shared_classnames.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\shareddefs.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\sharedInterface.h
# End Source File
# Begin Source File

SOURCE=.\SkyCamera.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\soundenvelope.h
# End Source File
# Begin Source File

SOURCE=..\Public\soundflags.h
# End Source File
# Begin Source File

SOURCE=..\Public\string_t.h
# End Source File
# Begin Source File

SOURCE=..\Public\StringPool.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\takedamageinfo.h
# End Source File
# Begin Source File

SOURCE=.\te_effect_dispatch.h
# End Source File
# Begin Source File

SOURCE=.\test_stressentities.h
# End Source File
# Begin Source File

SOURCE=.\textstatsmgr.h
# End Source File
# Begin Source File

SOURCE=..\Public\usercmd.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\usermessages.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\util_shared.h
# End Source File
# Begin Source File

SOURCE=..\Public\utldict.h
# End Source File
# Begin Source File

SOURCE=..\Public\utllinkedlist.h
# End Source File
# Begin Source File

SOURCE=..\public\utlmap.h
# End Source File
# Begin Source File

SOURCE=..\Public\UtlMemory.h
# End Source File
# Begin Source File

SOURCE=..\Public\utlpriorityqueue.h
# End Source File
# Begin Source File

SOURCE=..\Public\utlrbtree.h
# End Source File
# Begin Source File

SOURCE=..\Public\UtlVector.h
# End Source File
# Begin Source File

SOURCE=..\Public\vallocator.h
# End Source File
# Begin Source File

SOURCE=.\variant_t.h
# End Source File
# Begin Source File

SOURCE=..\Public\vcollide.h
# End Source File
# Begin Source File

SOURCE=..\Public\vcollide_parse.h
# End Source File
# Begin Source File

SOURCE=..\Public\vector.h
# End Source File
# Begin Source File

SOURCE=..\Public\vector2d.h
# End Source File
# Begin Source File

SOURCE=..\Public\vector4d.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\tf2\vehicle_mortar_shared.h
# End Source File
# Begin Source File

SOURCE=..\public\vphysics\vehicles.h
# End Source File
# Begin Source File

SOURCE=..\Public\vphysics_interface.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\vphysics_sound.h
# End Source File
# Begin Source File

SOURCE=..\Public\vplane.h
# End Source File
# Begin Source File

SOURCE=..\public\tier0\vprof.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\vstdlib.h
# End Source File
# Begin Source File

SOURCE=.\winlite.h
# End Source File
# Begin Source File

SOURCE=..\Public\worldsize.h
# End Source File
# End Group
# Begin Group "Data files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\bin\base.fgd

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\bin\halflife1.fgd

!IF  "$(CFG)" == "hl - Win32 Release HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\bin\halflife2.fgd

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\bin\tf2.fgd

!IF  "$(CFG)" == "hl - Win32 Release HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release TF2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug HL1"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Debug CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "hl - Win32 Release CounterStrike"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

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
