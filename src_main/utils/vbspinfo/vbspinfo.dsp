# Microsoft Developer Studio Project File - Name="vbspinfo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=vbspinfo - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vbspinfo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vbspinfo.mak" CFG="vbspinfo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vbspinfo - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "vbspinfo - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/utils/vbspinfo", WEVBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vbspinfo - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /O2 /I "..\..\public" /I "..\common" /I "..\..\common" /D "NDEBUG" /D "_WIN32" /D "_CONSOLE" /D "_MBCS" /D "DONT_PROTECT_FILEIO_FUNCTIONS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 /nologo /subsystem:console /map /debug /machine:I386 /libpath:"..\..\lib\common" /libpath:"..\..\lib\public"
# Begin Custom Build - Copying
TargetDir=.\Release
TargetPath=.\Release\vbspinfo.exe
TargetName=vbspinfo
InputPath=.\Release\vbspinfo.exe
SOURCE="$(InputPath)"

"..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\$(TargetName).exe attrib -r ..\..\..\bin\$(TargetName).exe 
	copy $(TargetPath) ..\..\..\bin 
	if exist $(TargetDir)\$(TargetName).map copy $(TargetDir)\$(TargetName).map ..\..\..\bin\$(TargetName).map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vbspinfo - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /I "..\..\public" /I "..\common" /I "..\..\common" /D "_DEBUG" /D "_WIN32" /D "_CONSOLE" /D "_MBCS" /D "DONT_PROTECT_FILEIO_FUNCTIONS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common" /libpath:"..\..\lib\public"
# Begin Custom Build - Copying
TargetDir=.\Debug
TargetPath=.\Debug\vbspinfo.exe
TargetName=vbspinfo
InputPath=.\Debug\vbspinfo.exe
SOURCE="$(InputPath)"

"..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\$(TargetName).exe attrib -r ..\..\..\bin\$(TargetName).exe 
	copy $(TargetPath) ..\..\..\bin 
	if exist $(TargetDir)\$(TargetName).map copy $(TargetDir)\$(TargetName).map ..\..\..\bin\$(TargetName).map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "vbspinfo - Win32 Release"
# Name "vbspinfo - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\common\bsplib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\characterset.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\checksum_crc.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_tools.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\Mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\common\scriplib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\utlsymbol.cpp
# End Source File
# Begin Source File

SOURCE=.\vbspinfo.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\public\characterset.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\checksum_crc.h
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_helpers.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
