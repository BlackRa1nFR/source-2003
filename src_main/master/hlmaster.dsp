# Microsoft Developer Studio Project File - Name="HLMaster" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=HLMaster - Win32 Debug Steam Master
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HLMaster.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HLMaster.mak" CFG="HLMaster - Win32 Debug Steam Master"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HLMaster - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "HLMaster - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "HLMaster - Win32 Release Steam Master" (based on "Win32 (x86) Application")
!MESSAGE "HLMaster - Win32 Debug Steam Master" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/master", IHJBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "HLMaster - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\common" /I "..\engine" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 wsock32.lib /nologo /subsystem:windows /machine:I386
# Begin Custom Build
ProjDir=.
InputPath=.\Release\HLMaster.exe
SOURCE="$(InputPath)"

"$(ProjDir)\..\..\hlmaster.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	call ..\filecopy.bat $(InputPath) $(ProjDir)\..\..\hlmaster.exe

# End Custom Build

!ELSEIF  "$(CFG)" == "HLMaster - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\common" /I "..\engine" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 wsock32.lib /nologo /subsystem:windows /debug /machine:I386
# Begin Custom Build
ProjDir=.
InputPath=.\Debug\HLMaster.exe
InputName=HLMaster
SOURCE="$(InputPath)"

"$(ProjDir)\..\..\$(InputName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	call ..\filecopy.bat $(InputPath) $(ProjDir)\..\..\bin\$(InputName).exe

# End Custom Build

!ELSEIF  "$(CFG)" == "HLMaster - Win32 Release Steam Master"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "HLMaster___Win32_Release_Steam_Master"
# PROP BASE Intermediate_Dir "HLMaster___Win32_Release_Steam_Master"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_Steam_Master"
# PROP Intermediate_Dir "Release_Steam_Master"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\common" /I "..\engine" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\common" /I "..\engine" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_STEAM_HLMASTER" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wsock32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 wsock32.lib /nologo /subsystem:windows /machine:I386
# Begin Custom Build
ProjDir=.
InputPath=.\Release_Steam_Master\HLMaster.exe
SOURCE="$(InputPath)"

"$(ProjDir)\..\..\hlmaster.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	call ..\filecopy.bat $(InputPath) $(ProjDir)\..\..\bin\hlmaster.exe

# End Custom Build

!ELSEIF  "$(CFG)" == "HLMaster - Win32 Debug Steam Master"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "HLMaster___Win32_Debug_Steam_Master"
# PROP BASE Intermediate_Dir "HLMaster___Win32_Debug_Steam_Master"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_Steam_Master"
# PROP Intermediate_Dir "Debug_Steam_Master"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\common" /I "..\engine" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\common" /I "..\engine" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_STEAM_HLMASTER" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wsock32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 wsock32.lib /nologo /subsystem:windows /debug /machine:I386
# Begin Custom Build
ProjDir=.
InputPath=.\Debug_Steam_Master\HLMaster.exe
InputName=HLMaster
SOURCE="$(InputPath)"

"$(ProjDir)\..\..\$(InputName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	call ..\filecopy.bat $(InputPath) $(ProjDir)\..\..\bin\$(InputName).exe

# End Custom Build

!ENDIF 

# Begin Target

# Name "HLMaster - Win32 Release"
# Name "HLMaster - Win32 Debug"
# Name "HLMaster - Win32 Release Steam Master"
# Name "HLMaster - Win32 Debug Steam Master"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\crc.cpp
# End Source File
# Begin Source File

SOURCE=.\HLMaster.cpp
# End Source File
# Begin Source File

SOURCE=.\HLMaster.h
# End Source File
# Begin Source File

SOURCE=.\HLMaster.rc
# End Source File
# Begin Source File

SOURCE=.\HLMasterDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\HLMasterDlg.h
# End Source File
# Begin Source File

SOURCE=.\HLMasterProtocol.h
# End Source File
# Begin Source File

SOURCE=.\info.cpp
# End Source File
# Begin Source File

SOURCE=..\common\proto_version.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Token.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\CRC.H
# End Source File
# Begin Source File

SOURCE=.\info.h
# End Source File
# Begin Source File

SOURCE=..\common\proto_oob.h
# End Source File
# Begin Source File

SOURCE=.\Token.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\HLMaster.ico
# End Source File
# Begin Source File

SOURCE=.\res\HLMaster.rc2
# End Source File
# End Group
# End Target
# End Project
