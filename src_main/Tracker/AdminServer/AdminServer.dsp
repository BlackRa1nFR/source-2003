# Microsoft Developer Studio Project File - Name="AdminServer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=AdminServer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "AdminServer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "AdminServer.mak" CFG="AdminServer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AdminServer - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "AdminServer - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GoldSrc/Tracker/AdminServer", FUODAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "AdminServer - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ADMINSERVER_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I "..\public" /I "..\..\public" /I "..\common" /I "..\..\common" /I "..\..\vgui2\controls" /I "..\..\vgui2\include" /I "..\..\shared" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ADMINSERVER_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 wsock32.lib ..\..\vgui2\bin\vgui_controls.lib /nologo /dll /machine:I386 /nodefaultlib:"LIBCMTD" /nodefaultlib:"LIBC" /nodefaultlib:"LIBCD"
# Begin Custom Build
TargetPath=.\Release\AdminServer.dll
InputPath=.\Release\AdminServer.dll
SOURCE="$(InputPath)"

"..\..\..\Platform\Admin\AdminServer.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\Platform\Admin\AdminServer.dll attrib -r ..\..\..\Platform\Admin\AdminServer.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\Platform\Admin\AdminServer.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "AdminServer - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ADMINSERVER_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "..\..\shared\\" /I "..\public" /I "..\..\public" /I "..\common" /I "..\..\common" /I "..\..\vgui2\controls" /I "..\..\vgui2\include" /I "..\..\shared" /I "..\trackerui" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ADMINSERVER_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib ..\..\vgui2\bin\vgui_controls.lib /nologo /dll /map /debug /machine:I386 /pdbtype:sept
# Begin Custom Build
TargetPath=.\Debug\AdminServer.dll
InputPath=.\Debug\AdminServer.dll
SOURCE="$(InputPath)"

"..\..\..\Platform\Admin\AdminServer.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\Platform\Admin\AdminServer.dll attrib -r ..\..\..\Platform\Admin\AdminServer.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\Platform\Admin\AdminServer.dll 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "AdminServer - Win32 Release"
# Name "AdminServer - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Generic Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DialogAddBan.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogAddServer.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogCvarChange.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogGameInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogKickPlayer.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogServerPassword.cpp
# End Source File
# End Group
# Begin Group "Favorites"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BaseGamesPage.cpp
# End Source File
# Begin Source File

SOURCE=.\FavoriteGames.cpp
# End Source File
# End Group
# Begin Group "Manage"

# PROP Default_Filter ""
# Begin Group "Menus"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BanContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\RulesContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerContextMenu.cpp
# End Source File
# End Group
# Begin Group "Msg Handlers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\logmsghandler.cpp
# End Source File
# Begin Source File

SOURCE=.\MasterMsgHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\playermsghandler.cpp
# End Source File
# Begin Source File

SOURCE=.\rconmsghandler.cpp
# End Source File
# Begin Source File

SOURCE=.\rulesinfomsghandler.cpp
# End Source File
# Begin Source File

SOURCE=.\serverinfomsghandler.cpp
# End Source File
# Begin Source File

SOURCE=.\serverpingmsghandler.cpp
# End Source File
# End Group
# Begin Group "Panels"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BanPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\ChatPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\GraphPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\MOTDPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\RawLogPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerConfigPanel.cpp
# End Source File
# End Group
# Begin Group "Queries"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\banlist.cpp
# End Source File
# Begin Source File

SOURCE=.\cmdlist.cpp
# End Source File
# Begin Source File

SOURCE=.\mapslist.cpp
# End Source File
# Begin Source File

SOURCE=.\playerlist.cpp
# End Source File
# Begin Source File

SOURCE=.\PlayerListCompare.cpp
# End Source File
# Begin Source File

SOURCE=.\rcon.cpp
# End Source File
# Begin Source File

SOURCE=.\rulesinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\serverinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerList.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerListCompare.cpp
# End Source File
# Begin Source File

SOURCE=.\serverping.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\GamePanelInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\HelpText.cpp
# End Source File
# End Group
# Begin Group "Utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\Info.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\common\msgbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\TrackerNET\NetAddress.cpp
# End Source File
# Begin Source File

SOURCE=..\common\netapi.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Socket.cpp
# End Source File
# Begin Source File

SOURCE=..\..\shared\TokenLine.cpp
# End Source File
# Begin Source File

SOURCE=..\common\util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\UtlBuffer.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\AdminServer.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfigPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\serverpage.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AdminServer.h
# End Source File
# Begin Source File

