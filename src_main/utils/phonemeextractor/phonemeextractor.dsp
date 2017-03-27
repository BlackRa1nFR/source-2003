# Microsoft Developer Studio Project File - Name="phonemeextractor" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=phonemeextractor - Win32 Debug IMS
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "phonemeextractor.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "phonemeextractor.mak" CFG="phonemeextractor - Win32 Debug IMS"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "phonemeextractor - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "phonemeextractor - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "phonemeextractor - Win32 Release IMS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "phonemeextractor - Win32 Debug IMS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "phonemeextractor"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "phonemeextractor - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PHONEMEEXTRACTOR_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../common" /I "../hlfaceposer" /I "../../public" /I "../sapi51/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PHONEMEEXTRACTOR_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 talkback_rd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /nodefaultlib:"MSVCRT"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetDir=.\Release
TargetPath=.\Release\phonemeextractor.dll
TargetName=phonemeextractor
InputPath=.\Release\phonemeextractor.dll
SOURCE="$(InputPath)"

"..\..\..\bin\phonemeextractors\$(TargetName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\phonemeextractors\$(TargetName).dll attrib -r ..\..\..\bin\phonemeextractors\$(TargetName).dll 
	copy $(TargetPath) ..\..\..\bin\phonemeextractors 
	if exist $(TargetDir)\$(TargetName).map copy $(TargetDir)\$(TargetName).map ..\..\..\bin\phonemeextractors\$(TargetName).map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "phonemeextractor - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PHONEMEEXTRACTOR_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../common" /I "../hlfaceposer" /I "../../public" /I "../sapi51/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PHONEMEEXTRACTOR_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 talkback_dd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"MSVCRT" /pdbtype:sept
# Begin Custom Build
TargetDir=.\Debug
TargetPath=.\Debug\phonemeextractor.dll
TargetName=phonemeextractor
InputPath=.\Debug\phonemeextractor.dll
SOURCE="$(InputPath)"

"..\..\..\bin\phonemeextractors\$(TargetName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\phonemeextractors\$(TargetName).dll attrib -r ..\..\..\bin\phonemeextractors\$(TargetName).dll 
	copy $(TargetPath) ..\..\..\bin\phonemeextractors 
	if exist $(TargetDir)\$(TargetName).map copy $(TargetDir)\$(TargetName).map ..\..\..\bin\phonemeextractors\$(TargetName).map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "phonemeextractor - Win32 Release IMS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseIMS"
# PROP BASE Intermediate_Dir "ReleaseIMS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseIMS"
# PROP Intermediate_Dir "ReleaseIMS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "../common" /I "../hlfaceposer" /I "../../public" /I "../sapi51/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PHONEMEEXTRACTOR_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../common" /I "../hlfaceposer" /I "../../public" /I "../sapi51/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PHONEMEEXTRACTOR_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 talkback_rd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /nodefaultlib:"LIBCMT"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 talkback_rs.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /nodefaultlib:"MSVCRT" /out:"ReleaseIMS/phonemeextractor_ims.dll"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetDir=.\ReleaseIMS
TargetPath=.\ReleaseIMS\phonemeextractor_ims.dll
TargetName=phonemeextractor_ims
InputPath=.\ReleaseIMS\phonemeextractor_ims.dll
SOURCE="$(InputPath)"

"..\..\..\bin\phonemeextractors\$(TargetName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\phonemeextractors\$(TargetName).dll attrib -r ..\..\..\bin\phonemeextractors\$(TargetName).dll 
	copy $(TargetPath) ..\..\..\bin\phonemeextractors 
	if exist $(TargetDir)\$(TargetName).map copy $(TargetDir)\$(TargetName).map ..\..\..\bin\phonemeextractors\$(TargetName).map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "phonemeextractor - Win32 Debug IMS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugIMS"
# PROP BASE Intermediate_Dir "DebugIMS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugIMS"
# PROP Intermediate_Dir "DebugIMS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../common" /I "../hlfaceposer" /I "../../public" /I "../sapi51/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PHONEMEEXTRACTOR_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../common" /I "../hlfaceposer" /I "../../public" /I "../sapi51/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PHONEMEEXTRACTOR_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 talkback_dd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"LIBCMTD" /nodefaultlib:"MSVCRT" /pdbtype:sept
# ADD LINK32 talkback_ds.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"MSVCRT" /out:"DebugIMS/phonemeextractor_ims.dll" /pdbtype:sept
# Begin Custom Build
TargetDir=.\DebugIMS
TargetPath=.\DebugIMS\phonemeextractor_ims.dll
TargetName=phonemeextractor_ims
InputPath=.\DebugIMS\phonemeextractor_ims.dll
SOURCE="$(InputPath)"

"..\..\..\bin\phonemeextractors\$(TargetName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\phonemeextractors\$(TargetName).dll attrib -r ..\..\..\bin\phonemeextractors\$(TargetName).dll 
	copy $(TargetPath) ..\..\..\bin\phonemeextractors 
	if exist $(TargetDir)\$(TargetName).map copy $(TargetDir)\$(TargetName).map ..\..\..\bin\phonemeextractors\$(TargetName).map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "phonemeextractor - Win32 Release"
# Name "phonemeextractor - Win32 Debug"
# Name "phonemeextractor - Win32 Release IMS"
# Name "phonemeextractor - Win32 Debug IMS"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\Public\amd3dx.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\..\public\characterset.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\characterset.h
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\commonmacros.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\dbg.h
# End Source File
# Begin Source File

SOURCE=.\extractor_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\fasttimer.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_helpers.h
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_tools.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_tools.h
# End Source File
# Begin Source File

SOURCE=..\..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=..\..\public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\MATHLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=..\hlfaceposer\phonemeconverter.cpp
# End Source File
# Begin Source File

SOURCE=..\hlfaceposer\PhonemeConverter.h
# End Source File
# Begin Source File

SOURCE=.\phonemeextractor.cpp

!IF  "$(CFG)" == "phonemeextractor - Win32 Release"

!ELSEIF  "$(CFG)" == "phonemeextractor - Win32 Debug"

!ELSEIF  "$(CFG)" == "phonemeextractor - Win32 Release IMS"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "phonemeextractor - Win32 Debug IMS"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\phonemeextractor.h
# End Source File
# Begin Source File

SOURCE=.\phonemeextractor_ims.cpp

!IF  "$(CFG)" == "phonemeextractor - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "phonemeextractor - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "phonemeextractor - Win32 Release IMS"

!ELSEIF  "$(CFG)" == "phonemeextractor - Win32 Debug IMS"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\platform.h
# End Source File
# Begin Source File

SOURCE=..\..\public\protected_things.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\sapi.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\sapiddk.h
# End Source File
# Begin Source File

SOURCE=..\..\public\sentence.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\sentence.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\Spddkhlp.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\spdebug.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\sperror.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\sphelper.h
# End Source File
# Begin Source File

SOURCE=..\..\public\string_t.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=.\talkback.h
# End Source File
# Begin Source File

SOURCE=..\..\public\utlbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\public\utllinkedlist.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlMemory.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlVector.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vector2d.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\vstdlib.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\sapi51\lib\i386\sapi.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
