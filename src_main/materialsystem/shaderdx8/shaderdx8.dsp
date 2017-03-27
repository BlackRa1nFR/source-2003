# Microsoft Developer Studio Project File - Name="shaderdx8" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=shaderdx8 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "shaderdx8.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "shaderdx8.mak" CFG="shaderdx8 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "shaderdx8 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "shaderdx8 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/common/MaterialSystem/shaderdx8", VJRCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "shaderdx8 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "shaderdx8___Win32_Release"
# PROP BASE Intermediate_Dir "shaderdx8___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "shaderdx8___Win32_Release"
# PROP Intermediate_Dir "shaderdx8___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /O2 /Ob2 /I "..\..\common" /I "..\..\public" /I "..\..\dx9sdk\include" /I "..\\" /D "NDEBUG" /D "SHADERAPIDX9" /D "_WIN32" /D "IMAGE_LOADER_NO_DXTC" /D "WIN32" /D "_MBCS" /D "_USRDLL" /D "SHADER_DLL_EXPORT" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WINDOWS" /Fr /YX /FD /c
# ADD CPP /nologo /G6 /W4 /O2 /Ob2 /I "..\..\common" /I "..\..\public" /I "..\..\dx9sdk\include" /I "..\\" /D "NDEBUG" /D "SHADERAPIDX9" /D "_WIN32" /D "IMAGE_LOADER_NO_DXTC" /D "WIN32" /D "_MBCS" /D "_USRDLL" /D "SHADER_DLL_EXPORT" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WINDOWS" /D "PROTECTED_THINGS_ENABLE" /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x2A000000" /dll /machine:I386 /out:"shaderdx8___Win32_Release_DX90/shaderapidx8.dll" /libpath:"..\..\..\lib\common" /libpath:"..\..\..\lib\public"
# SUBTRACT BASE LINK32 /profile /map /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x2A000000" /dll /machine:I386 /out:"shaderdx8___Win32_Release_DX90/shaderapidx8.dll" /libpath:"..\..\..\lib\common" /libpath:"..\..\..\lib\public"
# SUBTRACT LINK32 /profile /map /debug
# Begin Custom Build
TargetPath=.\shaderdx8___Win32_Release_DX90\shaderapidx8.dll
InputPath=.\shaderdx8___Win32_Release_DX90\shaderapidx8.dll
SOURCE="$(InputPath)"

"..\..\..\bin\shaderapidx9.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\shaderapidx9.dll attrib -r ..\..\..\bin\shaderapidx9.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\bin\shaderapidx9.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "shaderdx8___Win32_Debug"
# PROP BASE Intermediate_Dir "shaderdx8___Win32_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "shaderdx8___Win32_Debug"
# PROP Intermediate_Dir "shaderdx8___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /I "..\..\common" /I "..\..\public" /I "..\..\dx9sdk\include" /I "..\\" /D "_DEBUG" /D "SHADERAPIDX9" /D "_WIN32" /D "IMAGE_LOADER_NO_DXTC" /D "WIN32" /D "_MBCS" /D "_USRDLL" /D "SHADER_DLL_EXPORT" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WINDOWS" /FR /YX /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /I "..\..\common" /I "..\..\public" /I "..\..\dx9sdk\include" /I "..\\" /D "_DEBUG" /D "SHADERAPIDX9" /D "_WIN32" /D "IMAGE_LOADER_NO_DXTC" /D "WIN32" /D "_MBCS" /D "_USRDLL" /D "SHADER_DLL_EXPORT" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WINDOWS" /D "PROTECTED_THINGS_ENABLE" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x2A000000" /dll /debug /machine:I386 /out:"shaderdx8___Win32_Debug_DX90/shaderapidx8.dll" /pdbtype:sept /libpath:"..\..\..\lib\common" /libpath:"..\..\..\lib\public"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x2A000000" /dll /debug /machine:I386 /out:"shaderdx8___Win32_Debug_DX90/shaderapidx8.dll" /pdbtype:sept /libpath:"..\..\..\lib\common" /libpath:"..\..\..\lib\public"
# Begin Custom Build
TargetPath=.\shaderdx8___Win32_Debug_DX90\shaderapidx8.dll
InputPath=.\shaderdx8___Win32_Debug_DX90\shaderapidx8.dll
SOURCE="$(InputPath)"

"..\..\..\bin\shaderapidx9.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\shaderapidx9.dll attrib -r ..\..\..\bin\shaderapidx9.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\bin\shaderapidx9.dll 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "shaderdx8 - Win32 Release"
# Name "shaderdx8 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\public\characterset.cpp
# End Source File
# Begin Source File

SOURCE=.\CMaterialSystemStats.cpp
# End Source File
# Begin Source File

SOURCE=.\ColorFormatDX8.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\imageloader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\KeyValues.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\mempool.cpp
# End Source File
# Begin Source File

SOURCE=.\MeshDX8.cpp
# End Source File
# Begin Source File

SOURCE=.\Recording.cpp
# End Source File
# Begin Source File

SOURCE=.\ShaderAPIDX8.cpp
# End Source File
# Begin Source File

SOURCE=.\ShaderShadowDX8.cpp
# End Source File
# Begin Source File

SOURCE=.\TextureDX8.cpp
# End Source File
# Begin Source File

SOURCE=.\TransitionTable.cpp
# End Source File
# Begin Source File

SOURCE=.\TransitionTable.h
# End Source File
# Begin Source File

SOURCE=..\..\public\utlbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\utlsymbol.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexdecl.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexdecl.h
# End Source File
# Begin Source File

SOURCE=.\VertexShaderDX8.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CMaterialSystemStats.h
# End Source File
# Begin Source File

SOURCE=.\ColorFormatDX8.h
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3d8.h
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3d8caps.h
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3d8types.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3d9.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3d9caps.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3d9types.h
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3dx8.h
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3dx8core.h
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3dx8effect.h
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3dx8math.h
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3dx8math.inl
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3dx8mesh.h
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3dx8shape.h
# End Source File
# Begin Source File

SOURCE=..\..\dx8sdk\include\d3dx8tex.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3dx9.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3dx9anim.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3dx9core.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3dx9effect.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3dx9math.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3dx9math.inl
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3dx9mesh.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3dx9shader.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3dx9shape.h
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\include\d3dx9tex.h
# End Source File
# Begin Source File

SOURCE=.\dynamicib.h
# End Source File
# Begin Source File

SOURCE=.\dynamicvb.h
# End Source File
# Begin Source File

SOURCE=.\IMeshDX8.h
# End Source File
# Begin Source File

SOURCE=.\locald3dtypes.h
# End Source File
# Begin Source File

SOURCE=.\Recording.h
# End Source File
# Begin Source File

SOURCE=.\ShaderAPIDX8.h
# End Source File
# Begin Source File

SOURCE=.\ShaderAPIDX8_Global.h
# End Source File
# Begin Source File

SOURCE=.\ShaderShadowDX8.h
# End Source File
# Begin Source File

SOURCE=.\stubd3ddevice.h
# End Source File
# Begin Source File

SOURCE=.\TextureDX8.h
# End Source File
# Begin Source File

SOURCE=.\VertexShaderDX8.h
# End Source File
# Begin Source File

SOURCE=..\..\utils\scenemanager\workspace.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\lib\d3dx9.lib
# End Source File
# Begin Source File

SOURCE=..\..\dx9sdk\lib\d3d9.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
