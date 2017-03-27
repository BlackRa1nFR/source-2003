# Microsoft Developer Studio Project File - Name="mxtoolkitwin32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=mxtoolkitwin32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mxtoolkitwin32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mxtoolkitwin32.mak" CFG="mxtoolkitwin32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mxtoolkitwin32 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "mxtoolkitwin32 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/utils/mxtk/msvc", ODRCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "mxToolKit - Release"
# PROP Intermediate_Dir "mxToolKit - Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /Z7 /O2 /I "../include" /I "../../../common/MaterialSystem" /I "../../../Public" /D "_WIN32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\mxtk.lib"
# Begin Custom Build
TargetDir=.\Release
InputPath=.\Release\mxtk.lib
SOURCE="$(InputPath)"

"..\..\..\lib\common\mxtk.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\lib\common\mxtk.lib attrib -r ..\..\..\lib\common\mxtk.lib 
	copy $(TargetDir)\mxtk.lib ..\..\..\lib\common\mxtk.lib 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "mxToolKit - Debug"
# PROP Intermediate_Dir "mxToolKit - Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /Z7 /Od /I "../include" /I "../../../common/MaterialSystem" /I "../../../Public" /D "_WIN32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\mxtk.lib"
# Begin Custom Build
TargetDir=.\Debug
InputPath=.\Debug\mxtk.lib
SOURCE="$(InputPath)"

"..\..\..\lib\common\mxtk.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\lib\common\mxtk.lib attrib -r ..\..\..\lib\common\mxtk.lib 
	copy $(TargetDir)\mxtk.lib ..\..\..\lib\common\mxtk.lib 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "mxtoolkitwin32 - Win32 Release"
# Name "mxtoolkitwin32 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=..\src\win32\mx.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\common\mxBmp.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxButton.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxCheckBox.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxChoice.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxChooseColor.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxFileDialog.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxGlWindow.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxGroupBox.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxLabel.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxLineEdit.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxListBox.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxlistview.cpp
# End Source File
# Begin Source File

SOURCE=..\src\win32\mxMatSysWindow.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxMenu.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxMenuBar.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxMessageBox.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\common\mxpath.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\common\mxPcx.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxPopupMenu.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxProgressBar.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxRadioButton.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxScrollbar.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxSlider.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\common\mxstring.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxTab.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\common\mxTga.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxToolTip.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxTreeView.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxWidget.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\win32\mxWindow.cpp

!IF  "$(CFG)" == "mxtoolkitwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mxtoolkitwin32 - Win32 Debug"

# ADD CPP /FR

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=..\include\mx\gl.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mx.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxBmp.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxButton.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxCheckBox.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxChoice.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxChooseColor.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxEvent.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxFileDialog.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxGlWindow.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxGroupBox.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxImage.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxInit.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxLabel.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxLineEdit.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxLinkedList.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxListBox.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxlistview.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxMatSysWindow.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxMenu.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxMenuBar.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxMessageBox.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxpath.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxPcx.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxPopupMenu.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxProgressBar.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxRadioButton.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxScrollbar.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxSlider.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxstring.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxTab.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxTga.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxToggleButton.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxToolTip.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxTreeView.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxWidget.h
# End Source File
# Begin Source File

SOURCE=..\include\mx\mxWindow.h
# End Source File
# End Group
# End Target
# End Project
