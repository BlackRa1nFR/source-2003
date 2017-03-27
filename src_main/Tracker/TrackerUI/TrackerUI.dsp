# Microsoft Developer Studio Project File - Name="TrackerUI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TrackerUI - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TrackerUI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TrackerUI.mak" CFG="TrackerUI - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TrackerUI - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TrackerUI - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GoldSrc/Tracker/TrackerUI", QQACAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TrackerUI - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERUI_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "..\..\vgui2\include" /I "..\..\vgui2\controls" /I "..\..\common" /I "..\common" /I "..\..\public" /I "..\public" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERUI_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib /nologo /dll /machine:I386
# SUBTRACT LINK32 /incremental:yes /map /debug
# Begin Custom Build
TargetPath=.\Release\TrackerUI.dll
InputPath=.\Release\TrackerUI.dll
SOURCE="$(InputPath)"

"..\..\..\Platform\Friends\TrackerUI.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\Platform\Friends\TrackerUI.dll attrib -r ..\..\..\Platform\Friends\TrackerUI.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\Platform\Friends\TrackerUI.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TrackerUI - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERUI_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "..\..\vgui2\include" /I "..\..\vgui2\controls" /I "..\..\common" /I "..\common" /I "..\..\public" /I "..\public" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERUI_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib /nologo /dll /map /debug /machine:I386 /pdbtype:sept
# Begin Custom Build
TargetPath=.\Debug\TrackerUI.dll
InputPath=.\Debug\TrackerUI.dll
SOURCE="$(InputPath)"

"..\..\..\Platform\Friends\TrackerUI.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\Platform\Friends\TrackerUI.dll attrib -r ..\..\..\Platform\Friends\TrackerUI.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\Platform\Friends\TrackerUI.dll 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "TrackerUI - Win32 Release"
# Name "TrackerUI - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Buddy.cpp
# End Source File
# Begin Source File

SOURCE=.\BuddyButton.cpp
# End Source File
# Begin Source File

SOURCE=.\BuddySectionHeader.cpp
# End Source File
# Begin Source File

SOURCE=.\BuildNum.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogAbout.cpp
# End Source File
# Begin Source File

SOURCE=..\common\IceKey.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\Info.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\OnlineStatus.cpp
# End Source File
# Begin Source File

SOURCE=.\OnlineStatusSelectComboBox.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelValveLogo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\Random.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Platform\Script1.rc
# End Source File
# Begin Source File

SOURCE=.\ServerSession.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelBuddyList.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelTrackerState.cpp
# End Source File
# Begin Source File

SOURCE=.\Tracker.cpp
# End Source File
# Begin Source File

SOURCE=.\TrackerDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\TrackerDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\TrackerRunGameEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\TrackerUIVGuiModule.cpp
# End Source File
# Begin Source File

SOURCE=.\TrackerUser.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Buddy.h
# End Source File
# Begin Source File

SOURCE=.\BuddyButton.h
# End Source File
# Begin Source File

SOURCE=.\BuddySectionHeader.h
# End Source File
# Begin Source File

SOURCE=.\DialogAbout.h
# End Source File
# Begin Source File

SOURCE=..\..\common\info.h
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.h
# End Source File
# Begin Source File

SOURCE=..\..\common\ITrackerUser.h
# End Source File
# Begin Source File

SOURCE=.\OnlineStatus.h
# End Source File
# Begin Source File

SOURCE=.\OnlineStatusSelectComboBox.h
# End Source File
# Begin Source File

SOURCE=.\PanelValveLogo.h
# End Source File
# Begin Source File

SOURCE=..\..\common\Random.h
# End Source File
# Begin Source File

SOURCE=.\ServerSession.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelBuddyList.h
# End Source File
# Begin Source File

SOURCE=.\SystemBuddyButton.h
# End Source File
# Begin Source File

SOURCE=.\Tracker.h
# End Source File
# Begin Source File

SOURCE=.\TrackerDialog.h
# End Source File
# Begin Source File

SOURCE=.\TrackerDoc.h
# End Source File
# Begin Source File

SOURCE=..\common\TrackerProtocol.h
# End Source File
# End Group
# Begin Group "Login Wizard"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DialogLoginUser.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogLoginUser.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelConnectionIntro.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelConnectionIntro.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelConnectionTest.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelConnectionTest.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelCreateUser.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelCreateUser.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelCreateUser2.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelCreateUser2.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelCreateUser3.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelCreateUser3.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelError.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelError.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelLoginUser.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelLoginUser.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelLoginUser2.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelLoginUser2.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelSelectLoginOption.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelSelectLoginOption.h
# End Source File
# End Group
# Begin Group "Find Buddy Wizard"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DialogFindBuddy.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogFindBuddy.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelFindBuddy.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelFindBuddy.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelFindBuddyComplete.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelFindBuddyComplete.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelFindBuddyRequestAuth.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelFindBuddyRequestAuth.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelFindBuddyResults.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelFindBuddyResults.h
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DialogAuthRequest.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogAuthRequest.h
# End Source File
# Begin Source File

SOURCE=.\DialogChat.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogChat.h
# End Source File
# Begin Source File

SOURCE=.\DialogRemoveUser.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogRemoveUser.h
# End Source File
# End Group
# Begin Group "Options Dialog"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DialogOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogOptions.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelOptionsChat.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelOptionsChat.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelOptionsConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelOptionsConnection.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelOptionsPersonal.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelOptionsPersonal.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelOptionsSounds.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelOptionsSounds.h
# End Source File
# End Group
# Begin Group "UserInfo Dialog"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DialogUserInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogUserInfo.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelUserInfoDetails.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelUserInfoDetails.h
# End Source File
# Begin Source File

SOURCE=.\SubPanelUserInfoStatus.cpp
# End Source File
# Begin Source File

SOURCE=.\SubPanelUserInfoStatus.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Platform\icon1.ico
# End Source File
# Begin Source File

SOURCE=..\..\vgui2\bin\vgui_controls.lib
# End Source File
# End Target
# End Project
