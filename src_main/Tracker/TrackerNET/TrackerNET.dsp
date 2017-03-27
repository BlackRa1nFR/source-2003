# Microsoft Developer Studio Project File - Name="TrackerNET" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TrackerNET - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TrackerNET.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TrackerNET.mak" CFG="TrackerNET - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TrackerNET - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TrackerNET - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GoldSrc/Tracker/TrackerNET", ZPACAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TrackerNET - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERNET_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GR /GX /O2 /I "..\..\public" /I "..\public" /I "..\..\common" /I "..\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERNET_EXPORTS" /YX /FD /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 WS2_32.LIB kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib /nologo /dll /machine:I386
# Begin Custom Build
TargetPath=.\Release\TrackerNET.dll
InputPath=.\Release\TrackerNET.dll
SOURCE="$(InputPath)"

BuildCmds= \
	if exist ..\..\..\Platform\Friends\TrackerNET.dll attrib -r ..\..\..\Platform\Friends\TrackerNET.dll \
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\Platform\Friends\TrackerNET.dll \
	if exist ..\..\..\TrackerServer\TrackerNET.dll attrib -r ..\..\..\TrackerServer\TrackerNET.dll \
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\TrackerServer\TrackerNET.dll \
	

"..\..\..\Platform\Friends\TrackerNET.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\..\..\TrackerServer\TrackerNET.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "TrackerNET - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERNET_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /WX /Gm /GR /GX /ZI /Od /I "..\..\public" /I "..\public" /I "..\..\common" /I "..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TRACKERNET_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib WS2_32.LIB /nologo /dll /map /debug /machine:I386 /pdbtype:sept
# Begin Custom Build
TargetPath=.\Debug\TrackerNET.dll
InputPath=.\Debug\TrackerNET.dll
SOURCE="$(InputPath)"

BuildCmds= \
	if exist ..\..\..\Platform\Friends\TrackerNET.dll attrib -r ..\..\..\Platform\Friends\TrackerNET.dll \
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\Platform\Friends\TrackerNET.dll \
	if exist ..\..\..\TrackerServer\TrackerNET.dll attrib -r ..\..\..\TrackerServer\TrackerNET.dll \
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\TrackerServer\TrackerNET.dll \
	

"..\..\..\Platform\Friends\TrackerNET.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\..\..\TrackerServer\TrackerNET.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# Begin Target

# Name "TrackerNET - Win32 Release"
# Name "TrackerNET - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BinaryBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CompletionEvent.cpp
# End Source File
# Begin Source File

SOURCE=..\common\IceKey.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\MemPool.cpp
# End Source File
# Begin Source File

SOURCE=.\NetAddress.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\Random.cpp
# End Source File
# Begin Source File

SOURCE=.\ReceiveMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\SendMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\Sockets.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadedSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\threads.cpp
# End Source File
# Begin Source File

SOURCE=.\TrackerNET.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BinaryBuffer.h
# End Source File
# Begin Source File

SOURCE=.\BufferAllocator.h
# End Source File
# Begin Source File

SOURCE=..\common\IceKey.H
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.h
# End Source File
# Begin Source File

SOURCE=..\..\common\MemPool.h
# End Source File
# Begin Source File

SOURCE=.\NetAddress.h
# End Source File
# Begin Source File

SOURCE=.\PacketHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\common\Random.h
# End Source File
# Begin Source File

SOURCE=.\ReceiveMessage.h
# End Source File
# Begin Source File

SOURCE=.\SendMessage.h
# End Source File
# Begin Source File

SOURCE=.\Sockets.h
# End Source File
# Begin Source File

SOURCE=.\ThreadedSocket.h
# End Source File
# Begin Source File

SOURCE=.\threads.h
# End Source File
# Begin Source File

SOURCE=.\TrackerNET_Interface.h
# End Source File
# Begin Source File

SOURCE=..\common\winlite.h
# End Source File
# End Group
# End Target
# End Project
