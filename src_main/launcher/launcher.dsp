# Microsoft Developer Studio Project File - Name="launcher" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=launcher - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "launcher.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "launcher.mak" CFG="launcher - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "launcher - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "launcher - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/launcher", SCQCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "launcher - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GR /GX /Zi /O2 /I "..\dxsdk\include" /I "..\common" /I "..\public" /I "..\vgui2\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "LAUNCHERONLY" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\dx8sdk\lib\dinput8.lib ..\dx8sdk\lib\ddraw.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /libpath:"..\lib\public" /libpath:"..\lib\common"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetPath=.\Release\launcher.dll
InputPath=.\Release\launcher.dll
SOURCE="$(InputPath)"

"..\..\bin\launcher.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	attrib -r ..\..\bin\launcher.dll 
	copy "$(TargetPath)" ..\..\bin\launcher.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "launcher - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /I "..\dxsdk\include" /I "..\common" /I "..\public" /I "..\vgui2\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "LAUNCHERONLY" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\dx8sdk\lib\dinput8.lib ..\dx8sdk\lib\ddraw.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib:"LIBC" /pdbtype:sept /libpath:"..\lib\public" /libpath:"..\lib\common"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetPath=.\Debug\launcher.dll
InputPath=.\Debug\launcher.dll
SOURCE="$(InputPath)"

"..\..\bin\launcher.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	attrib -r ..\..\bin\launcher.dll 
	copy "$(TargetPath)" ..\..\bin\launcher.dll 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "launcher - Win32 Release"
# Name "launcher - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\filesystem.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\interface.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\launcher.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=..\public\registry.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\Public\dbg\dbg.h
# End Source File
# Begin Source File

SOURCE=..\Public\engine_launcher_api.h
# End Source File
# Begin Source File

SOURCE=..\Public\platform\fasttimer.h
# End Source File
# Begin Source File

SOURCE=..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\ICommandLine.h
# End Source File
# Begin Source File

SOURCE=.\ifilesystem.h
# End Source File
# Begin Source File

SOURCE=..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=..\public\iregistry.h
# End Source File
# Begin Source File

SOURCE=..\public\vgui\KeyCode.h
# End Source File
# Begin Source File

SOURCE=..\public\vgui\MouseCode.h
# End Source File
# Begin Source File

SOURCE=..\Public\platform\platform.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=..\public\platform\vcr_shared.h
# End Source File
# Begin Source File

SOURCE=..\public\platform\vcrmode.h
# End Source File
# Begin Source File

SOURCE=..\Public\vector2d.h
# End Source File
# Begin Source File

SOURCE=..\public\vgui\VGUI.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\vstdlib.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
