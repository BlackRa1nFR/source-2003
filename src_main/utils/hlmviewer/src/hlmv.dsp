# Microsoft Developer Studio Project File - Name="hlmv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=hlmv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hlmv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hlmv.mak" CFG="hlmv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hlmv - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "hlmv - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/utils/HLMVIEWER/msvc", MNMBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hlmv - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "HLMV - Release"
# PROP Intermediate_Dir "HLMV - Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /GR /Zi /O2 /I "..\..\..\statemachine" /I "..\..\mxtk\include" /I "..\..\common" /I "..\..\..\Public" /I "..\..\..\Game_Shared" /I "..\..\..\game_shared\tf2" /D "WIN32" /D "_WINDOWS" /D "PROTECTED_THINGS_DISABLE" /D "NDEBUG" /D "_WIN32" /YX /FD /I /mxtk/include" " /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x807 /d "NDEBUG"
# ADD RSC /l 0x807 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib shell32.lib gdi32.lib advapi32.lib comctl32.lib winmm.lib comdlg32.lib /nologo /version:1.2 /entry:"mainCRTStartup" /subsystem:windows /map /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"hlmv.exe" /libpath:"..\..\..\lib\common" /libpath:"..\..\..\lib\public" /release
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing Custom Build Step on $(InputPath)
TargetDir=\hl2\src\utils\hlmviewer\src
TargetPath=.\hlmv.exe
TargetName=hlmv
InputPath=.\hlmv.exe
SOURCE="$(InputPath)"

"..\..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist "..\..\..\..\bin\$(TargetName).exe" attrib -r "..\..\..\..\bin\$(TargetName).exe" 
	copy $(TargetPath) ..\..\..\..\bin 
	if exist $(TargetDir)\$(TargetName).map copy $(TargetDir)\$(TargetName).map ..\..\..\..\bin\$(TargetName).map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hlmv - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "HLMV - Debug"
# PROP Intermediate_Dir "HLMV - Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /Gm /Gi /GR /ZI /Od /I "..\..\..\statemachine" /I "..\..\mxtk\include" /I "..\..\common" /I "..\..\..\Public" /I "..\..\..\Game_Shared" /I "..\..\..\game_shared\tf2" /D "VECTOR" /D "WIN32" /D "_WINDOWS" /D "PROTECTED_THINGS_DISABLE" /D "_DEBUG" /D "_WIN32" /FR /YX /FD /I /mxtk/include" " /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x807 /d "_DEBUG"
# ADD RSC /l 0x807 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib shell32.lib gdi32.lib advapi32.lib comctl32.lib winmm.lib comdlg32.lib /nologo /version:1.1 /entry:"mainCRTStartup" /subsystem:windows /debug /machine:I386 /out:"hlmv.exe" /pdbtype:sept /libpath:"..\..\..\lib\common" /libpath:"..\..\..\lib\public"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetDir=\hl2\src\utils\hlmviewer\src
TargetPath=.\hlmv.exe
TargetName=hlmv
InputPath=.\hlmv.exe
SOURCE="$(InputPath)"

"..\..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist "..\..\..\..\bin\$(TargetName).exe" attrib -r "..\..\..\..\bin\$(TargetName).exe" 
	copy $(TargetPath) ..\..\..\..\bin 
	if exist $(TargetDir)\$(TargetName).map copy $(TargetDir)\$(TargetName).map ..\..\..\..\bin\$(TargetName).map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "hlmv - Win32 Release"
# Name "hlmv - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\attachments_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\game_shared\bone_setup.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\public\characterset.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\cmdlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\CollisionUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\src\ControlPanel.cpp
# End Source File
# Begin Source File

SOURCE=..\src\FileAssociation.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\public\filesystem_helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\FileSystem_Tools.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\ImageLoader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\Mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\src\matsyswin.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mdlviewer.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mxLineEdit2.cpp
# End Source File
# Begin Source File

