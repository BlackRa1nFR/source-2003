# Microsoft Developer Studio Project File - Name="GameUI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=GameUI - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GameUI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GameUI.mak" CFG="GameUI - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GameUI - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "GameUI - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GoldSrc/GameUI", ZIRCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GameUI - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GAMEUI_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /O2 /I "..\public" /I "..\common" /I "..\hltv\common" /I "..\tracker\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GAMEUI_EXPORTS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 Shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /machine:I386 /nodefaultlib:"LIBC" /nodefaultlib:"LIBCD" /nodefaultlib:"LIBCMTD"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
ProjDir=.
InputPath=.\Release\GameUI.dll
InputName=GameUI
SOURCE="$(InputPath)"

BuildCmds= \
	if exist $(ProjDir)\..\..\tf2\bin\$(InputName).dll attrib -r $(ProjDir)\..\..\tf2\bin\$(InputName).dll \
	copy $(InputPath) $(ProjDir)\..\..\tf2\bin\$(InputName).dll \
	if exist $(ProjDir)\..\..\hl2\bin\$(InputName).dll attrib -r $(ProjDir)\..\..\hl2\bin\$(InputName).dll \
	copy $(InputPath) $(ProjDir)\..\..\hl2\bin\$(InputName).dll \
	if exist $(ProjDir)\..\..\hl1\bin\$(InputName).dll attrib -r $(ProjDir)\..\..\hl1\bin\$(InputName).dll \
	copy $(InputPath) $(ProjDir)\..\..\hl1\bin\$(InputName).dll \
	if exist $(ProjDir)\..\..\cstrike\bin\$(InputName).dll attrib -r $(ProjDir)\..\..\cstrike\bin\$(InputName).dll \
	copy $(InputPath) $(ProjDir)\..\..\cstrike\bin\$(InputName).dll \
	

"$(ProjDir)\..\..\tf2\bin\$(InputName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\..\..\hl2\bin\$(InputName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\..\..\hl1\bin\$(InputName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\..\..\cstrike\bin\$(InputName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "GameUI - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GAMEUI_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /W3 /Gm /GR /ZI /Od /I "..\vgui2\include" /I "..\vgui2\controls" /I "..\public" /I "..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GAMEUI_EXPORTS" /Fr /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 gdi32.lib kernel32.lib user32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib /nologo /dll /map /debug /machine:I386 /nodefaultlib:"LIBC" /nodefaultlib:"LIBCMT" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
ProjDir=.
InputPath=.\Debug\GameUI.dll
InputName=GameUI
SOURCE="$(InputPath)"

BuildCmds= \
	if exist $(ProjDir)\..\..\tf2\bin\$(InputName).dll attrib -r $(ProjDir)\..\..\tf2\bin\$(InputName).dll \
	copy $(InputPath) $(ProjDir)\..\..\tf2\bin\$(InputName).dll \
	if exist $(ProjDir)\..\..\hl2\bin\$(InputName).dll attrib -r $(ProjDir)\..\..\hl2\bin\$(InputName).dll \
	copy $(InputPath) $(ProjDir)\..\..\hl2\bin\$(InputName).dll \
	if exist $(ProjDir)\..\..\hl1\bin\$(InputName).dll attrib -r $(ProjDir)\..\..\hl1\bin\$(InputName).dll \
	copy $(InputPath) $(ProjDir)\..\..\hl1\bin\$(InputName).dll \
	if exist $(ProjDir)\..\..\cstrike\bin\$(InputName).dll attrib -r $(ProjDir)\..\..\cstrike\bin\$(InputName).dll \
	copy $(InputPath) $(ProjDir)\..\..\cstrike\bin\$(InputName).dll \
	

"$(ProjDir)\..\..\tf2\bin\$(InputName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\..\..\hl2\bin\$(InputName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\..\..\hl1\bin\$(InputName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\..\..\cstrike\bin\$(InputName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# Begin Target

# Name "GameUI - Win32 Release"
# Name "GameUI - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BackgroundMenuButton.cpp
# End Source File
# Begin Source File

SOURCE=.\BasePanel.cpp
# End Source File
# Begin Source File

