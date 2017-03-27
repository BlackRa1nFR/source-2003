# Microsoft Developer Studio Project File - Name="stdshader_dx8" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=stdshader_dx8 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "stdshader_dx8.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "stdshader_dx8.mak" CFG="stdshader_dx8 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "stdshader_dx8 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "stdshader_dx8 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "stdshader_dx8"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_dx8"
# PROP BASE Intermediate_Dir "Release_dx8"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_dx8"
# PROP Intermediate_Dir "Release_dx8"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX8_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX8_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Release_dx8
TargetPath=.\Release_dx8\stdshader_dx8.dll
InputPath=.\Release_dx8\stdshader_dx8.dll
SOURCE="$(InputPath)"

"..\..\..\bin\stdshader_dx8.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\stdshader_dx8.dll attrib -r ..\..\..\bin\stdshader_dx8.dll 
	copy $(TargetPath) ..\..\..\bin\stdshader_dx8.dll 
	if exist $(TargetDir)\stdshader_dx8.map copy $(TargetDir)\stdshader_dx8.map ..\..\..\bin\stdshader_dx8.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_dx8"
# PROP BASE Intermediate_Dir "Debug_dx8"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_dx8"
# PROP Intermediate_Dir "Debug_dx8"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX8_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX8_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Debug_dx8
TargetPath=.\Debug_dx8\stdshader_dx8.dll
InputPath=.\Debug_dx8\stdshader_dx8.dll
SOURCE="$(InputPath)"

"..\..\..\bin\stdshader_dx8.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\stdshader_dx8.dll attrib -r ..\..\..\bin\stdshader_dx8.dll 
	copy $(TargetPath) ..\..\..\bin\stdshader_dx8.dll 
	if exist $(TargetDir)\stdshader_dx8.map copy $(TargetDir)\stdshader_dx8.map ..\..\..\bin\stdshader_dx8.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "stdshader_dx8 - Win32 Release"
# Name "stdshader_dx8 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\alienscale_dx8.cpp
# End Source File
# Begin Source File

SOURCE=.\basetimeslightmapwet.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseVSShader.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseVSShader.h
# End Source File
# Begin Source File

SOURCE=..\..\public\bumpvects.cpp
# End Source File
# Begin Source File

SOURCE=.\cable.cpp
# End Source File
# Begin Source File

SOURCE=.\eyes.cpp
# End Source File
# Begin Source File

SOURCE=.\gooinglass.cpp
# End Source File
# Begin Source File

SOURCE=.\jellyfish.cpp
# End Source File
# Begin Source File

SOURCE=.\jojirium.cpp
# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_dx8.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=.\modulate.cpp
# End Source File
# Begin Source File

SOURCE=.\overlay_fit.cpp
# End Source File
# Begin Source File

SOURCE=.\particlesphere.cpp
# End Source File
# Begin Source File

SOURCE=.\predator.cpp
# End Source File
# Begin Source File

SOURCE=.\refract_dx80.cpp
# End Source File
# Begin Source File

SOURCE=.\shadow_dx8.cpp
# End Source File
# Begin Source File

SOURCE=.\shadowbuild.cpp
# End Source File
# Begin Source File

SOURCE=.\shadowmodel.cpp
# End Source File
# Begin Source File

SOURCE=.\sprite.cpp
# End Source File
# Begin Source File

SOURCE=.\teeth_dx8.cpp
# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_dx8.cpp
# End Source File
# Begin Source File

SOURCE=.\unlittwotexture.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_dx8.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\vmatrix.cpp
# End Source File
# Begin Source File

SOURCE=.\water_dx80.cpp
# End Source File
# Begin Source File

SOURCE=.\water_dx81.cpp
# End Source File
# Begin Source File

SOURCE=.\worldvertexalpha_dx8.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldVertexTransition_dx8.cpp
# End Source File
# Begin Source File

SOURCE=.\writez.cpp
# End Source File
# Begin Source File

SOURCE=.\yuv.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\lightmappedgeneric_inc.vsh
# End Source File
# Begin Source File

SOURCE=.\macros.vsh
# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_inc.vsh
# End Source File
# Begin Source File

SOURCE=.\unlittwotexture_inc.vsh
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_inc.vsh
# End Source File
# End Group
# Begin Group "Shaders"
# Begin Source File

