# Microsoft Developer Studio Project File - Name="vgui_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=vgui_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vgui_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vgui_dll.mak" CFG="vgui_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vgui_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vgui_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GoldSrc/vgui2/src", XCLCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vgui_dll - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VGUI_DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /W3 /GR /Zi /O2 /I "..\include" /I "..\..\public" /I "..\..\common" /I "." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DONT_PROTECT_FILEIO_FUNCTIONS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /dll /map /debug /machine:I386 /out:"Release/vgui2.dll"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetDir=.\Release
TargetPath=.\Release\vgui2.dll
InputPath=.\Release\vgui2.dll
SOURCE="$(InputPath)"

"..\..\..\bin\vgui2.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\vgui2.dll attrib -r ..\..\..\bin\vgui2.dll 
	copy $(TargetPath) ..\..\..\bin\vgui2.dll 
	if exist $(TargetDir)\vgui2.map copy $(TargetDir)\vgui2.map ..\..\..\bin\vgui2.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vgui_dll - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VGUI_DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /W3 /Gm /GR /ZI /Od /I "..\include" /I "..\..\public" /I "..\..\common" /I "." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DONT_PROTECT_FILEIO_FUNCTIONS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"LIBCMTD" /out:"Debug/vgui2.dll" /pdbtype:sept
# Begin Custom Build
TargetDir=.\Debug
TargetPath=.\Debug\vgui2.dll
InputPath=.\Debug\vgui2.dll
SOURCE="$(InputPath)"

"..\..\..\bin\vgui2.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\vgui2.dll attrib -r ..\..\..\bin\vgui2.dll 
	copy $(TargetPath) ..\..\..\bin\vgui2.dll 
	if exist $(TargetDir)\vgui2.map copy $(TargetDir)\vgui2.map ..\..\..\bin\vgui2.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "vgui_dll - Win32 Release"
# Name "vgui_dll - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Bitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\Border.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\characterset.cpp
# End Source File
# Begin Source File

SOURCE=.\fileimage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_helpers.cpp
# End Source File
# Begin Source File

SOURCE=.\HtmlWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\InputWin32.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\KeyValues.cpp
# End Source File
# Begin Source File

SOURCE=.\LocalizedStringTable.cpp
# End Source File
# Begin Source File

SOURCE=.\MemoryBitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\Memorybitmap.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=.\MessageListener.cpp
# End Source File
# Begin Source File

SOURCE=.\Scheme.cpp
# End Source File
# Begin Source File

SOURCE=.\Surface.cpp
# End Source File
# Begin Source File

SOURCE=.\System.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\utlbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\utlsymbol.cpp
# End Source File
# Begin Source File

SOURCE=.\vgui.cpp
# End Source File
# Begin Source File

SOURCE=.\vgui_internal.cpp
# End Source File
# Begin Source File

SOURCE=.\vgui_key_translation.cpp
# End Source File
# Begin Source File

SOURCE=.\VPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\VPanelWrapper.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=.\bitmap.h
# End Source File
# Begin Source File

SOURCE=..\..\public\Color.h
# End Source File
# Begin Source File

SOURCE=..\..\public\dbg\dbg.h
# End Source File
# Begin Source File

SOURCE=..\..\public\platform\fasttimer.h
# End Source File
# Begin Source File

SOURCE=.\fileimage.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\common\vgui_surfacelib\FontAmalgam.h
# End Source File
# Begin Source File

SOURCE=..\..\common\vgui_surfacelib\FontManager.h
# End Source File
# Begin Source File

SOURCE=..\..\common\vgui\HtmlWindow.h
# End Source File
# Begin Source File

SOURCE=.\IInputInternal.h
# End Source File
# Begin Source File

SOURCE=..\..\public\IKeyValues.h
# End Source File
# Begin Source File

SOURCE=.\IMessageListener.h
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\KeyValues.h
# End Source File
# Begin Source File

SOURCE=..\..\public\platform\platform.h
# End Source File
# Begin Source File

SOURCE=..\..\common\SteamBootStrapper.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=..\..\public\utlbuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\public\utllinkedlist.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlMemory.h
# End Source File
# Begin Source File

SOURCE=..\..\public\utlpriorityqueue.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\utlrbtree.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlVector.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vector2d.h
# End Source File
# Begin Source File

SOURCE=.\vgui_internal.h
# End Source File
# Begin Source File

SOURCE=.\VPanel.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\vstdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\common\vgui_surfacelib\Win32Font.h
# End Source File
# End Group
# Begin Group "Interfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\IBorder.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\IClientPanel.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\IHTML.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\IImage.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\IInput.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\ILocalize.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\IPanel.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\IScheme.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\ISurface.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\ISystem.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vgui\IVGui.h
# End Source File
# Begin Source File

SOURCE=.\VGUI_Border.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\vgui_surfacelib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