SOURCE=.\ChangeGameDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\public\checksum_md5.cpp
# End Source File
# Begin Source File

SOURCE=..\public\convar.cpp
# End Source File
# Begin Source File

SOURCE=.\CreateMultiplayerGameBotPage.cpp
# End Source File
# Begin Source File

SOURCE=.\CreateMultiplayerGameDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\CreateMultiplayerGameGameplayPage.cpp
# End Source File
# Begin Source File

SOURCE=.\CreateMultiplayerGameServerPage.cpp
# End Source File
# Begin Source File

SOURCE=.\GameConsole.cpp
# End Source File
# Begin Source File

SOURCE=.\GameUI_Interface.cpp
# End Source File
# Begin Source File

SOURCE=..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadingDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\LogoFile.cpp
# End Source File
# Begin Source File

SOURCE=.\ModInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\Tracker\common\msgbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\Tracker\common\netapi.cpp
# End Source File
# Begin Source File

SOURCE=..\common\GameUI\ObjectList.cpp
# End Source File
# Begin Source File

SOURCE=..\common\GameUI\Random.cpp
# End Source File
# Begin Source File

SOURCE=.\RunGameEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\ScriptObject.cpp
# End Source File
# Begin Source File

SOURCE=..\Tracker\common\Socket.cpp
# End Source File
# Begin Source File

SOURCE=.\Sys_Utils.cpp
# End Source File
# Begin Source File

SOURCE=.\Taskbar.cpp
# End Source File
# Begin Source File

SOURCE=.\TaskButton.cpp
# End Source File
# Begin Source File

SOURCE=.\TaskButton.h
# End Source File
# Begin Source File

SOURCE=..\public\UtlBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\public\utlsymbol.cpp
# End Source File
# Begin Source File

SOURCE=.\VGuiSystemModuleLoader.cpp
# End Source File
# Begin Source File

SOURCE=.\VGuiSystemModuleLoader.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BackgroundMenuButton.h
# End Source File
# Begin Source File

SOURCE=.\BasePanel.h
# End Source File
# Begin Source File

SOURCE=.\ChangeGameDialog.h
# End Source File
# Begin Source File

SOURCE=.\CreateMultiplayerGameDialog.h
# End Source File
# Begin Source File

SOURCE=.\CreateMultiplayerGameGameplayPage.h
# End Source File
# Begin Source File

SOURCE=.\CreateMultiplayerGameServerPage.h
# End Source File
# Begin Source File

SOURCE=.\DemoEditDialog.h
# End Source File
# Begin Source File

SOURCE=.\DemoEventsDialog.h
# End Source File
# Begin Source File

SOURCE=.\DemoPlayerDialog.h
# End Source File
# Begin Source File

SOURCE=.\DemoPlayerFileDialog.h
# End Source File
# Begin Source File

SOURCE=.\GameConsole.h
# End Source File
# Begin Source File

SOURCE=.\GameUI_Interface.h
# End Source File
# Begin Source File

SOURCE=..\public\BaseUI\IBaseUI.h
# End Source File
# Begin Source File

SOURCE=..\public\cl_dll\IGameClientExports.h
# End Source File
# Begin Source File

SOURCE=..\public\BaseUI\IGameConsole.h
# End Source File
# Begin Source File

SOURCE=..\common\GameUI\IGameUI.h
# End Source File
# Begin Source File

SOURCE=..\public\interface.h
# End Source File
# Begin Source File

SOURCE=..\common\IObjectContainer.h
# End Source File
# Begin Source File

SOURCE=..\common\IRunGameEngine.h
# End Source File
# Begin Source File

SOURCE=..\common\IVGuiModule.h
# End Source File
# Begin Source File

SOURCE=..\common\IVGuiModuleLoader.h
# End Source File
# Begin Source File

SOURCE=.\LoadingDialog.h
# End Source File
# Begin Source File

SOURCE=..\vgui2\src\Memorybitmap.h
# End Source File
# Begin Source File

SOURCE=.\ModInfo.h
# End Source File
# Begin Source File

