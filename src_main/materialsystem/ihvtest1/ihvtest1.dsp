# Microsoft Developer Studio Project File - Name="ihvtest1" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ihvtest1 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ihvtest1.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ihvtest1.mak" CFG="ihvtest1 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ihvtest1 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ihvtest1 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/common/MaterialSystem/ihvtest1", PKXDAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ihvtest1 - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /GX /Zi /O2 /Ob2 /I "..\..\common" /I "..\..\public" /I "..\..\game_shared" /D "NDEBUG" /D "IHVTEST" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386
# Begin Custom Build
TargetPath=.\Release\ihvtest1.exe
TargetName=ihvtest1
InputPath=.\Release\ihvtest1.exe
SOURCE="$(InputPath)"

"..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\$(TargetName).exe attrib -r ..\..\..\bin\$(TargetName).exe 
	if exist $(TargetPath) copy $(TargetPath)  ..\..\..\bin 
	call copysrc.bat 
	call copybin.bat 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ihvtest1 - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\common" /I "..\..\public" /I "..\..\game_shared" /D "_DEBUG" /D "IHVTEST" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# Begin Custom Build
TargetPath=.\Debug\ihvtest1.exe
TargetName=ihvtest1
InputPath=.\Debug\ihvtest1.exe
SOURCE="$(InputPath)"

"..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\$(TargetName).exe attrib -r ..\..\..\bin\$(TargetName).exe 
	if exist $(TargetPath) copy $(TargetPath)  ..\..\..\bin 
	call copysrc.bat 
	call copybin.bat 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "ihvtest1 - Win32 Release"
# Name "ihvtest1 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\game_shared\bone_setup.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\collisionutils.cpp
# End Source File
# Begin Source File

SOURCE=.\ihvtest1.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=.\sys_clock.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "build bat files"

# PROP Default_Filter "*.bat"
# Begin Source File

SOURCE=.\copybin.bat
# End Source File
# Begin Source File

SOURCE=.\copycommonsrc.bat
# End Source File
# Begin Source File

SOURCE=.\copydx8.bat
# End Source File
# Begin Source File

SOURCE=.\copygamesharedsrc.bat
# End Source File
# Begin Source File

SOURCE=.\copyihvtestsrc.bat
# End Source File
# Begin Source File

SOURCE=.\copylib.bat
# End Source File
# Begin Source File

SOURCE=.\copymatsyssrc.bat
# End Source File
# Begin Source File

SOURCE=.\copypublicsrc.bat
# End Source File
# Begin Source File

SOURCE=.\copyshaderdx8src.bat
# End Source File
# Begin Source File

SOURCE=.\copysrc.bat
# End Source File
# Begin Source File

SOURCE=.\copystudiorendersrc.bat
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\common\vtuneapi.lib
# End Source File
# End Target
# End Project
