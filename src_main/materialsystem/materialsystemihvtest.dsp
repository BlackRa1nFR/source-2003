# Microsoft Developer Studio Project File - Name="MaterialSystemIHVTest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MaterialSystemIHVTest - Win32 IHVTest Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "materialsystemihvtest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "materialsystemihvtest.mak" CFG="MaterialSystemIHVTest - Win32 IHVTest Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MaterialSystemIHVTest - Win32 IHVTest Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MaterialSystemIHVTest - Win32 IHVTest Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/common/MaterialSystem", FEACAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MaterialSystemIHVTest - Win32 IHVTest Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "IHVTestRelease"
# PROP Intermediate_Dir "IHVTestRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MATERIALSYSTEM_EXPORTS" /YX /FD /c
# ADD CPP /nologo /W3 /GR /GX /O2 /I "..\common" /I "..\public" /D "NDEBUG" /D "IHVTEST" /D "DEFINE_MATERIALSYSTEM_INTERFACE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MATERIALSYSTEM_EXPORTS" /FR /FD /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"IHVTestRelease/MaterialSystem.dll" /libpath:".."
# SUBTRACT LINK32 /debug /nodefaultlib
# Begin Custom Build
TargetPath=.\IHVTestRelease\MaterialSystem.dll
InputPath=.\IHVTestRelease\MaterialSystem.dll
SOURCE="$(InputPath)"

"..\..\bin\MaterialSystem.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\bin\MaterialSystem.dll attrib -r ..\..\bin\MaterialSystem.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\bin\MaterialSystem.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "MaterialSystemIHVTest - Win32 IHVTest Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MaterialSystemIHVTest___Win32_Debug"
# PROP BASE Intermediate_Dir "MaterialSystemIHVTest___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "IHVTestDebug"
# PROP Intermediate_Dir "IHVTestDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MATERIALSYSTEM_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /I "..\common" /I "..\public" /D "_DEBUG" /D fopen=dont_use_fopen /D "IHVTEST" /D "DEFINE_MATERIALSYSTEM_INTERFACE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MATERIALSYSTEM_EXPORTS" /FR /FD /GZ /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"IHVTestDebug/MaterialSystem.dll" /pdbtype:sept /libpath:".."
# SUBTRACT LINK32 /nodefaultlib
# Begin Custom Build
TargetPath=.\IHVTestDebug\MaterialSystem.dll
InputPath=.\IHVTestDebug\MaterialSystem.dll
SOURCE="$(InputPath)"

"..\..\bin\MaterialSystem.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\bin\MaterialSystem.dll attrib -r ..\..\bin\MaterialSystem.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\bin\MaterialSystem.dll 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "MaterialSystemIHVTest - Win32 IHVTest Release"
# Name "MaterialSystemIHVTest - Win32 IHVTest Debug"
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Group "Shader Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\basetimesmod2xenvmap.cpp
# End Source File
# Begin Source File

SOURCE=.\debugfbtexture.cpp
# End Source File
# Begin Source File

SOURCE=.\debuglightingonly.cpp
# End Source File
# Begin Source File

SOURCE=.\debugluxel.cpp
# End Source File
# Begin Source File

SOURCE=.\debugmodifyvertex.cpp
# End Source File
# Begin Source File

SOURCE=.\debugnormalmap.cpp
# End Source File
# Begin Source File

SOURCE=.\debugsoftwarevertexshader.cpp
# End Source File
# Begin Source File

SOURCE=.\debugtangentspace.cpp
# End Source File
# Begin Source File

SOURCE=.\unlitgeneric.cpp
# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_dx6.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_dx7.cpp
# End Source File
# Begin Source File

