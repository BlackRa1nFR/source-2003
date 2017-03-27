# Microsoft Developer Studio Project File - Name="MasterTest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=MasterTest - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MasterTest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MasterTest.mak" CFG="MasterTest - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MasterTest - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MasterTest - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/master/hlmodmaster", RYHBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MasterTest - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\MasterTest\Release"
# PROP BASE Intermediate_Dir ".\MasterTest\Release"
# PROP BASE Target_Dir ".\MasterTest"
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\MasterTest\Release"
# PROP Intermediate_Dir ".\MasterTest\Release"
# PROP Target_Dir ".\MasterTest"
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\engine" /I "..\..\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "MasterTest - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\MasterTest\Debug"
# PROP BASE Intermediate_Dir ".\MasterTest\Debug"
# PROP BASE Target_Dir ".\MasterTest"
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\MasterTest\Debug"
# PROP Intermediate_Dir ".\MasterTest\Debug"
# PROP Target_Dir ".\MasterTest"
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\engine" /I "..\..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386
# Begin Custom Build
TargetPath=.\MasterTest\Debug\MasterTest.exe
InputPath=.\MasterTest\Debug\MasterTest.exe
SOURCE="$(InputPath)"

"cms.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	attrib -r d:\quiver\cms.exe 
	copy "$(TargetPath)" d:\quiver\cms.exe 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "MasterTest - Win32 Release"
# Name "MasterTest - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\MasterTest\HLMasterAsyncSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\MasterTest\MasterTest.cpp
# End Source File
# Begin Source File

SOURCE=.\MasterTest\MasterTest.rc
# ADD BASE RSC /l 0x409 /i "MasterTest"
# ADD RSC /l 0x409 /i "MasterTest" /i ".\MasterTest"
# End Source File
# Begin Source File

SOURCE=.\MasterTest\MasterTestDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\MasterTest\MessageBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\MasterTest\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\MasterTest\UTIL.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