SOURCE=..\src\pakviewer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\physdll.cpp
# End Source File
# Begin Source File

SOURCE=..\src\physmesh.cpp
# End Source File
# Begin Source File

SOURCE=..\src\studio_flex.cpp
# End Source File
# Begin Source File

SOURCE=..\src\studio_render.cpp
# End Source File
# Begin Source File

SOURCE=..\src\studio_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\src\sys_win.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\UtlBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\utlsymbol.cpp
# End Source File
# Begin Source File

SOURCE=..\src\ViewerSettings.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\..\..\Public\amd3dx.h
# End Source File
# Begin Source File

SOURCE=..\src\attachments_window.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\basehandle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\game_shared\bone_setup.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\bspflags.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\characterset.h
# End Source File
# Begin Source File

SOURCE=..\..\common\cmdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\cmodel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\CollisionUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\commonmacros.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\const.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\vphysics\constraints.h
# End Source File
# Begin Source File

SOURCE=..\src\ControlPanel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\tier0\dbg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\tier0\fasttimer.h
# End Source File
# Begin Source File

SOURCE=..\src\FileAssociation.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\filesystem_helpers.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\FileSystem_Tools.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\gametrace.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\vstdlib\ICommandLine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\ihandleentity.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\ImageLoader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\materialsystem\imaterial.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\materialsystem\imaterialproxyfactory.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\materialsystem\imaterialsystem.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\materialsystem\imaterialsystemhardwareconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\materialsystem\imaterialvar.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\materialsystem\imesh.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\IStudioRender.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\materialsystem\itexture.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\materialsystem\materialsystem_config.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\MATHLIB.H
# End Source File
# Begin Source File

SOURCE=..\src\matsyswin.h
# End Source File
# Begin Source File

SOURCE=..\src\mdlviewer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\tier0\memdbgoff.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\tier0\memdbgon.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\mempool.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\mouthinfo.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mx.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxBmp.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxButton.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxCheckBox.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxChoice.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxChooseColor.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxFileDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxGlWindow.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxGroupBox.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxImage.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxInit.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxLabel.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxLineEdit.h
# End Source File
# Begin Source File

SOURCE=..\src\mxLineEdit2.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxLinkedList.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxListBox.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxMatSysWindow.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxMenu.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxMenuBar.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxMessageBox.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxpath.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxPcx.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxPopupMenu.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxProgressBar.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxRadioButton.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxScrollbar.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxSlider.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxstring.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxTab.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxTga.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxToggleButton.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxToolTip.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxTreeView.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxWidget.h
# End Source File
# Begin Source File

SOURCE=..\..\mxtk\include\mx\mxWindow.h
# End Source File
# Begin Source File

SOURCE=..\src\pakviewer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\phyfile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\physdll.h
# End Source File
# Begin Source File

SOURCE=..\src\physmesh.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\tier0\platform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\protected_things.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\vstdlib\random.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\s3_intrf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\string_t.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\studio.h
# End Source File
# Begin Source File

SOURCE=..\src\studio_render.h
# End Source File
# Begin Source File

SOURCE=..\src\StudioModel.h
# End Source File
# Begin Source File

SOURCE=..\src\sys.h
# End Source File
# Begin Source File

SOURCE=..\..\..\game_shared\tf_shareddefs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\TGALoader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\UtlBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\utllinkedlist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\UtlMemory.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\utlrbtree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\utlsymbol.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\UtlVector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\vcollide.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\vcollide_parse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\vector2d.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\vector4d.h
# End Source File
# Begin Source File

SOURCE=..\src\ViewerSettings.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Public\vphysics_interface.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\tier0\vprof.h
# End Source File
# Begin Source File

SOURCE=..\..\..\public\vstdlib\vstdlib.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\hlmviewer.rc
# End Source File
# Begin Source File

SOURCE=..\src\icon1.ico
# End Source File
# Begin Source File

SOURCE=..\src\resource.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\lib\common\mxtk.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\common\s3tc.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
