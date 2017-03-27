# Microsoft Developer Studio Project File - Name="playback" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=playback - Win32 Debug DX9
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "playback.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "playback.mak" CFG="playback - Win32 Debug DX9"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "playback - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "playback - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "playback - Win32 Debug DX9" (based on "Win32 (x86) Application")
!MESSAGE "playback - Win32 Release DX9" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/common/MaterialSystem/shaderdx8/playback", DRODAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "playback - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /Zi /O2 /I "..\..\..\public" /I "..\\" /I "..\..\..\dx8sdk\include" /D "NDEBUG" /D "TGAWRITER_USE_FOPEN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /libpath:"..\..\..\..\dx8sdk\lib"
# Begin Custom Build
TargetPath=.\Release\playback.exe
InputPath=.\Release\playback.exe
SOURCE="$(InputPath)"

"..\..\..\..\bin\playback.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\..\bin\playback.exe attrib -r ..\..\..\..\bin\playback.exe 
	if exist "$(TargetPath)" copy "$(TargetPath)" ..\..\..\..\bin 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "playback - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\public" /I "..\\" /I "..\..\..\dx8sdk\include" /D "_DEBUG" /D "TGAWRITER_USE_FOPEN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\dx8sdk\lib"
# Begin Custom Build
TargetPath=.\Debug\playback.exe
InputPath=.\Debug\playback.exe
SOURCE="$(InputPath)"

"..\..\..\..\bin\playback.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\..\bin\playback.exe attrib -r ..\..\..\..\bin\playback.exe 
	if exist "$(TargetPath)" copy "$(TargetPath)" ..\..\..\..\bin 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "playback - Win32 Debug DX9"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "playback___Win32_Debug_DX9"
# PROP BASE Intermediate_Dir "playback___Win32_Debug_DX9"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "playback___Win32_Debug_DX9"
# PROP Intermediate_Dir "playback___Win32_Debug_DX9"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\public" /I "..\\" /I "..\..\..\dx8sdk\include" /D "_DEBUG" /D "TGAWRITER_USE_FOPEN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\public" /I "..\\" /I "..\..\..\dx9sdk\include" /D "_DEBUG" /D "SHADERAPIDX9" /D "TGAWRITER_USE_FOPEN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\dx8sdk\lib"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\dx8sdk\lib"
# Begin Custom Build
TargetPath=.\playback___Win32_Debug_DX9\playback.exe
InputPath=.\playback___Win32_Debug_DX9\playback.exe
SOURCE="$(InputPath)"

"..\..\..\..\bin\playback.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\..\bin\playback.exe attrib -r ..\..\..\..\bin\playback.exe 
	if exist "$(TargetPath)" copy "$(TargetPath)" ..\..\..\..\bin 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "playback - Win32 Release DX9"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "playback___Win32_Release_DX9"
# PROP BASE Intermediate_Dir "playback___Win32_Release_DX9"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "playback___Win32_Release_DX9"
# PROP Intermediate_Dir "playback___Win32_Release_DX9"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Zi /O2 /I "..\..\..\public" /I "..\\" /I "..\..\..\dx8sdk\include" /D "NDEBUG" /D "TGAWRITER_USE_FOPEN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /FR /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /I "..\..\..\public" /I "..\\" /I "..\..\..\dx9sdk\include" /D "NDEBUG" /D "SHADERAPIDX9" /D "TGAWRITER_USE_FOPEN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /libpath:"..\..\..\..\dx8sdk\lib"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /libpath:"..\..\..\..\dx8sdk\lib"
# Begin Custom Build
TargetPath=.\playback___Win32_Release_DX9\playback.exe
InputPath=.\playback___Win32_Release_DX9\playback.exe
SOURCE="$(InputPath)"

"..\..\..\..\bin\playback.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\..\bin\playback.exe attrib -r ..\..\..\..\bin\playback.exe 
	if exist "$(TargetPath)" copy "$(TargetPath)" ..\..\..\..\bin 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "playback - Win32 Release"
# Name "playback - Win32 Debug"
# Name "playback - Win32 Debug DX9"
# Name "playback - Win32 Release DX9"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\Public\imageloader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=.\playback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\public\tgawriter.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Source File

SOURCE=..\..\..\lib\common\s3tc.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\DX8SDK\lib\d3d8.lib

!IF  "$(CFG)" == "playback - Win32 Release"

!ELSEIF  "$(CFG)" == "playback - Win32 Debug"

!ELSEIF  "$(CFG)" == "playback - Win32 Debug DX9"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "playback - Win32 Release DX9"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\DX8SDK\lib\d3dx8.lib

!IF  "$(CFG)" == "playback - Win32 Release"

!ELSEIF  "$(CFG)" == "playback - Win32 Debug"

!ELSEIF  "$(CFG)" == "playback - Win32 Debug DX9"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "playback - Win32 Release DX9"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\dx9sdk\lib\d3d9.lib

!IF  "$(CFG)" == "playback - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "playback - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "playback - Win32 Debug DX9"

!ELSEIF  "$(CFG)" == "playback - Win32 Release DX9"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\dx9sdk\lib\d3dx9.lib

!IF  "$(CFG)" == "playback - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "playback - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "playback - Win32 Debug DX9"

!ELSEIF  "$(CFG)" == "playback - Win32 Release DX9"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\lib\public\tier0.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\public\vstdlib.lib
# End Source File
# End Target
# End Project