SOURCE=.\ban.h
# End Source File
# Begin Source File

SOURCE=.\BanContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\banlist.h
# End Source File
# Begin Source File

SOURCE=.\BanPanel.h
# End Source File
# Begin Source File

SOURCE=.\BaseGamesPage.h
# End Source File
# Begin Source File

SOURCE=.\ChatContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\ChatPanel.h
# End Source File
# Begin Source File

SOURCE=.\cmdlist.h
# End Source File
# Begin Source File

SOURCE=.\ConfigPanel.h
# End Source File
# Begin Source File

SOURCE=.\DialogAddBan.h
# End Source File
# Begin Source File

SOURCE=.\DialogAddServer.h
# End Source File
# Begin Source File

SOURCE=.\DialogCvarChange.h
# End Source File
# Begin Source File

SOURCE=.\DialogGameInfo.h
# End Source File
# Begin Source File

SOURCE=.\DialogKickPlayer.h
# End Source File
# Begin Source File

SOURCE=.\DialogServerPassword.h
# End Source File
# Begin Source File

SOURCE=.\FavoriteGames.h
# End Source File
# Begin Source File

SOURCE=.\GamePanelInfo.h
# End Source File
# Begin Source File

SOURCE=.\GraphPanel.h
# End Source File
# Begin Source File

SOURCE=.\HelpText.h
# End Source File
# Begin Source File

SOURCE=..\..\common\AdminServer\IAdminServer.h
# End Source File
# Begin Source File

SOURCE=..\common\IGameList.h
# End Source File
# Begin Source File

SOURCE=..\common\inetapi.h
# End Source File
# Begin Source File

SOURCE=..\..\common\info.h
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.h
# End Source File
# Begin Source File

SOURCE=.\Iresponse.h
# End Source File
# Begin Source File

SOURCE=..\..\common\ServerBrowser\IServerBrowser.h
# End Source File
# Begin Source File

SOURCE=..\common\IServerRefreshResponse.h
# End Source File
# Begin Source File

SOURCE=.\logmsghandler.h
# End Source File
# Begin Source File

SOURCE=.\maps.h
# End Source File
# Begin Source File

SOURCE=.\mapslist.h
# End Source File
# Begin Source File

SOURCE=..\common\MasterMsgHandler.h
# End Source File
# Begin Source File

SOURCE=.\MOTDPanel.h
# End Source File
# Begin Source File

SOURCE=..\common\msgbuffer.h
# End Source File
# Begin Source File

SOURCE=.\Player.h
# End Source File
# Begin Source File

SOURCE=.\PlayerContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\playerlist.h
# End Source File
# Begin Source File

SOURCE=.\PlayerListCompare.h
# End Source File
# Begin Source File

SOURCE=.\playermsghandler.h
# End Source File
# Begin Source File

SOURCE=.\PlayerPanel.h
# End Source File
# Begin Source File

SOURCE=.\point.h
# End Source File
# Begin Source File

SOURCE=.\RawLogPanel.h
# End Source File
# Begin Source File

SOURCE=.\rcon.h
# End Source File
# Begin Source File

SOURCE=.\rconmsghandler.h
# End Source File
# Begin Source File

SOURCE=.\RulesContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\rulesinfo.h
# End Source File
# Begin Source File

SOURCE=.\rulesinfomsghandler.h
# End Source File
# Begin Source File

SOURCE=..\common\server.h
# End Source File
# Begin Source File

SOURCE=.\ServerConfigPanel.h
# End Source File
# Begin Source File

SOURCE=.\ServerContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\serverinfo.h
# End Source File
# Begin Source File

SOURCE=.\serverinfomsghandler.h
# End Source File
# Begin Source File

SOURCE=.\ServerList.h
# End Source File
# Begin Source File

SOURCE=.\ServerListCompare.h
# End Source File
# Begin Source File

SOURCE=.\serverpage.h
# End Source File
# Begin Source File

SOURCE=.\serverping.h
# End Source File
# Begin Source File

SOURCE=.\serverpingmsghandler.h
# End Source File
# Begin Source File

SOURCE=..\common\Socket.h
# End Source File
# Begin Source File

SOURCE=..\..\shared\TokenLine.h
# End Source File
# Begin Source File

SOURCE=..\common\util.h
# End Source File
# Begin Source File

SOURCE=..\..\public\UtlBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\public\utllinkedlist.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\..\vgui2\bin\vgui_controls.lib
# End Source File
# End Target
# End Project
