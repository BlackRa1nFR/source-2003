# Microsoft Developer Studio Project File - Name="vmpi_browser_job_watch" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=vmpi_browser_job_watch - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vmpi_browser_job_watch.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vmpi_browser_job_watch.mak" CFG="vmpi_browser_job_watch - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vmpi_browser_job_watch - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vmpi_browser_job_watch - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "vmpi_browser_job_watch"
# PROP Scc_LocalPath "..\..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vmpi_browser_job_watch - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /I "..\..\..\public" /I "..\..\common" /I "..\mysql\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 nafxcw.lib libcmt.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"nafxcw.lib libcmt.lib"
# Begin Custom Build
TargetPath=.\Release\vmpi_browser_job_watch.exe
InputPath=.\Release\vmpi_browser_job_watch.exe
SOURCE="$(InputPath)"

"..\..\..\..\bin\vmpi_browser_job_watch.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetPath) ..\..\..\..\bin

# End Custom Build

!ELSEIF  "$(CFG)" == "vmpi_browser_job_watch - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\..\public" /I "..\..\common" /I "..\mysql\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 nafxcwd.lib libcmtd.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"nafxcwd.lib libcmtd.lib" /pdbtype:sept
# Begin Custom Build
TargetPath=.\Debug\vmpi_browser_job_watch.exe
InputPath=.\Debug\vmpi_browser_job_watch.exe
SOURCE="$(InputPath)"

"..\..\..\..\bin\vmpi_browser_job_watch.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetPath) ..\..\..\..\bin

# End Custom Build

!ENDIF 

# Begin Target

# Name "vmpi_browser_job_watch - Win32 Release"
# Name "vmpi_browser_job_watch - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\common\consolewnd.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\GraphControl.cpp
# End Source File
# Begin Source File

SOURCE=..\idle_dialog.cpp
# End Source File
# Begin Source File

SOURCE=.\JobWatchDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\mysql_wrapper.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\vmpi_browser_job_watch.cpp
# End Source File
# Begin Source File

SOURCE=.\vmpi_browser_job_watch.rc
# End Source File
# Begin Source File

SOURCE=..\win_idle.cpp
# End Source File
# Begin Source File

SOURCE=..\window_anchor_mgr.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\common\consolewnd.h
# End Source File
# Begin Source File

SOURCE=.\GraphControl.h
# End Source File
# Begin Source File

SOURCE=..\idle_dialog.h
# End Source File
# Begin Source File

SOURCE=.\JobWatchDlg.h
# End Source File
# Begin Source File

SOURCE=..\mysql_wrapper.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\vmpi_browser_job_watch.h
# End Source File
# Begin Source File

SOURCE=..\win_idle.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\vmpi_browser_job_watch.ico
# End Source File
# Begin Source File

SOURCE=.\res\vmpi_browser_job_watch.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=..\mysql\lib\opt\libmySQL.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