SOURCE=.\wireframe.cpp
# End Source File
# End Group
# Begin Group "pixel shaders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\materials\dxshaders\BaseTimesLightmapWet.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\BumpmappedEnvmap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\BumpmappedLightmap_LightingOnly_OverBright2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\BumpmappedLightmap_OverBright1.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\BumpmappedLightmap_OverBright2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\DebugFBTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\Eyes.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\Eyes_Overbright2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\JellyFish.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightingOnly.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightingOnly_Overbright2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_BaseAlphaMaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_BaseTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_Detail.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_DetailNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_DetailSelfIlluminated.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_EnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_EnvMapNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_EnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_MaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_MaskedEnvMapNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_MaskedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_MultiplyByLighting.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_MultiplyByLightingNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_MultiplyByLightingSelfIllum.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_NoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_SelfIlluminated.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_SelfIlluminatedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_SelfIlluminatedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_SelfIlluminatedMaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\LightmappedGeneric_SelfIlluminatedMaskedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\predator.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\Predator_Envmap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\Predator_EnvmapTimesEnvmapMask.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\ShadowModel.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_BaseAlphaMaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_Detail.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_DetailBaseAlphaMaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_DetailEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_DetailEnvMapMask.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_DetailEnvMapMaskNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_DetailEnvMapNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_DetailNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_EnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_EnvMapMask.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_EnvMapMaskNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_EnvMapNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitGeneric_NoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\UnlitTwoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_BaseAlphaMaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_BaseAlphaMaskedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_Detail.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailBaseAlphaMaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailBaseAlphaMaskedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailMaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailMaskedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailSelfIlluminated.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailSelfIlluminatedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailSelfIlluminatedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailSelfIlluminatedMaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DetailSelfIlluminatedMaskedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DiffBumpLightingOnly_Overbright1.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DiffBumpLightingOnly_Overbright1_Translucent.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DiffBumpLightingOnly_Overbright2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DiffBumpLightingOnly_Overbright2_Translucent.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DiffBumpTimesBase_Overbright1.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DiffBumpTimesBase_Overbright1_Translucent.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DiffBumpTimesBase_Overbright2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_DiffBumpTimesBase_Overbright2_Translucent.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_EnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_EnvMapNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\hl2\materials\dxshaders\VertexLitGeneric_EnvmappedBumpmapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\hl2\materials\dxshaders\VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha.psh
# End Source File
# Begin Source File

SOURCE=..\..\hl2\materials\dxshaders\VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha_ps14.psh
# End Source File
# Begin Source File

SOURCE=..\..\hl2\materials\dxshaders\VertexLitGeneric_EnvmappedBumpmapV2_ps14.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_EnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_MaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_MaskedEnvMapNoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_MaskedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_NoTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_SelfIlluminated.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_SelfIlluminatedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_SelfIlluminatedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_SelfIlluminatedMaskedEnvMap.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_SelfIlluminatedMaskedEnvMapV2.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\VertexLitGeneric_SelfIllumOnly.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\WaterReflect.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\WaterRefract.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\WorldTexture.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\WorldVertexTransition.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\WriteEnvMapMaskToAlphaBuffer.psh
# End Source File
# Begin Source File

SOURCE=..\..\materials\dxshaders\WriteZWithHeightClipPlane.psh
# End Source File
# End Group
# Begin Source File

SOURCE=..\public\characterset.cpp
# End Source File
# Begin Source File

SOURCE=.\cmaterial.cpp
# End Source File
# Begin Source File

SOURCE=.\cmaterialsystem.cpp
# End Source File
# Begin Source File

SOURCE=.\cmaterialvar.cpp
# End Source File
# Begin Source File

SOURCE=.\colorspace.cpp
# End Source File
# Begin Source File

SOURCE=..\public\convar.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\crtmemdebug.cpp
# End Source File
# Begin Source File

SOURCE=.\cshader.cpp
# End Source File
# Begin Source File

SOURCE=.\ctexture.cpp
# End Source File
# Begin Source File

SOURCE=..\public\filesystem_helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\imageloader.cpp
# End Source File
# Begin Source File

SOURCE=.\imagepacker.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\materialsystem_global.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=.\matsys_cvar.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\mempool.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\resource_file.cpp
# End Source File
# Begin Source File

SOURCE=.\shader.cpp
# End Source File
# Begin Source File

SOURCE=.\shadersystem.cpp
# End Source File
# Begin Source File

SOURCE=.\texturemanager.cpp
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

# PROP Default_Filter ""
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

SOURCE=.\ColorQuant.h
# End Source File
# Begin Source File

SOURCE=.\ColorSpace.h
# End Source File
# Begin Source File

SOURCE=..\Public\const.h
# End Source File
# Begin Source File

SOURCE=..\Public\crtmemdebug.h
# End Source File
# Begin Source File

SOURCE=.\CShader.h
# End Source File
# Begin Source File

SOURCE=..\common\cstringhash.h
# End Source File
# Begin Source File

SOURCE=..\public\dbg\dbg.h
# End Source File
# Begin Source File

SOURCE=..\public\platform\fasttimer.h
# End Source File
# Begin Source File

SOURCE=..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\public\appframework\IAppSystem.h
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

SOURCE=.\IShaderUtil.h
# End Source File
# Begin Source File

SOURCE=..\public\materialsystem\itexture.h
# End Source File
# Begin Source File

SOURCE=.\ITextureInternal.h
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

SOURCE=..\Public\MemPool.h
# End Source File
# Begin Source File

SOURCE=..\Public\PixelWriter.h
# End Source File
# Begin Source File

SOURCE=..\public\platform\platform.h
# End Source File
# Begin Source File

SOURCE=..\Public\resource_file.h
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

SOURCE=..\public\UtlBuffer.h
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
# End Target
# End Project
