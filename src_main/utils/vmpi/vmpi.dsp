# Microsoft Developer Studio Project File - Name="vmpi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vmpi - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vmpi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vmpi.mak" CFG="vmpi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vmpi - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "vmpi - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "vmpi"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vmpi - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\public" /I "..\common" /I "zlib" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "PROTECTED_THINGS_DISABLE" /YX /FD /c
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
InputPath=.\Release\vmpi.lib
SOURCE="$(InputPath)"

"..\..\lib\vmpi\vmpi.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	call ..\..\filecopy.bat $(TargetDir)\vmpi.lib ..\..\lib\vmpi\vmpi.lib

# End Custom Build

!ELSEIF  "$(CFG)" == "vmpi - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\public" /I "..\common" /I "zlib" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "PROTECTED_THINGS_DISABLE" /YX /FD /GZ /c
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
InputPath=.\Debug\vmpi.lib
SOURCE="$(InputPath)"

"..\..\lib\vmpi\vmpi.lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	call ..\..\filecopy.bat $(TargetDir)\vmpi.lib ..\..\lib\vmpi\vmpi.lib

# End Custom Build

!ENDIF 

# Begin Target

# Name "vmpi - Win32 Release"
# Name "vmpi - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\public\bitbuf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_tools.cpp
# End Source File
# Begin Source File

SOURCE=.\iphelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\loopback_channel.cpp
# End Source File
# Begin Source File

SOURCE=.\messbuf.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadedTCPSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadedTCPSocketEmu.cpp
# End Source File
# Begin Source File

SOURCE=.\threadhelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\vmpi.cpp
# End Source File
# Begin Source File

SOURCE=.\vmpi_filesystem.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\public\bitbuf.h
# End Source File
# Begin Source File

SOURCE=.\ichannel.h
# End Source File
# Begin Source File

SOURCE=.\iphelpers.h
# End Source File
# Begin Source File

SOURCE=.\IThreadedTCPSocket.h
# End Source File
# Begin Source File

SOURCE=.\loopback_channel.h
# End Source File
# Begin Source File

SOURCE=.\messbuf.h
# End Source File
# Begin Source File

SOURCE=.\tcpsocket.h
# End Source File
# Begin Source File

SOURCE=.\ThreadedTCPSocketEmu.h
# End Source File
# Begin Source File

SOURCE=.\threadhelpers.h
# End Source File
# Begin Source File

SOURCE=.\vmpi.h
# End Source File
# Begin Source File

SOURCE=.\vmpi_defs.h
# End Source File
# Begin Source File

SOURCE=.\vmpi_filesystem.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ZLib.lib
# End Source File
# End Target
# End Project
