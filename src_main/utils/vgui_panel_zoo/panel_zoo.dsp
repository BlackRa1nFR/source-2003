# Microsoft Developer Studio Project File - Name="panel_zoo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=panel_zoo - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "panel_zoo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "panel_zoo.mak" CFG="panel_zoo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "panel_zoo - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "panel_zoo - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "panel_zoo"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "panel_zoo - Win32 Release"

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
# ADD CPP /nologo /W4 /GR /GX /O2 /I "..\..\public" /I "..\..\vgui2\vlocalize" /I "..\..\common" /I "..\..\vgui2\controls" /I "..\..\vgui2\include" /I "../../vgui2/controls" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc.lib"
# Begin Custom Build
TargetPath=.\Release\panel_zoo.exe
InputPath=.\Release\panel_zoo.exe
SOURCE="$(InputPath)"

"..\..\..\Platform\Bin\panel_zoo.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\Platform\Bin\panel_zoo.exe attrib -r ..\..\..\Platform\Bin\panel_zoo.exe 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\Platform\Bin\panel_zoo.exe 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "panel_zoo - Win32 Debug"

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
# ADD CPP /nologo /MTd /W4 /Gm /GR /GX /ZI /Od /I "..\..\public" /I "..\..\vgui2\vlocalize" /I "..\..\common" /I "..\..\vgui2\controls" /I "..\..\vgui2\include" /I "../../vgui2/controls" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept
# Begin Custom Build
TargetPath=.\Debug\panel_zoo.exe
InputPath=.\Debug\panel_zoo.exe
SOURCE="$(InputPath)"

"..\..\..\bin\panel_zoo.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\panel_zoo.exe attrib -r ..\..\..\bin\panel_zoo.exe 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\bin\panel_zoo.exe 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "panel_zoo - Win32 Release"
# Name "panel_zoo - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AnimatingImagePanelDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\ButtonDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\ButtonDemo2.cpp
# End Source File
# Begin Source File

SOURCE=.\CascadingMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\CControlCatalog.cpp
# End Source File
# Begin Source File

SOURCE=.\CheckButtonDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\ComboBox2.cpp
# End Source File
# Begin Source File

SOURCE=.\ComboBoxDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\DefaultColors.cpp
# End Source File
# Begin Source File

SOURCE=.\DemoPage.cpp
# End Source File
# Begin Source File

SOURCE=.\EditablePanel2Demo.cpp
# End Source File
# Begin Source File

SOURCE=.\EditablePanelDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\FileOpenDialogDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\FrameDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLDemo2.cpp
# End Source File
# Begin Source File

SOURCE=.\ImageDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\ImagePanelDemo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\Label2Demo.cpp
# End Source File
# Begin Source File

SOURCE=.\LabelDemo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vgui2\controls\LabeledSlider.cpp
# End Source File
# Begin Source File

SOURCE=.\ListPanelDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\MenuBarDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\MenuDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\MenuDemo2.cpp
# End Source File
# Begin Source File

SOURCE=.\MessageBoxDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgressBarDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\QueryBoxDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\RadioButtonDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleButtons.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleCheckButtons.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleDropDowns.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleEditFields.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleListCategories.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleListPanelBoth.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleListPanelColumns.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleMenus.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleRadioButtons.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleSliders.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleTabs.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrollBar2Demo.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrollBarDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\testfile.cpp
# End Source File
# Begin Source File

SOURCE=.\TextEntryDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\TextEntryDemo2.cpp
# End Source File
# Begin Source File

SOURCE=.\TextEntryDemo3.cpp
# End Source File
# Begin Source File

SOURCE=.\TextEntryDemo4.cpp
# End Source File
# Begin Source File

SOURCE=.\TextEntryDemo5.cpp
# End Source File
# Begin Source File

SOURCE=.\TextImageDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\ToggleButtonDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\TooltipDemo.cpp
# End Source File
# Begin Source File

SOURCE=.\WizardPanelDemo.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CControlCatalog.h
# End Source File
# Begin Source File

SOURCE=.\DemoPage.h
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.h
# End Source File
# Begin Source File

SOURCE=.\MenuDemo.h
# End Source File
# Begin Source File

SOURCE=.\SampleListPanelBoth.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\..\lib\public\vgui_controls.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
