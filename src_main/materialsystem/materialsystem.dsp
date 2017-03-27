# Microsoft Developer Studio Project File - Name="MaterialSystem" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MATERIALSYSTEM - WIN32 RELEASE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "materialsystem.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "materialsystem.mak" CFG="MATERIALSYSTEM - WIN32 RELEASE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MaterialSystem - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MaterialSystem - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/common/MaterialSystem", FEACAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MaterialSystem - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MATERIALSYSTEM_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /O2 /Ob2 /I "..\common" /I "..\public" /D "NDEBUG" /D "_WIN32" /D "DEFINE_MATERIALSYSTEM_INTERFACE" /D "WIN32" /D "_MBCS" /D "_USRDLL" /D "MATERIALSYSTEM_EXPORTS" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WINDOWS" /D "PROTECTED_THINGS_ENABLE" /Fr /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"libcd.lib" /libpath:"..\..\lib\public" /libpath:"..\..\lib\common"
# SUBTRACT LINK32 /profile /map
# Begin Custom Build
TargetDir=.\Release
TargetPath=.\Release\materialsystem.dll
InputPath=.\Release\materialsystem.dll
SOURCE="$(InputPath)"

"..\..\bin\MaterialSystem.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\bin\MaterialSystem.dll attrib -r ..\..\bin\MaterialSystem.dll 
	if exist ..\..\bin\MaterialSystem.map attrib -r ..\..\bin\MaterialSystem.map 
	if exist $(TargetPath) copy $(TargetPath) ..\..\bin\MaterialSystem.dll 
	if exist $(TargetDir)\MaterialSystem.map copy $(TargetDir)\MaterialSystem.map ..\..\bin\MaterialSystem.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "MaterialSystem - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MaterialSystem___Win32_Debug"
# PROP BASE Intermediate_Dir "MaterialSystem___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MATERIALSYSTEM_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /I "..\common" /I "..\public" /D fopen=dont_use_fopen /D "_DEBUG" /D "_WIN32" /D "DEFINE_MATERIALSYSTEM_INTERFACE" /D "WIN32" /D "_MBCS" /D "_USRDLL" /D "MATERIALSYSTEM_EXPORTS" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /D "_WINDOWS" /D "PROTECTED_THINGS_ENABLE" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x28000000" /dll /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /nodefaultlib:"libc.lib" /pdbtype:sept /libpath:"..\..\lib\public" /libpath:"..\..\lib\common"
# SUBTRACT LINK32 /nodefaultlib
# Begin Custom Build
TargetDir=.\Debug
TargetPath=.\Debug\materialsystem.dll
InputPath=.\Debug\materialsystem.dll
SOURCE="$(InputPath)"

"..\..\bin\MaterialSystem.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\bin\MaterialSystem.dll attrib -r ..\..\bin\MaterialSystem.dll 
	if exist ..\..\bin\MaterialSystem.map attrib -r ..\..\bin\MaterialSystem.map 
	if exist $(TargetPath) copy $(TargetPath) ..\..\bin\MaterialSystem.dll 
	if exist $(TargetDir)\MaterialSystem.map copy $(TargetDir)\MaterialSystem.map ..\..\bin\MaterialSystem.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "MaterialSystem - Win32 Release"
# Name "MaterialSystem - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\public\characterset.cpp
# End Source File
# Begin Source File

SOURCE=.\CMaterial.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\CMaterialSystem.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\CMaterialVar.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ColorSpace.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\CTexture.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\public\filesystem_helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\imageloader.cpp
# End Source File
# Begin Source File

SOURCE=.\ImagePacker.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\public\KeyValues.cpp
# End Source File
# Begin Source File

SOURCE=.\MaterialSystem.rc
# End Source File
# Begin Source File

SOURCE=.\MaterialSystem_Global.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=.\matrendertexture.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\mempool.cpp
# End Source File
# Begin Source File

SOURCE=.\ShaderSystem.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\TextureManager.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\tgaloader.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\tgawriter.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\utlbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\utlsymbol.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\vmatrix.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Public\amd3dx.h
# End Source File
# Begin Source File

SOURCE=..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\Public\bumpvects.h
# End Source File
# Begin Source File

SOURCE=.\ColorSpace.h
# End Source File
# Begin Source File

SOURCE=..\Public\const.h
# End Source File
# Begin Source File

SOURCE=..\public\convar.h
# End Source File
# Begin Source File

SOURCE=..\Public\crtmemdebug.h
# End Source File
# Begin Source File

SOURCE=..\common\cstringhash.h
# End Source File
# Begin Source File

SOURCE=..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=.\IHardwareConfigInternal.h
# End Source File
# Begin Source File

SOURCE=..\Public\ImageLoader.h
# End Source File
# Begin Source File

SOURCE=.\ImagePacker.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterial.h
# End Source File
# Begin Source File

SOURCE=.\IMaterialInternal.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterialproxy.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterialproxyfactory.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterialsystem.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterialsystemhardwareconfig.h
# End Source File
# Begin Source File

SOURCE=.\IMaterialSystemInternal.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imaterialvar.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\imesh.h
# End Source File
# Begin Source File

SOURCE=..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\IShader.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\ishaderapi.h
# End Source File
# Begin Source File

SOURCE=..\common\materialsystem\IShaderSystem.h
# End Source File
# Begin Source File

SOURCE=.\IShaderUtil.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\itexture.h
# End Source File
# Begin Source File

SOURCE=.\ITextureInternal.h
# End Source File
# Begin Source File

SOURCE=..\public\KeyValues.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\materialsystem_config.h
# End Source File
# Begin Source File

SOURCE=.\MaterialSystem_Global.h
# End Source File
# Begin Source File

SOURCE=..\Public\MATHLIB.H
# End Source File
# Begin Source File

SOURCE=.\matrendertexture.h
# End Source File
# Begin Source File

SOURCE=..\Public\MemPool.h
# End Source File
# Begin Source File

SOURCE=..\Public\PixelWriter.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\Public\s3_intrf.h
# End Source File
# Begin Source File

SOURCE=.\ShaderAPI.h
# End Source File
# Begin Source File

SOURCE=.\ShaderSystem.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=.\TextureManager.h
# End Source File
# Begin Source File

SOURCE=..\Public\TGALoader.h
# End Source File
# Begin Source File

SOURCE=..\Public\TGAWriter.h
# End Source File
# Begin Source File

SOURCE=..\Public\UtlBuffer.h
# End Source File
# Begin Source File

SOURCE=..\Public\UtlMemory.h
# End Source File
# Begin Source File

SOURCE=..\Public\utlrbtree.h
# End Source File
# Begin Source File

SOURCE=..\Public\utlsymbol.h
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

SOURCE=..\Public\vmatrix.h
# End Source File
# Begin Source File

SOURCE=..\Public\vplane.h
# End Source File
# Begin Source File

SOURCE=..\public\vstdlib\vstdlib.h
# End Source File
# Begin Source File

SOURCE=..\public\vtf\vtf.h
# End Source File
# End Group
# Begin Group "Shader Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Wireframe.cpp

!IF  "$(CFG)" == "MaterialSystem - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "MaterialSystem - Win32 Debug"

# ADD CPP /MD
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\lib\public\vtf.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\lib\common\s3tc.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\tier0.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\shaderlib.lib
# End Source File
# End Target
# End Project
