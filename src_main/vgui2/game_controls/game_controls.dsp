# Microsoft Developer Studio Project File - Name="game_controls" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=game_controls - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "game_controls.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "game_controls.mak" CFG="game_controls - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "game_controls - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "game_controls - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "game_controls"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "game_controls - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "..\..\public" /I "..\..\common" /I "..\..\game_shared" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Custom Build
TargetDir=.\Release
TargetName=game_controls
InputPath=.\Release\game_controls.lib
SOURCE="$(InputPath)"

"..\..\lib\public\$(TargetName)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\lib\public\$(TargetName).lib attrib -r ..\..\lib\public\$(TargetName).lib 
	if exist $(TargetDir)\game_controls.lib copy $(TargetDir)\game_controls.lib ..\..\lib\public 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "game_controls - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G5 /MTd /W4 /Gm /GR /ZI /Od /I "..\..\public" /I "..\..\common" /I "..\..\game_shared" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Custom Build
TargetDir=.\Debug
TargetName=game_controls
InputPath=.\Debug\game_controls.lib
SOURCE="$(InputPath)"

"..\..\lib\public\$(TargetName)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\lib\public\$(TargetName).lib attrib -r ..\..\lib\public\$(TargetName).lib 
	if exist $(TargetDir)\game_controls.lib copy $(TargetDir)\game_controls.lib ..\..\lib\public 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "game_controls - Win32 Release"
# Name "game_controls - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\classmenu.cpp

!IF  "$(CFG)" == "game_controls - Win32 Release"

!ELSEIF  "$(CFG)" == "game_controls - Win32 Debug"

# ADD CPP /G5

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clientmotd.cpp

!IF  "$(CFG)" == "game_controls - Win32 Release"

!ELSEIF  "$(CFG)" == "game_controls - Win32 Debug"

# ADD CPP /G5

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ClientScoreBoardDialog.cpp

!IF  "$(CFG)" == "game_controls - Win32 Release"

!ELSEIF  "$(CFG)" == "game_controls - Win32 Debug"

# ADD CPP /G5

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CommandMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\MapOverview.cpp
# End Source File
# Begin Source File

SOURCE=.\SpectatorGUI.cpp

!IF  "$(CFG)" == "game_controls - Win32 Release"

!ELSEIF  "$(CFG)" == "game_controls - Win32 Debug"

# ADD CPP /G5

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\teammenu.cpp

!IF  "$(CFG)" == "game_controls - Win32 Release"

!ELSEIF  "$(CFG)" == "game_controls - Win32 Debug"

# ADD CPP /G5

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vgui_TeamFortressViewport.cpp

!IF  "$(CFG)" == "game_controls - Win32 Release"

!ELSEIF  "$(CFG)" == "game_controls - Win32 Debug"

# ADD CPP /G5

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vgui_TeamFortressViewport_msgs.cpp

!IF  "$(CFG)" == "game_controls - Win32 Release"

!ELSEIF  "$(CFG)" == "game_controls - Win32 Debug"

# ADD CPP /I "..\..\cl_dll"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vguitextwindow.cpp

!IF  "$(CFG)" == "game_controls - Win32 Release"

!ELSEIF  "$(CFG)" == "game_controls - Win32 Debug"

# ADD CPP /G5

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\public\game_controls\classmenu.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\clientmotd.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\ClientScoreBoardDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\iscoreboardinterface.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\ispectatorinterface.h
# End Source File
# Begin Source File

SOURCE=..\..\public\cl_dll\IVGuiClientDll.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\iviewport.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\iviewportmsgs.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\MapOverview.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\mouseoverhtmlbutton.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\mouseoverpanelbutton.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\SpectatorGUI.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\teammenu.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\utlqueue.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\vgui_TeamFortressViewport.h
# End Source File
# Begin Source File

SOURCE=..\..\public\game_controls\vguitextwindow.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\MiniMap.h
# End Source File
# End Target
# End Project
