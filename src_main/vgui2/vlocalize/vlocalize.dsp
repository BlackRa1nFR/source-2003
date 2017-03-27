# Microsoft Developer Studio Project File - Name="vlocalize" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=vlocalize - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vlocalize.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vlocalize.mak" CFG="vlocalize - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vlocalize - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vlocalize - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "vlocalize"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vlocalize - Win32 Release"

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
# ADD CPP /nologo /W3 /GR /O2 /I "..\..\public" /I "..\..\common" /I "..\..\vgui2\controls" /I "..\..\vgui2\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib WS2_32.LIB /nologo /subsystem:windows /machine:I386
# Begin Custom Build
TargetPath=.\Release\vlocalize.exe
InputPath=.\Release\vlocalize.exe
SOURCE="$(InputPath)"

"..\..\..\platform\bin\vlocalize.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\platform\bin\vlocalize.exe attrib -r ..\..\..\platform\bin\vlocalize.exe 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\platform\bin\vlocalize.exe 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vlocalize - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GR /ZI /Od /I "..\..\public" /I "..\..\common" /I "..\..\vgui2\controls" /I "..\..\vgui2\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\lib\public\vgui_controls.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib WS2_32.LIB /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"LIBCMT" /pdbtype:sept
# Begin Custom Build
TargetPath=.\Debug\vlocalize.exe
InputPath=.\Debug\vlocalize.exe
SOURCE="$(InputPath)"

"..\..\..\platform\bin\vlocalize.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\platform\bin\vlocalize.exe attrib -r ..\..\..\platform\bin\vlocalize.exe 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\platform\bin\vlocalize.exe 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "vlocalize - Win32 Release"
# Name "vlocalize - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CreateTokenDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\LocalizationDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=.\CreateTokenDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\public\dbg\dbg.h
# End Source File
# Begin Source File

SOURCE=..\..\public\platform\fasttimer.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=.\LocalizationDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\public\platform\platform.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlMemory.h
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

SOURCE=..\include\VGUI.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_Button.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_Color.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_Controls.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_Dar.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_EditablePanel.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_FileOpenDialog.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_FocusNavGroup.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_Frame.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_IClientPanel.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_IHTML.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_ILocalize.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_IScheme.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_ISurface.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_IVGui.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_KeyCode.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_KeyValues.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_Label.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_ListPanel.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_Menu.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_MenuButton.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_MessageBox.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_MessageMap.h
# End Source File
# Begin Source File

SOURCE=..\include\VGUI_MouseCode.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_Panel.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_PHandle.h
# End Source File
# Begin Source File

SOURCE=..\controls\VGUI_TextEntry.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\vstdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\Tracker\common\winlite.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\vgui_controls.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
