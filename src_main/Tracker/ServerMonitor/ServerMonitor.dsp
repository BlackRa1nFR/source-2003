# Microsoft Developer Studio Project File - Name="ServerMonitor" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ServerMonitor - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ServerMonitor.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ServerMonitor.mak" CFG="ServerMonitor - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ServerMonitor - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ServerMonitor - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GoldSrc/Tracker/ServerMonitor", PAWDAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ServerMonitor - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ServerMonitor___Win32_Release"
# PROP BASE Intermediate_Dir "ServerMonitor___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ServerMonitor___Win32_Release"
# PROP Intermediate_Dir "ServerMonitor___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\public" /I "..\..\common" /I "..\common" /I "..\..\vgui2\controls" /I "..\..\vgui2\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ..\..\vgui2\bin\vgui_controls.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib WS2_32.LIB /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc.lib" /out:"Win32_Release/ServerMonitor.exe"
# Begin Custom Build
TargetPath=.\Win32_Release\ServerMonitor.exe
InputPath=.\Win32_Release\ServerMonitor.exe
SOURCE="$(InputPath)"

"..\..\..\platform\bin\ServerMonitor.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\platform\bin\ServerMonitor.exe attrib -r ..\..\..\platform\bin\ServerMonitor.exe 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\platform\bin\ServerMonitor.exe 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ServerMonitor - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ServerMonitor___Win32_Debug"
# PROP BASE Intermediate_Dir "ServerMonitor___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32_Debug"
# PROP Intermediate_Dir "Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\public" /I "..\..\common" /I "..\common" /I "..\..\vgui2\controls" /I "..\..\vgui2\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\vgui2\bin\vgui_controls.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib WS2_32.LIB /nologo /subsystem:windows /map /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept
# Begin Custom Build
TargetPath=.\Win32_Debug\ServerMonitor.exe
InputPath=.\Win32_Debug\ServerMonitor.exe
SOURCE="$(InputPath)"

"..\..\..\platform\bin\ServerMonitor.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\platform\bin\ServerMonitor.exe attrib -r ..\..\..\platform\bin\ServerMonitor.exe 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\platform\bin\ServerMonitor.exe 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "ServerMonitor - Win32 Release"
# Name "ServerMonitor - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\Main.cpp
# End Source File
# Begin Source File

SOURCE=.\MainDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\TrackerNET\NetAddress.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerInfoPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerList.cpp
# End Source File
# Begin Source File

SOURCE=.\ServersPage.cpp
# End Source File
# Begin Source File

SOURCE=.\UsersPage.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\public\interface.h
# End Source File
# Begin Source File

SOURCE=.\MainDialog.h
# End Source File
# Begin Source File

SOURCE=..\TrackerNET\NetAddress.h
# End Source File
# Begin Source File

SOURCE=.\ServerInfoPanel.h
# End Source File
# Begin Source File

SOURCE=.\ServerList.h
# End Source File
# Begin Source File

SOURCE=.\ServersPage.h
# End Source File
# Begin Source File

SOURCE=.\UsersPage.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\..\vgui2\bin\vgui_controls.lib
# End Source File
# End Target
# End Project