SOURCE=.\BaseTimesLightmapWet.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__0=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\BaseTimesLightmapWet.vsh
InputName=BaseTimesLightmapWet

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__0=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\BaseTimesLightmapWet.vsh
InputName=BaseTimesLightmapWet

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\BumpmappedEnvmap.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__1=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedEnvmap.vsh
InputName=BumpmappedEnvmap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__1=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedEnvmap.vsh
InputName=BumpmappedEnvmap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Cable.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__2=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Cable.vsh
InputName=Cable

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__2=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Cable.vsh
InputName=Cable

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Eyes.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__3=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Eyes.vsh
InputName=Eyes

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__3=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Eyes.vsh
InputName=Eyes

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\glowwarp.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__4=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\glowwarp.vsh
InputName=glowwarp

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__4=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\glowwarp.vsh
InputName=glowwarp

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\HotSpotGlow.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__5=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\HotSpotGlow.vsh
InputName=HotSpotGlow

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__5=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\HotSpotGlow.vsh
InputName=HotSpotGlow

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\JellyFish.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__6=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\JellyFish.vsh
InputName=JellyFish

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__6=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\JellyFish.vsh
InputName=JellyFish

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Jojirium.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__7=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Jojirium.vsh
InputName=Jojirium

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__7=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Jojirium.vsh
InputName=Jojirium

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__8=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric.vsh
InputName=LightmappedGeneric

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__8=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric.vsh
InputName=LightmappedGeneric

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_BaseTexture.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__9=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BaseTexture.vsh
InputName=LightmappedGeneric_BaseTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__9=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BaseTexture.vsh
InputName=LightmappedGeneric_BaseTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_BumpmappedEnvmap.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__10=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedEnvmap.vsh
InputName=LightmappedGeneric_BumpmappedEnvmap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__10=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedEnvmap.vsh
InputName=LightmappedGeneric_BumpmappedEnvmap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_BumpmappedEnvmap_ps14.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__11=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedEnvmap_ps14.vsh
InputName=LightmappedGeneric_BumpmappedEnvmap_ps14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__11=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedEnvmap_ps14.vsh
InputName=LightmappedGeneric_BumpmappedEnvmap_ps14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_BumpmappedLightmap.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__12=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedLightmap.vsh
InputName=LightmappedGeneric_BumpmappedLightmap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__12=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedLightmap.vsh
InputName=LightmappedGeneric_BumpmappedLightmap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_Detail.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__13=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_Detail.vsh
InputName=LightmappedGeneric_Detail

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__13=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_Detail.vsh
InputName=LightmappedGeneric_Detail

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_DetailVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__14=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_DetailVertexColor.vsh
InputName=LightmappedGeneric_DetailVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__14=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_DetailVertexColor.vsh
InputName=LightmappedGeneric_DetailVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_EnvMap.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__15=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMap.vsh
InputName=LightmappedGeneric_EnvMap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__15=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMap.vsh
InputName=LightmappedGeneric_EnvMap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_EnvMapCameraSpace.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__16=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapCameraSpace.vsh
InputName=LightmappedGeneric_EnvMapCameraSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__16=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapCameraSpace.vsh
InputName=LightmappedGeneric_EnvMapCameraSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_EnvMapCameraSpaceVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__17=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapCameraSpaceVertexColor.vsh
InputName=LightmappedGeneric_EnvMapCameraSpaceVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__17=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapCameraSpaceVertexColor.vsh
InputName=LightmappedGeneric_EnvMapCameraSpaceVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_EnvMapSphere.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__18=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapSphere.vsh
InputName=LightmappedGeneric_EnvMapSphere

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__18=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapSphere.vsh
InputName=LightmappedGeneric_EnvMapSphere

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_EnvMapSphereVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__19=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapSphereVertexColor.vsh
InputName=LightmappedGeneric_EnvMapSphereVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__19=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapSphereVertexColor.vsh
InputName=LightmappedGeneric_EnvMapSphereVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_EnvMapVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__20=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapVertexColor.vsh
InputName=LightmappedGeneric_EnvMapVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__20=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapVertexColor.vsh
InputName=LightmappedGeneric_EnvMapVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_VertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__21=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_VertexColor.vsh
InputName=LightmappedGeneric_VertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__21=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_VertexColor.vsh
InputName=LightmappedGeneric_VertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_BaseTextureBlend.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__22=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BaseTextureBlend.vsh
InputName=LightmappedGeneric_BaseTextureBlend

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__22=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BaseTextureBlend.vsh
InputName=LightmappedGeneric_BaseTextureBlend

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedTranslucentTexture.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__23=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedTranslucentTexture.vsh
InputName=LightmappedTranslucentTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__23=	"LightmappedGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedTranslucentTexture.vsh
InputName=LightmappedTranslucentTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\ParticleSphere.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__24=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\ParticleSphere.vsh
InputName=ParticleSphere

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__24=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\ParticleSphere.vsh
InputName=ParticleSphere

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\predator.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__25=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\predator.vsh
InputName=predator

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__25=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\predator.vsh
InputName=predator

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\ScreenSpaceEffect.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__26=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\ScreenSpaceEffect.vsh
InputName=ScreenSpaceEffect

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__26=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\ScreenSpaceEffect.vsh
InputName=ScreenSpaceEffect

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\shadow_vs14.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__27=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\shadow_vs14.vsh
InputName=shadow_vs14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__27=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\shadow_vs14.vsh
InputName=shadow_vs14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\shadow_ps14.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__28=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\shadow_ps14.psh
InputName=shadow_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__28=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\shadow_ps14.psh
InputName=shadow_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\ShadowModel.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__29=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\ShadowModel.vsh
InputName=ShadowModel

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__29=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\ShadowModel.vsh
InputName=ShadowModel

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Teeth.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__30=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Teeth.vsh
InputName=Teeth

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__30=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Teeth.vsh
InputName=Teeth

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__31=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric.vsh
InputName=UnlitGeneric

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__31=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric.vsh
InputName=UnlitGeneric

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_Detail.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__32=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_Detail.vsh
InputName=UnlitGeneric_Detail

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__32=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_Detail.vsh
InputName=UnlitGeneric_Detail

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailEnvMap.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__33=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMap.vsh
InputName=UnlitGeneric_DetailEnvMap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__33=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMap.vsh
InputName=UnlitGeneric_DetailEnvMap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailEnvMapCameraSpace.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__34=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapCameraSpace.vsh
InputName=UnlitGeneric_DetailEnvMapCameraSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__34=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapCameraSpace.vsh
InputName=UnlitGeneric_DetailEnvMapCameraSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailEnvMapCameraSpaceVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__35=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapCameraSpaceVertexColor.vsh
InputName=UnlitGeneric_DetailEnvMapCameraSpaceVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__35=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapCameraSpaceVertexColor.vsh
InputName=UnlitGeneric_DetailEnvMapCameraSpaceVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailEnvMapSphere.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__36=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapSphere.vsh
InputName=UnlitGeneric_DetailEnvMapSphere

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__36=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapSphere.vsh
InputName=UnlitGeneric_DetailEnvMapSphere

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailEnvMapSphereVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__37=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapSphereVertexColor.vsh
InputName=UnlitGeneric_DetailEnvMapSphereVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__37=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapSphereVertexColor.vsh
InputName=UnlitGeneric_DetailEnvMapSphereVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailEnvMapVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__38=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapVertexColor.vsh
InputName=UnlitGeneric_DetailEnvMapVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__38=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapVertexColor.vsh
InputName=UnlitGeneric_DetailEnvMapVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__39=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailVertexColor.vsh
InputName=UnlitGeneric_DetailVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__39=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailVertexColor.vsh
InputName=UnlitGeneric_DetailVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_EnvMap.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__40=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMap.vsh
InputName=UnlitGeneric_EnvMap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__40=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMap.vsh
InputName=UnlitGeneric_EnvMap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_EnvMapCameraSpace.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__41=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapCameraSpace.vsh
InputName=UnlitGeneric_EnvMapCameraSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__41=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapCameraSpace.vsh
InputName=UnlitGeneric_EnvMapCameraSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_EnvMapCameraSpaceVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__42=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapCameraSpaceVertexColor.vsh
InputName=UnlitGeneric_EnvMapCameraSpaceVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__42=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapCameraSpaceVertexColor.vsh
InputName=UnlitGeneric_EnvMapCameraSpaceVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_EnvMapSphere.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__43=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapSphere.vsh
InputName=UnlitGeneric_EnvMapSphere

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__43=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapSphere.vsh
InputName=UnlitGeneric_EnvMapSphere

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_EnvMapSphereVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__44=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapSphereVertexColor.vsh
InputName=UnlitGeneric_EnvMapSphereVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__44=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapSphereVertexColor.vsh
InputName=UnlitGeneric_EnvMapSphereVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_EnvMapVertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__45=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapVertexColor.vsh
InputName=UnlitGeneric_EnvMapVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__45=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapVertexColor.vsh
InputName=UnlitGeneric_EnvMapVertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_LightingOnlyWorld.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__46=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_LightingOnlyWorld.vsh
InputName=UnlitGeneric_LightingOnlyWorld

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__46=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_LightingOnlyWorld.vsh
InputName=UnlitGeneric_LightingOnlyWorld

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_VertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__47=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_VertexColor.vsh
InputName=UnlitGeneric_VertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__47=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_VertexColor.vsh
InputName=UnlitGeneric_VertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitTwoTexture.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__48=	"macros.vsh"	"unlittwotexture_inc.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitTwoTexture.vsh
InputName=UnlitTwoTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__48=	"macros.vsh"	"unlittwotexture_inc.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitTwoTexture.vsh
InputName=UnlitTwoTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitTwoTexture_VertexColor.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__49=	"macros.vsh"	"unlittwotexture_inc.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitTwoTexture_VertexColor.vsh
InputName=UnlitTwoTexture_VertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__49=	"macros.vsh"	"unlittwotexture_inc.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitTwoTexture_VertexColor.vsh
InputName=UnlitTwoTexture_VertexColor

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitEnvMappedTexture.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__50=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitEnvMappedTexture.vsh
InputName=VertexLitEnvMappedTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__50=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitEnvMappedTexture.vsh
InputName=VertexLitEnvMappedTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitEnvMappedTexture_CameraSpace.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__51=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitEnvMappedTexture_CameraSpace.vsh
InputName=VertexLitEnvMappedTexture_CameraSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__51=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitEnvMappedTexture_CameraSpace.vsh
InputName=VertexLitEnvMappedTexture_CameraSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitEnvMappedTexture_SphereMap.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__52=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitEnvMappedTexture_SphereMap.vsh
InputName=VertexLitEnvMappedTexture_SphereMap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__52=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitEnvMappedTexture_SphereMap.vsh
InputName=VertexLitEnvMappedTexture_SphereMap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_Detail.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__53=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_Detail.vsh
InputName=VertexLitGeneric_Detail

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__53=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_Detail.vsh
InputName=VertexLitGeneric_Detail

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DetailEnvMap.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__54=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailEnvMap.vsh
InputName=VertexLitGeneric_DetailEnvMap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__54=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailEnvMap.vsh
InputName=VertexLitGeneric_DetailEnvMap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DetailEnvMapCameraSpace.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__55=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailEnvMapCameraSpace.vsh
InputName=VertexLitGeneric_DetailEnvMapCameraSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__55=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailEnvMapCameraSpace.vsh
InputName=VertexLitGeneric_DetailEnvMapCameraSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DetailEnvMapSphere.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__56=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailEnvMapSphere.vsh
InputName=VertexLitGeneric_DetailEnvMapSphere

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__56=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailEnvMapSphere.vsh
InputName=VertexLitGeneric_DetailEnvMapSphere

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DiffBumpTimesBase.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__57=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase.vsh
InputName=VertexLitGeneric_DiffBumpTimesBase

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__57=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase.vsh
InputName=VertexLitGeneric_DiffBumpTimesBase

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_EnvmappedBumpmap_NoLighting.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__58=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmap_NoLighting.vsh
InputName=VertexLitGeneric_EnvmappedBumpmap_NoLighting

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__58=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmap_NoLighting.vsh
InputName=VertexLitGeneric_EnvmappedBumpmap_NoLighting

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__59=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14.vsh
InputName=VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__59=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14.vsh
InputName=VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_EnvmappedBumpmap_VertLit.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__60=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmap_VertLit.vsh
InputName=VertexLitGeneric_EnvmappedBumpmap_VertLit

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__60=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmap_VertLit.vsh
InputName=VertexLitGeneric_EnvmappedBumpmap_VertLit

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_SelfIllumOnly.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__61=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_SelfIllumOnly.vsh
InputName=VertexLitGeneric_SelfIllumOnly

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__61=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_SelfIllumOnly.vsh
InputName=VertexLitGeneric_SelfIllumOnly

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitTexture.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__62=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitTexture.vsh
InputName=VertexLitTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__62=	"VertexLitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitTexture.vsh
InputName=VertexLitTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Water_vs11.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__63=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Water_vs11.vsh
InputName=Water_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__63=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Water_vs11.vsh
InputName=Water_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Water_ps14.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__64=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Water_ps14.vsh
InputName=Water_ps14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__64=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Water_ps14.vsh
InputName=Water_ps14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldTexture.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__65=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WorldTexture.vsh
InputName=WorldTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__65=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WorldTexture.vsh
InputName=WorldTexture

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldVertexAlpha.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__66=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexAlpha.vsh
InputName=WorldVertexAlpha

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__66=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexAlpha.vsh
InputName=WorldVertexAlpha

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldVertexTransition.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__67=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition.vsh
InputName=WorldVertexTransition

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__67=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition.vsh
InputName=WorldVertexTransition

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldVertexTransition_Micros.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__68=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_Micros.vsh
InputName=WorldVertexTransition_Micros

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__68=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_Micros.vsh
InputName=WorldVertexTransition_Micros

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\writez.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__69=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\writez.vsh
InputName=writez

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__69=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\writez.vsh
InputName=writez

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WriteZWithHeightClipPlane.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__70=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WriteZWithHeightClipPlane.vsh
InputName=WriteZWithHeightClipPlane

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__70=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WriteZWithHeightClipPlane.vsh
InputName=WriteZWithHeightClipPlane

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WriteZWithHeightClipPlane_World.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__71=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WriteZWithHeightClipPlane_World.vsh
InputName=WriteZWithHeightClipPlane_World

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__71=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WriteZWithHeightClipPlane_World.vsh
InputName=WriteZWithHeightClipPlane_World

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\alienscale_vs11.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__72=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_vs11.vsh
InputName=alienscale_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__72=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_vs11.vsh
InputName=alienscale_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\alienscale_vs14.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__73=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_vs14.vsh
InputName=alienscale_vs14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__73=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_vs14.vsh
InputName=alienscale_vs14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\overlay_fit_vs11.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__74=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\overlay_fit_vs11.vsh
InputName=overlay_fit_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__74=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\overlay_fit_vs11.vsh
InputName=overlay_fit_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Refract_model_vs11.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__75=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Refract_model_vs11.vsh
InputName=Refract_model_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__75=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Refract_model_vs11.vsh
InputName=Refract_model_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Refract_world_vs11.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__76=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Refract_world_vs11.vsh
InputName=Refract_world_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__76=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\Refract_world_vs11.vsh
InputName=Refract_world_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WaterCheap_vs11.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__77=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_vs11.vsh
InputName=WaterCheap_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__77=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_vs11.vsh
InputName=WaterCheap_vs11

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WaterCheap_vs14.vsh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__78=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_vs14.vsh
InputName=WaterCheap_vs14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__78=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_vs14.vsh
InputName=WaterCheap_vs14

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\alienscale_ps11.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__79=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_ps11.psh
InputName=alienscale_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__79=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_ps11.psh
InputName=alienscale_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\alienscale_overbright2_ps11.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__80=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_overbright2_ps11.psh
InputName=alienscale_overbright2_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__80=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_overbright2_ps11.psh
InputName=alienscale_overbright2_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\alienscale_ps14.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__81=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_ps14.psh
InputName=alienscale_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__81=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_ps14.psh
InputName=alienscale_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\alienscale_overbright2_ps14.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__82=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_overbright2_ps14.psh
InputName=alienscale_overbright2_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__82=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\alienscale_overbright2_ps14.psh
InputName=alienscale_overbright2_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Modulate_ps11.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__83=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Modulate_ps11.psh
InputName=Modulate_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__83=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Modulate_ps11.psh
InputName=Modulate_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\overlay_fit_ps11.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__84=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\overlay_fit_ps11.psh
InputName=overlay_fit_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__84=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\overlay_fit_ps11.psh
InputName=overlay_fit_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Refract_ps11.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__85=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Refract_ps11.psh
InputName=Refract_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__85=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Refract_ps11.psh
InputName=Refract_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WaterCheap_ps11.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__86=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_ps11.psh
InputName=WaterCheap_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__86=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_ps11.psh
InputName=WaterCheap_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WaterCheap_ps14.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__87=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_ps14.psh
InputName=WaterCheap_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__87=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_ps14.psh
InputName=WaterCheap_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\BaseTimesLightmapWet.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__88=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BaseTimesLightmapWet.psh
InputName=BaseTimesLightmapWet

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__88=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BaseTimesLightmapWet.psh
InputName=BaseTimesLightmapWet

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\BumpmappedEnvmap.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__89=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedEnvmap.psh
InputName=BumpmappedEnvmap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__89=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedEnvmap.psh
InputName=BumpmappedEnvmap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\BumpmappedLightmap_OverBright1.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__90=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedLightmap_OverBright1.psh
InputName=BumpmappedLightmap_OverBright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__90=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedLightmap_OverBright1.psh
InputName=BumpmappedLightmap_OverBright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\BumpmappedLightmap_OverBright2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__91=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedLightmap_OverBright2.psh
InputName=BumpmappedLightmap_OverBright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__91=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedLightmap_OverBright2.psh
InputName=BumpmappedLightmap_OverBright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Cable.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__92=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Cable.psh
InputName=Cable

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__92=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Cable.psh
InputName=Cable

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Eyes.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__93=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Eyes.psh
InputName=Eyes

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__93=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Eyes.psh
InputName=Eyes

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Eyes_Overbright2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__94=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Eyes_Overbright2.psh
InputName=Eyes_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__94=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Eyes_Overbright2.psh
InputName=Eyes_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\JellyFish.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__95=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\JellyFish.psh
InputName=JellyFish

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__95=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\JellyFish.psh
InputName=JellyFish

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__96=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric.psh
InputName=LightmappedGeneric

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__96=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric.psh
InputName=LightmappedGeneric

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_BaseAlphaMaskedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__97=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BaseAlphaMaskedEnvMapV2.psh
InputName=LightmappedGeneric_BaseAlphaMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__97=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BaseAlphaMaskedEnvMapV2.psh
InputName=LightmappedGeneric_BaseAlphaMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_BaseTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__98=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BaseTexture.psh
InputName=LightmappedGeneric_BaseTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__98=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BaseTexture.psh
InputName=LightmappedGeneric_BaseTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_basetextureblend.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__99=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_basetextureblend.psh
InputName=lightmappedgeneric_basetextureblend

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__99=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_basetextureblend.psh
InputName=lightmappedgeneric_basetextureblend

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_BumpmappedEnvmap.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__100=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedEnvmap.psh
InputName=LightmappedGeneric_BumpmappedEnvmap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__100=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedEnvmap.psh
InputName=LightmappedGeneric_BumpmappedEnvmap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_BumpmappedEnvmap_ps14.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__101=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedEnvmap_ps14.psh
InputName=LightmappedGeneric_BumpmappedEnvmap_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__101=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedEnvmap_ps14.psh
InputName=LightmappedGeneric_BumpmappedEnvmap_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_BumpmappedLightmap.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__102=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedLightmap.psh
InputName=LightmappedGeneric_BumpmappedLightmap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__102=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_BumpmappedLightmap.psh
InputName=LightmappedGeneric_BumpmappedLightmap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_Detail.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__103=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_Detail.psh
InputName=LightmappedGeneric_Detail

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__103=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_Detail.psh
InputName=LightmappedGeneric_Detail

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_DetailNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__104=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_DetailNoTexture.psh
InputName=LightmappedGeneric_DetailNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__104=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_DetailNoTexture.psh
InputName=LightmappedGeneric_DetailNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_DetailSelfIlluminated.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__105=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_DetailSelfIlluminated.psh
InputName=LightmappedGeneric_DetailSelfIlluminated

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__105=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_DetailSelfIlluminated.psh
InputName=LightmappedGeneric_DetailSelfIlluminated

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_EnvMapNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__106=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapNoTexture.psh
InputName=LightmappedGeneric_EnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__106=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapNoTexture.psh
InputName=LightmappedGeneric_EnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_EnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__107=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapV2.psh
InputName=LightmappedGeneric_EnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__107=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_EnvMapV2.psh
InputName=LightmappedGeneric_EnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_MaskedEnvMapNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__108=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_MaskedEnvMapNoTexture.psh
InputName=LightmappedGeneric_MaskedEnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__108=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_MaskedEnvMapNoTexture.psh
InputName=LightmappedGeneric_MaskedEnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_MaskedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__109=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_MaskedEnvMapV2.psh
InputName=LightmappedGeneric_MaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__109=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_MaskedEnvMapV2.psh
InputName=LightmappedGeneric_MaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_MultiplyByLighting.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__110=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_MultiplyByLighting.psh
InputName=LightmappedGeneric_MultiplyByLighting

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__110=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_MultiplyByLighting.psh
InputName=LightmappedGeneric_MultiplyByLighting

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_MultiplyByLightingNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__111=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_MultiplyByLightingNoTexture.psh
InputName=LightmappedGeneric_MultiplyByLightingNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__111=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_MultiplyByLightingNoTexture.psh
InputName=LightmappedGeneric_MultiplyByLightingNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_MultiplyByLightingSelfIllum.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__112=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_MultiplyByLightingSelfIllum.psh
InputName=LightmappedGeneric_MultiplyByLightingSelfIllum

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__112=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_MultiplyByLightingSelfIllum.psh
InputName=LightmappedGeneric_MultiplyByLightingSelfIllum

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_NoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__113=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_NoTexture.psh
InputName=LightmappedGeneric_NoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__113=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_NoTexture.psh
InputName=LightmappedGeneric_NoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_SelfIlluminated.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__114=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_SelfIlluminated.psh
InputName=LightmappedGeneric_SelfIlluminated

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__114=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_SelfIlluminated.psh
InputName=LightmappedGeneric_SelfIlluminated

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_SelfIlluminatedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__115=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_SelfIlluminatedEnvMapV2.psh
InputName=LightmappedGeneric_SelfIlluminatedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__115=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_SelfIlluminatedEnvMapV2.psh
InputName=LightmappedGeneric_SelfIlluminatedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_SelfIlluminatedMaskedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__116=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_SelfIlluminatedMaskedEnvMapV2.psh
InputName=LightmappedGeneric_SelfIlluminatedMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__116=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_SelfIlluminatedMaskedEnvMapV2.psh
InputName=LightmappedGeneric_SelfIlluminatedMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedTranslucentTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__117=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedTranslucentTexture.psh
InputName=LightmappedTranslucentTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__117=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedTranslucentTexture.psh
InputName=LightmappedTranslucentTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\predator.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__118=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\predator.psh
InputName=predator

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__118=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\predator.psh
InputName=predator

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Shadow.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__119=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Shadow.psh
InputName=Shadow

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__119=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Shadow.psh
InputName=Shadow

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\ShadowBuildTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__120=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\ShadowBuildTexture.psh
InputName=ShadowBuildTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__120=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\ShadowBuildTexture.psh
InputName=ShadowBuildTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\ShadowModel.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__121=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\ShadowModel.psh
InputName=ShadowModel

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__121=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\ShadowModel.psh
InputName=ShadowModel

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\SpriteRenderNormal.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__122=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\SpriteRenderNormal.psh
InputName=SpriteRenderNormal

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__122=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\SpriteRenderNormal.psh
InputName=SpriteRenderNormal

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\SpriteRenderTransColor.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__123=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\SpriteRenderTransColor.psh
InputName=SpriteRenderTransColor

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__123=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\SpriteRenderTransColor.psh
InputName=SpriteRenderTransColor

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\SpriteRenderTransAdd.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__124=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\SpriteRenderTransAdd.psh
InputName=SpriteRenderTransAdd

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__124=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\SpriteRenderTransAdd.psh
InputName=SpriteRenderTransAdd

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__125=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric.psh
InputName=UnlitGeneric

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__125=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric.psh
InputName=UnlitGeneric

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_BaseAlphaMaskedEnvMap.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__126=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_BaseAlphaMaskedEnvMap.psh
InputName=UnlitGeneric_BaseAlphaMaskedEnvMap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__126=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_BaseAlphaMaskedEnvMap.psh
InputName=UnlitGeneric_BaseAlphaMaskedEnvMap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_Detail.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__127=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_Detail.psh
InputName=UnlitGeneric_Detail

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__127=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_Detail.psh
InputName=UnlitGeneric_Detail

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailBaseAlphaMaskedEnvMap.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__128=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailBaseAlphaMaskedEnvMap.psh
InputName=UnlitGeneric_DetailBaseAlphaMaskedEnvMap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__128=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailBaseAlphaMaskedEnvMap.psh
InputName=UnlitGeneric_DetailBaseAlphaMaskedEnvMap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailEnvMap.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__129=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMap.psh
InputName=UnlitGeneric_DetailEnvMap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__129=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMap.psh
InputName=UnlitGeneric_DetailEnvMap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailEnvMapMask.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__130=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapMask.psh
InputName=UnlitGeneric_DetailEnvMapMask

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__130=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapMask.psh
InputName=UnlitGeneric_DetailEnvMapMask

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailEnvMapMaskNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__131=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapMaskNoTexture.psh
InputName=UnlitGeneric_DetailEnvMapMaskNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__131=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapMaskNoTexture.psh
InputName=UnlitGeneric_DetailEnvMapMaskNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailEnvMapNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__132=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapNoTexture.psh
InputName=UnlitGeneric_DetailEnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__132=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailEnvMapNoTexture.psh
InputName=UnlitGeneric_DetailEnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_DetailNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__133=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailNoTexture.psh
InputName=UnlitGeneric_DetailNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__133=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_DetailNoTexture.psh
InputName=UnlitGeneric_DetailNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_EnvMap.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__134=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMap.psh
InputName=UnlitGeneric_EnvMap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__134=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMap.psh
InputName=UnlitGeneric_EnvMap

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_EnvMapMask.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__135=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapMask.psh
InputName=UnlitGeneric_EnvMapMask

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__135=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapMask.psh
InputName=UnlitGeneric_EnvMapMask

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_EnvMapMaskNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__136=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapMaskNoTexture.psh
InputName=UnlitGeneric_EnvMapMaskNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__136=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapMaskNoTexture.psh
InputName=UnlitGeneric_EnvMapMaskNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_EnvMapNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__137=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapNoTexture.psh
InputName=UnlitGeneric_EnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__137=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_EnvMapNoTexture.psh
InputName=UnlitGeneric_EnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_NoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__138=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_NoTexture.psh
InputName=UnlitGeneric_NoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__138=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_NoTexture.psh
InputName=UnlitGeneric_NoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitTwoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__139=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitTwoTexture.psh
InputName=UnlitTwoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__139=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitTwoTexture.psh
InputName=UnlitTwoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__140=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric.psh
InputName=VertexLitGeneric

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__140=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric.psh
InputName=VertexLitGeneric

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_BaseAlphaMaskedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__141=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_BaseAlphaMaskedEnvMapV2.psh
InputName=VertexLitGeneric_BaseAlphaMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__141=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_BaseAlphaMaskedEnvMapV2.psh
InputName=VertexLitGeneric_BaseAlphaMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_Detail.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__142=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_Detail.psh
InputName=VertexLitGeneric_Detail

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__142=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_Detail.psh
InputName=VertexLitGeneric_Detail

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DetailBaseAlphaMaskedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__143=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailBaseAlphaMaskedEnvMapV2.psh
InputName=VertexLitGeneric_DetailBaseAlphaMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__143=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailBaseAlphaMaskedEnvMapV2.psh
InputName=VertexLitGeneric_DetailBaseAlphaMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DetailEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__144=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailEnvMapV2.psh
InputName=VertexLitGeneric_DetailEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__144=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailEnvMapV2.psh
InputName=VertexLitGeneric_DetailEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DetailMaskedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__145=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailMaskedEnvMapV2.psh
InputName=VertexLitGeneric_DetailMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__145=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailMaskedEnvMapV2.psh
InputName=VertexLitGeneric_DetailMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DetailNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__146=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailNoTexture.psh
InputName=VertexLitGeneric_DetailNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__146=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailNoTexture.psh
InputName=VertexLitGeneric_DetailNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DetailSelfIlluminated.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__147=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailSelfIlluminated.psh
InputName=VertexLitGeneric_DetailSelfIlluminated

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__147=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailSelfIlluminated.psh
InputName=VertexLitGeneric_DetailSelfIlluminated

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DetailSelfIlluminatedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__148=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailSelfIlluminatedEnvMapV2.psh
InputName=VertexLitGeneric_DetailSelfIlluminatedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__148=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailSelfIlluminatedEnvMapV2.psh
InputName=VertexLitGeneric_DetailSelfIlluminatedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DetailSelfIlluminatedMaskedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__149=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailSelfIlluminatedMaskedEnvMapV2.psh
InputName=VertexLitGeneric_DetailSelfIlluminatedMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__149=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DetailSelfIlluminatedMaskedEnvMapV2.psh
InputName=VertexLitGeneric_DetailSelfIlluminatedMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DiffBumpLightingOnly_Overbright1.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__150=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpLightingOnly_Overbright1.psh
InputName=VertexLitGeneric_DiffBumpLightingOnly_Overbright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__150=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpLightingOnly_Overbright1.psh
InputName=VertexLitGeneric_DiffBumpLightingOnly_Overbright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DiffBumpLightingOnly_Overbright2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__151=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpLightingOnly_Overbright2.psh
InputName=VertexLitGeneric_DiffBumpLightingOnly_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__151=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpLightingOnly_Overbright2.psh
InputName=VertexLitGeneric_DiffBumpLightingOnly_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DiffBumpTimesBase_Overbright1.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__152=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase_Overbright1.psh
InputName=VertexLitGeneric_DiffBumpTimesBase_Overbright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__152=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase_Overbright1.psh
InputName=VertexLitGeneric_DiffBumpTimesBase_Overbright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DiffBumpTimesBase_Overbright1_Translucent.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__153=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase_Overbright1_Translucent.psh
InputName=VertexLitGeneric_DiffBumpTimesBase_Overbright1_Translucent

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__153=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase_Overbright1_Translucent.psh
InputName=VertexLitGeneric_DiffBumpTimesBase_Overbright1_Translucent

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DiffBumpTimesBase_Overbright2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__154=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase_Overbright2.psh
InputName=VertexLitGeneric_DiffBumpTimesBase_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__154=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase_Overbright2.psh
InputName=VertexLitGeneric_DiffBumpTimesBase_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DiffBumpTimesBase_Overbright2_Translucent.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__155=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase_Overbright2_Translucent.psh
InputName=VertexLitGeneric_DiffBumpTimesBase_Overbright2_Translucent

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__155=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase_Overbright2_Translucent.psh
InputName=VertexLitGeneric_DiffBumpTimesBase_Overbright2_Translucent

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_EnvMapNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__156=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvMapNoTexture.psh
InputName=VertexLitGeneric_EnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__156=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvMapNoTexture.psh
InputName=VertexLitGeneric_EnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_EnvmappedBumpmapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__157=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmapV2.psh
InputName=VertexLitGeneric_EnvmappedBumpmapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__157=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmapV2.psh
InputName=VertexLitGeneric_EnvmappedBumpmapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__158=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha.psh
InputName=VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__158=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha.psh
InputName=VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha_ps14.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__159=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha_ps14.psh
InputName=VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__159=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha_ps14.psh
InputName=VertexLitGeneric_EnvmappedBumpmapV2_MultByAlpha_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_EnvmappedBumpmapV2_ps14.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__160=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmapV2_ps14.psh
InputName=VertexLitGeneric_EnvmappedBumpmapV2_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__160=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmapV2_ps14.psh
InputName=VertexLitGeneric_EnvmappedBumpmapV2_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_EnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__161=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvMapV2.psh
InputName=VertexLitGeneric_EnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__161=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvMapV2.psh
InputName=VertexLitGeneric_EnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_MaskedEnvMapNoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__162=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_MaskedEnvMapNoTexture.psh
InputName=VertexLitGeneric_MaskedEnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__162=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_MaskedEnvMapNoTexture.psh
InputName=VertexLitGeneric_MaskedEnvMapNoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_MaskedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__163=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_MaskedEnvMapV2.psh
InputName=VertexLitGeneric_MaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__163=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_MaskedEnvMapV2.psh
InputName=VertexLitGeneric_MaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_NoTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__164=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_NoTexture.psh
InputName=VertexLitGeneric_NoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__164=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_NoTexture.psh
InputName=VertexLitGeneric_NoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_SelfIlluminated.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__165=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_SelfIlluminated.psh
InputName=VertexLitGeneric_SelfIlluminated

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__165=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_SelfIlluminated.psh
InputName=VertexLitGeneric_SelfIlluminated

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_SelfIlluminatedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__166=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_SelfIlluminatedEnvMapV2.psh
InputName=VertexLitGeneric_SelfIlluminatedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__166=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_SelfIlluminatedEnvMapV2.psh
InputName=VertexLitGeneric_SelfIlluminatedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_SelfIlluminatedMaskedEnvMapV2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__167=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_SelfIlluminatedMaskedEnvMapV2.psh
InputName=VertexLitGeneric_SelfIlluminatedMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__167=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_SelfIlluminatedMaskedEnvMapV2.psh
InputName=VertexLitGeneric_SelfIlluminatedMaskedEnvMapV2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_SelfIllumOnly.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__168=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_SelfIllumOnly.psh
InputName=VertexLitGeneric_SelfIllumOnly

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__168=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_SelfIllumOnly.psh
InputName=VertexLitGeneric_SelfIllumOnly

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__169=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitTexture.psh
InputName=VertexLitTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__169=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitTexture.psh
InputName=VertexLitTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitTexture_Overbright2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__170=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitTexture_Overbright2.psh
InputName=VertexLitTexture_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__170=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitTexture_Overbright2.psh
InputName=VertexLitTexture_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WaterReflect_ps11.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__171=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WaterReflect_ps11.psh
InputName=WaterReflect_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__171=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WaterReflect_ps11.psh
InputName=WaterReflect_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WaterRefract_ps11.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__172=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WaterRefract_ps11.psh
InputName=WaterRefract_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__172=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WaterRefract_ps11.psh
InputName=WaterRefract_ps11

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Water_ps14.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__173=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Water_ps14.psh
InputName=Water_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__173=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\Water_ps14.psh
InputName=Water_ps14

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\white.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__174=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\white.psh
InputName=white

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__174=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\white.psh
InputName=white

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldTexture.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__175=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldTexture.psh
InputName=WorldTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__175=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldTexture.psh
InputName=WorldTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldVertexAlpha.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__176=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexAlpha.psh
InputName=WorldVertexAlpha

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__176=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexAlpha.psh
InputName=WorldVertexAlpha

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldVertexTransition.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__177=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition.psh
InputName=WorldVertexTransition

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__177=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition.psh
InputName=WorldVertexTransition

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldVertexTransition_Micros.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__178=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_Micros.psh
InputName=WorldVertexTransition_Micros

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__178=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_Micros.psh
InputName=WorldVertexTransition_Micros

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldVertexTransition_Micros_EditMode.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__179=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_Micros_EditMode.psh
InputName=WorldVertexTransition_Micros_EditMode

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__179=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_Micros_EditMode.psh
InputName=WorldVertexTransition_Micros_EditMode

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldVertexTransition_BlendBase2.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__180=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_BlendBase2.psh
InputName=WorldVertexTransition_BlendBase2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__180=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_BlendBase2.psh
InputName=WorldVertexTransition_BlendBase2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WriteEnvMapMaskToAlphaBuffer.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__181=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WriteEnvMapMaskToAlphaBuffer.psh
InputName=WriteEnvMapMaskToAlphaBuffer

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__181=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WriteEnvMapMaskToAlphaBuffer.psh
InputName=WriteEnvMapMaskToAlphaBuffer

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WriteZWithHeightClipPlane.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__182=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WriteZWithHeightClipPlane.psh
InputName=WriteZWithHeightClipPlane

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__182=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\WriteZWithHeightClipPlane.psh
InputName=WriteZWithHeightClipPlane

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\yuv.psh
!IF  "$(CFG)" == "stdshader_dx8 - Win32 Release"
USERDEP__183=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\yuv.psh
InputName=yuv

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx8 - Win32 Debug"
USERDEP__183=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\yuv.psh
InputName=yuv

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\devtools\bin\psh_prep.pl
# End Source File
# Begin Source File

SOURCE=.\stdshader_dx8.cpp
# End Source File
# Begin Source File

SOURCE=.\stdshader_dx8.txt
# End Source File
# Begin Source File

SOURCE=..\..\devtools\bin\vsh_prep.pl
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\shaderlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
