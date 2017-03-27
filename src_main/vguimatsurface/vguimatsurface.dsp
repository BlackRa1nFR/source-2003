# Microsoft Developer Studio Project File - Name="vguimatsurface" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=VGUIMATSURFACE - WIN32 RELEASE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vguimatsurface.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vguimatsurface.mak" CFG="VGUIMATSURFACE - WIN32 RELEASE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vguimatsurface - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vguimatsurface - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "vguimatsurface"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vguimatsurface - Win32 Release"

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
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\src\common" /I "..\..\src\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "VGUIMATSURFACE_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\vgui2\controls" /I "..\common\materialsystem" /I "..\vgui2\include" /I "..\common" /I "..\public" /D "NDEBUG" /D "BENCHMARK" /D "_USRDLL" /D "_WINDOWS" /D fopen=dont_use_fopen /D "VGUIMATSURFACE_DLL_EXPORT" /D "_WIN32" /D "_MBCS" /D "GAMEUI_EXPORTS" /D "DONT_PROTECT_FILEIO_FUNCTIONS" /D "ENABLE_HTMLWINDOW" /D "PROTECTED_THINGS_ENABLE" /FR /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\src\lib\common\\" /libpath:"..\..\src\lib\public\\"
# ADD LINK32 vgui_surfacelib.lib gdi32.lib user32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib:"LIBCMTD" /nodefaultlib:"LIBCMT" /nodefaultlib:"libcd" /libpath:"..\lib\common\\" /libpath:"..\lib\public\\"
# Begin Custom Build - Publishing to target directory (..\..\bin\)...
TargetDir=.\Release
TargetPath=.\Release\vguimatsurface.dll
InputPath=.\Release\vguimatsurface.dll
SOURCE="$(InputPath)"

"..\..\bin\vguimatsurface.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\bin\vguimatsurface.dll attrib -r ..\..\bin\vguimatsurface.dll 
	copy $(TargetPath) ..\..\bin\vguimatsurface.dll 
	if exist $(TargetDir)\vguimatsurface.map copy $(TargetDir)\vguimatsurface.map ..\..\bin\vguimatsurface.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vguimatsurface - Win32 Debug"

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
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\src\common" /I "..\..\src\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "VGUIMATSURFACE_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /GR /ZI /Od /Op /I "..\vgui2\controls" /I "..\common\materialsystem" /I "..\vgui2\include" /I "..\common" /I "..\public" /D "_DEBUG" /D "BENCHMARK" /D "_USRDLL" /D "_WINDOWS" /D fopen=dont_use_fopen /D "VGUIMATSURFACE_DLL_EXPORT" /D "_WIN32" /D "_MBCS" /D "GAMEUI_EXPORTS" /D "DONT_PROTECT_FILEIO_FUNCTIONS" /D "ENABLE_HTMLWINDOW" /D "PROTECTED_THINGS_ENABLE" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\src\lib\common\\" /libpath:"..\..\src\lib\public\\"
# ADD LINK32 vgui_surfacelib.lib gdi32.lib user32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib:"LIBCMTD" /nodefaultlib:"LIBCMT" /nodefaultlib:"libc" /pdbtype:sept /libpath:"..\lib\common\\" /libpath:"..\lib\public\\"
# Begin Custom Build - Publishing to target directory (..\..\bin\)...
TargetDir=.\Debug
TargetPath=.\Debug\vguimatsurface.dll
InputPath=.\Debug\vguimatsurface.dll
SOURCE="$(InputPath)"

"..\..\bin\vguimatsurface.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\bin\vguimatsurface.dll attrib -r ..\..\bin\vguimatsurface.dll 
	copy $(TargetPath) ..\..\bin\vguimatsurface.dll 
	if exist $(TargetDir)\vguimatsurface.map copy $(TargetDir)\vguimatsurface.map ..\..\bin\vguimatsurface.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "vguimatsurface - Win32 Release"
# Name "vguimatsurface - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\public\characterset.cpp
# End Source File
# Begin Source File

SOURCE=..\public\checksum_crc.cpp
# End Source File
# Begin Source File

SOURCE=.\Clip2D.cpp
# End Source File
# Begin Source File

SOURCE=.\Cursor.cpp
# End Source File
# Begin Source File

SOURCE=..\public\filesystem_helpers.cpp
# End Source File
# Begin Source File

SOURCE=.\FontTextureCache.cpp
# End Source File
# Begin Source File

SOURCE=.\FontTextureCache.h
# End Source File
# Begin Source File

SOURCE=..\vgui2\src\htmlwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\Input.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=.\MatSystemSurface.cpp
# End Source File
# Begin Source File

SOURCE=.\MatSystemSurface.h
# End Source File
# Begin Source File

SOURCE=..\vgui2\src\MemoryBitmap.cpp
# End Source File
# Begin Source File

SOURCE=..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=.\TextureDictionary.cpp
# End Source File
# Begin Source File

SOURCE=..\vgui2\src\vgui_key_translation.cpp
# End Source File
# Begin Source File

SOURCE=..\vgui2\src\vgui_key_translation.h
# End Source File
# Begin Source File

SOURCE=.\vguimatsurfacestats.cpp
# End Source File
# Begin Source File

SOURCE=.\vguimatsurfacestats.h
# End Source File
# Begin Source File

SOURCE=..\Public\vmatrix.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\public\characterset.h
# End Source File
# Begin Source File

SOURCE=..\Public\checksum_crc.h
# End Source File
# Begin Source File

SOURCE=.\Clip2D.h
# End Source File
# Begin Source File

SOURCE=..\public\commonmacros.h
# End Source File
# Begin Source File

SOURCE=.\Cursor.h
# End Source File
# Begin Source File

SOURCE=..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\public\filesystem_helpers.h
# End Source File
# Begin Source File

SOURCE=..\common\vgui_surfacelib\FontAmalgam.h
# End Source File
# Begin Source File

SOURCE=..\common\vgui_surfacelib\FontManager.h
# End Source File
# Begin Source File

SOURCE=..\common\vgui\HtmlWindow.h
# End Source File
# Begin Source File

SOURCE=..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\public\icreatevfont.h
# End Source File
# Begin Source File

SOURCE=..\public\imageloader.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterial.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterialsystem.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterialvar.h
# End Source File
# Begin Source File

SOURCE=..\public\VGuiMatSurface\IMatSystemSurface.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imesh.h
# End Source File
# Begin Source File

SOURCE=.\Input.h
# End Source File
# Begin Source File

SOURCE=..\public\interface.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\itexture.h
# End Source File
# Begin Source File

SOURCE=..\public\pixelwriter.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=.\TextureDictionary.h
# End Source File
# Begin Source File

SOURCE=..\public\utllinkedlist.h
# End Source File
# Begin Source File

SOURCE=..\Public\UtlMemory.h
# End Source File
# Begin Source File

SOURCE=..\Public\utlrbtree.h
# End Source File
# Begin Source File

SOURCE=..\Public\UtlVector.h
# End Source File
# Begin Source File

SOURCE=..\Public\vector.h
# End Source File
# Begin Source File

SOURCE=..\Public\vector2d.h
# End Source File
# Begin Source File

SOURCE=..\Public\vector4d.h
# End Source File
# Begin Source File

SOURCE=.\vguimatsurface.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\vstdlib.h
# End Source File
# Begin Source File

SOURCE=..\public\vtf\vtf.h
# End Source File
# Begin Source File

SOURCE=..\common\vgui_surfacelib\Win32Font.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\vgui_surfacelib.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\vgui_controls.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