SOURCE=..\common\GameUI\ObjectList.h
# End Source File
# Begin Source File

SOURCE=..\common\GameUI\Random.h
# End Source File
# Begin Source File

SOURCE=.\ScriptObject.h
# End Source File
# Begin Source File

SOURCE=.\Sys_Utils.h
# End Source File
# Begin Source File

SOURCE=.\Taskbar.h
# End Source File
# Begin Source File

SOURCE=.\taskframe.h
# End Source File
# Begin Source File

SOURCE=..\Tracker\common\TrackerMessageFlags.h
# End Source File
# End Group
# Begin Group "Controls"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BitmapImagePanel.cpp
# End Source File
# Begin Source File

SOURCE=.\BitmapImagePanel.h
# End Source File
# Begin Source File

SOURCE=.\CommandCheckButton.cpp
# End Source File
# Begin Source File

SOURCE=.\CommandCheckButton.h
# End Source File
# Begin Source File

SOURCE=.\CvarNegateCheckButton.cpp
# End Source File
# Begin Source File

SOURCE=.\CvarNegateCheckButton.h
# End Source File
# Begin Source File

SOURCE=.\CvarSlider.cpp
# End Source File
# Begin Source File

SOURCE=.\CvarSlider.h
# End Source File
# Begin Source File

SOURCE=.\CvarTextEntry.cpp
# End Source File
# Begin Source File

SOURCE=.\CvarTextEntry.h
# End Source File
# Begin Source File

SOURCE=.\CvarToggleCheckButton.cpp
# End Source File
# Begin Source File

SOURCE=.\CvarToggleCheckButton.h
# End Source File
# Begin Source File

SOURCE=.\KeyToggleCheckButton.cpp
# End Source File
# Begin Source File

SOURCE=.\KeyToggleCheckButton.h
# End Source File
# Begin Source File

SOURCE=.\LabeledCommandComboBox.cpp
# End Source File
# Begin Source File

SOURCE=.\LabeledCommandComboBox.h
# End Source File
# Begin Source File

SOURCE=.\PanelListPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelListPanel.h
# End Source File
# Begin Source File

SOURCE=.\vcontrolslistpanel.cpp
# End Source File
# Begin Source File

SOURCE=.\vcontrolslistpanel.h
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ContentControlDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ContentControlDialog.h
# End Source File
# Begin Source File

SOURCE=.\GameConsoleDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\GameConsoleDialog.h
# End Source File
# Begin Source File

SOURCE=.\HelpDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\HelpDialog.h
# End Source File
# Begin Source File

SOURCE=.\LoadGameDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadGameDialog.h
# End Source File
# Begin Source File

SOURCE=.\MultiplayerAdvancedDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiplayerAdvancedDialog.h
# End Source File
# Begin Source File

SOURCE=.\NewGameDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\NewGameDialog.h
# End Source File
# Begin Source File

SOURCE=.\PlayerListDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerListDialog.h
# End Source File
# Begin Source File

SOURCE=.\SaveGameDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\SaveGameDialog.h
# End Source File
# Begin Source File

SOURCE=.\SteamPasswordDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\SteamPasswordDialog.h
# End Source File
# End Group
# Begin Group "Options Dialog"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\OptionsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsDialog.h
# End Source File
# Begin Source File

SOURCE=.\OptionsSubAdvanced.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsSubAdvanced.h
# End Source File
# Begin Source File

SOURCE=.\OptionsSubAudio.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsSubAudio.h
# End Source File
# Begin Source File

SOURCE=.\OptionsSubKeyboard.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsSubKeyboard.h
# End Source File
# Begin Source File

SOURCE=.\OptionsSubMouse.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsSubMouse.h
# End Source File
# Begin Source File

SOURCE=.\OptionsSubMultiplayer.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsSubMultiplayer.h
# End Source File
# Begin Source File

SOURCE=.\OptionsSubVideo.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsSubVideo.h
# End Source File
# Begin Source File

SOURCE=.\OptionsSubVoice.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsSubVoice.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\lib\public\vgui_controls.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\tier0.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\vstdlib.lib
# End Source File
# End Target
# End Project
