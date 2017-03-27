# Microsoft Developer Studio Project File - Name="ServerBrowser" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ServerBrowser - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ServerBrowser.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ServerBrowser.mak" CFG="ServerBrowser - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ServerBrowser - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ServerBrowser - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GoldSrc/Tracker/ServerBrowser", FUODAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ServerBrowser - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SERVERBROWSER_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GR /GX /O2 /I "..\public" /I "..\..\public" /I "..\common" /I "..\..\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SERVERBROWSER_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 wsock32.lib /nologo /dll /machine:I386 /nodefaultlib:"LIBCMTD" /nodefaultlib:"LIBC" /nodefaultlib:"LIBCD"
# Begin Custom Build
TargetPath=.\Release\ServerBrowser.dll
InputPath=.\Release\ServerBrowser.dll
SOURCE="$(InputPath)"

"..\..\..\bin\ServerBrowser.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\ServerBrowser.dll attrib -r ..\..\..\bin\ServerBrowser.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\bin\ServerBrowser.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ServerBrowser - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SERVERBROWSER_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /GR /GX /ZI /Od /I "..\public" /I "..\..\public" /I "..\common" /I "..\..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SERVERBROWSER_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib /nologo /dll /map /debug /machine:I386 /nodefaultlib:"LIBCMT" /nodefaultlib:"LIBCD" /pdbtype:sept
# Begin Custom Build
TargetPath=.\Debug\ServerBrowser.dll
InputPath=.\Debug\ServerBrowser.dll
SOURCE="$(InputPath)"

"..\..\..\Platform\Servers\ServerBrowser.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\Platform\Servers\ServerBrowser.dll attrib -r ..\..\..\Platform\Servers\ServerBrowser.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\Platform\Servers\ServerBrowser.dll 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "ServerBrowser - Win32 Release"
# Name "ServerBrowser - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BaseGamesPage.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogAddServer.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogGameInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogServerPassword.cpp
# End Source File
# Begin Source File

SOURCE=.\FavoriteGames.cpp
# End Source File
# Begin Source File

SOURCE=.\FriendsGames.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\info_key.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\InternetGames.cpp
# End Source File
# Begin Source File

SOURCE=.\LanBroadcastMsgHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\LanGames.cpp
# End Source File
# Begin Source File

SOURCE=.\MasterMsgHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=.\ModList.cpp
# End Source File
# Begin Source File

SOURCE=..\common\msgbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\common\netapi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\GameUI\Random.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerBrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerBrowserDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerList.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerListCompare.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerMsgHandlerDetails.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Socket.cpp
# End Source File
# Begin Source File

SOURCE=.\SpectateGames.cpp
# End Source File
# Begin Source File

SOURCE=..\common\util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\UtlBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\utlsymbol.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BaseGamesPage.h
# End Source File
# Begin Source File

SOURCE=.\DialogAddServer.h
# End Source File
# Begin Source File

SOURCE=.\DialogGameInfo.h
# End Source File
# Begin Source File

SOURCE=.\DialogServerPassword.h
# End Source File
# Begin Source File

SOURCE=.\FavoriteGames.h
# End Source File
# Begin Source File

SOURCE=.\FriendsGames.h
# End Source File
# Begin Source File

SOURCE=..\common\IGameList.h
# End Source File
# Begin Source File

SOURCE=..\common\inetapi.h
# End Source File
# Begin Source File

SOURCE=..\..\common\info_key.h
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.h
# End Source File
# Begin Source File

SOURCE=.\InternetGames.h
# End Source File
# Begin Source File

SOURCE=..\..\common\ServerBrowser\IServerBrowser.h
# End Source File
# Begin Source File

SOURCE=..\common\IServerRefreshResponse.h
# End Source File
# Begin Source File

SOURCE=.\LanBroadcastMsgHandler.h
# End Source File
# Begin Source File

SOURCE=.\LanGames.h
# End Source File
# Begin Source File

SOURCE=..\common\MasterMsgHandler.h
# End Source File
# Begin Source File

SOURCE=.\ModList.h
# End Source File
# Begin Source File

SOURCE=..\common\msgbuffer.h
# End Source File
# Begin Source File

SOURCE=..\common\server.h
# End Source File
# Begin Source File

SOURCE=.\ServerBrowser.h
# End Source File
# Begin Source File

SOURCE=.\ServerBrowserDialog.h
# End Source File
# Begin Source File

SOURCE=.\ServerContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\ServerList.h
# End Source File
# Begin Source File

SOURCE=.\ServerListCompare.h
# End Source File
# Begin Source File

SOURCE=.\ServerMsgHandlerDetails.h
# End Source File
# Begin Source File

SOURCE=..\common\Socket.h
# End Source File
# Begin Source File

SOURCE=.\SpectateGames.h
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
