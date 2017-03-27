# Microsoft Developer Studio Project File - Name="Status" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Status - Win32 Profile
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Status.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Status.mak" CFG="Status - Win32 Profile"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Status - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Status - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Status - Win32 Profile" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Goldsrc/status", GSBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Status - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\common" /I "..\engine" /I "..\public" /I "..\utils\common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "LAUNCHERONLY" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386 /libpath:"..\lib\public"
# Begin Custom Build
TargetPath=.\Release\Status.exe
InputPath=.\Release\Status.exe
SOURCE="$(InputPath)"

"..\..\bin\status.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetPath) ..\..\bin

# End Custom Build

!ELSEIF  "$(CFG)" == "Status - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\common" /I "..\engine" /I "..\public" /I "..\utils\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "LAUNCHERONLY" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 winmm.lib /nologo /subsystem:windows /map /debug /machine:I386 /libpath:"..\lib\public"
# SUBTRACT LINK32 /profile
# Begin Custom Build
TargetPath=.\Debug\Status.exe
InputPath=.\Debug\Status.exe
SOURCE="$(InputPath)"

"..\..\bin\status.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetPath) ..\..\bin

# End Custom Build

!ELSEIF  "$(CFG)" == "Status - Win32 Profile"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Status___Win32_Profile"
# PROP BASE Intermediate_Dir "Status___Win32_Profile"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Status___Win32_Profile"
# PROP Intermediate_Dir "Status___Win32_Profile"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\common" /I "..\engine" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "LAUNCHERONLY" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\common" /I "..\engine" /I "..\public" /I "..\utils\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "LAUNCHERONLY" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib /nologo /subsystem:windows /profile /map /debug /machine:I386
# ADD LINK32 winmm.lib /nologo /subsystem:windows /profile /map /debug /machine:I386 /libpath:"..\lib\public"
# Begin Custom Build
TargetPath=.\Status___Win32_Profile\Status.exe
InputPath=.\Status___Win32_Profile\Status.exe
SOURCE="$(InputPath)"

"..\..\bin\status.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetPath) ..\..\bin

# End Custom Build

!ENDIF 

# Begin Target

# Name "Status - Win32 Release"
# Name "Status - Win32 Debug"
# Name "Status - Win32 Profile"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\AddEmailDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\AddEmailDialog.h
# End Source File
# Begin Source File

SOURCE=.\BaseDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseDlg.h
# End Source File
# Begin Source File

SOURCE=..\public\characterset.cpp
# End Source File
# Begin Source File

SOURCE=..\utils\common\cmdlib.cpp
# End Source File
# Begin Source File

SOURCE=..\utils\common\cmdlib.h
# End Source File
# Begin Source File

SOURCE=.\EmailDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\EmailDialog.h
# End Source File
# Begin Source File

SOURCE=.\EmailManager.cpp
# End Source File
# Begin Source File

SOURCE=..\public\filesystem.h
# End Source File
# Begin Source File

SOURCE=..\public\filesystem_helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\public\filesystem_tools.cpp
# End Source File
# Begin Source File

SOURCE=..\public\filesystem_tools.h
# End Source File
# Begin Source File

SOURCE=.\Graph.cpp
# End Source File
# Begin Source File

SOURCE=.\Graph.h
# End Source File
# Begin Source File

SOURCE=.\HLAsyncSocket.h
# End Source File
# Begin Source File

SOURCE=..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\public\interface.h
# End Source File
# Begin Source File

SOURCE=..\public\KeyValues.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadDialog.h
# End Source File
# Begin Source File

SOURCE=..\public\mempool.cpp
# End Source File
# Begin Source File

SOURCE=.\MessageBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\MessageBuffer.h
# End Source File
# Begin Source File

SOURCE=.\mod.cpp
# End Source File
# Begin Source File

SOURCE=.\mod.h
# End Source File
# Begin Source File

SOURCE=.\ModInfoSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\ModInfoSocket.h
# End Source File
# Begin Source File

SOURCE=.\modupdate.h
# End Source File
# Begin Source File

SOURCE=.\ODStatic.cpp
# End Source File
# Begin Source File

SOURCE=.\ODStatic.h
# End Source File
# Begin Source File

SOURCE=.\OptionsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsDlg.h
# End Source File
# Begin Source File

SOURCE=.\ProjectionWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\ProjectionWnd.h
# End Source File
# Begin Source File

SOURCE=..\common\proto_oob.h
# End Source File
# Begin Source File

SOURCE=..\common\proto_version.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Scheduler.h
# End Source File
# Begin Source File

SOURCE=.\SchedulerDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Status.cpp
# End Source File
# Begin Source File

SOURCE=.\Status.h
# End Source File
# Begin Source File

SOURCE=.\Status.rc
# End Source File
# Begin Source File

SOURCE=.\status_colors.h
# End Source File
# Begin Source File

SOURCE=.\status_protocol.h
# End Source File
# Begin Source File

SOURCE=.\StatusDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StatusDlg.h
# End Source File
# Begin Source File

SOURCE=.\StatusWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\StatusWindow.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=..\public\utlbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\public\utlbuffer.h
# End Source File
# Begin Source File

SOURCE=..\public\utlmemory.h
# End Source File
# Begin Source File

SOURCE=..\public\utlrbtree.h
# End Source File
# Begin Source File

SOURCE=..\public\utlsymbol.cpp
# End Source File
# Begin Source File

SOURCE=..\public\utlsymbol.h
# End Source File
# Begin Source File

SOURCE=..\public\utlvector.h
# End Source File
# Begin Source File

SOURCE=.\WarnDialog.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\public\characterset.h
# End Source File
# Begin Source File

SOURCE=..\public\Color.h
# End Source File
# Begin Source File

SOURCE=..\public\commonmacros.h
# End Source File
# Begin Source File

SOURCE=.\EmailManager.h
# End Source File
# Begin Source File

SOURCE=..\public\filesystem_helpers.h
# End Source File
# Begin Source File

SOURCE=..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\public\KeyValues.h
# End Source File
# Begin Source File

SOURCE=..\public\mempool.h
# End Source File
# Begin Source File

SOURCE=..\public\protected_things.h
# End Source File
# Begin Source File

SOURCE=.\SchedulerDialog.h
# End Source File
# Begin Source File

SOURCE=..\public\string_t.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=..\public\utllinkedlist.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\vstdlib.h
# End Source File
# Begin Source File

SOURCE=.\WarnDialog.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Status.ico
# End Source File
# Begin Source File

SOURCE=.\res\Status.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\todo.txt
# End Source File
# Begin Source File

SOURCE=..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
