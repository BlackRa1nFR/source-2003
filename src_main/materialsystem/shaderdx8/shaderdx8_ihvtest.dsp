# Microsoft Developer Studio Project File - Name="shaderdx8_ihvtest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=shaderdx8_ihvtest - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "shaderdx8_ihvtest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "shaderdx8_ihvtest.mak" CFG="shaderdx8_ihvtest - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "shaderdx8_ihvtest - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "shaderdx8_ihvtest - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/common/MaterialSystem/shaderdx8", VJRCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SHADERDX8_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /GX /Zi /O2 /Ob2 /I "..\..\common" /I "..\..\public" /I "..\..\dx8sdk\include" /I "..\\" /D "NDEBUG" /D "IHVTEST" /D "_WIN32" /D "IMAGE_LOADER_NO_DXTC" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SHADER_DLL_EXPORT" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /debug /machine:I386 /out:"IHVTestRelease/shaderapidx8.dll" /fixed:no
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetPath=.\IHVTestRelease\shaderapidx8.dll
InputPath=.\IHVTestRelease\shaderapidx8.dll
SOURCE="$(InputPath)"

"..\..\..\bin\shaderapidx8.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\shaderapidx8.dll attrib -r ..\..\..\bin\shaderapidx8.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\bin\shaderapidx8.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "IHVTestDebug"
# PROP Intermediate_Dir "IHVTestDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SHADERDX8_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\common" /I "..\..\public" /I "..\..\dx8sdk\include" /I "..\\" /D "_DEBUG" /D "IHVTEST" /D "_WIN32" /D "IMAGE_LOADER_NO_DXTC" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SHADER_DLL_EXPORT" /D strncpy=use_Q_strncpy_instead /D _snprintf=use_Q_snprintf_instead /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"IHVTestDebug/shaderapidx8.dll" /pdbtype:sept
# Begin Custom Build
TargetPath=.\IHVTestDebug\shaderapidx8.dll
InputPath=.\IHVTestDebug\shaderapidx8.dll
SOURCE="$(InputPath)"

"..\..\..\bin\shaderapidx8.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\shaderapidx8.dll attrib -r ..\..\..\bin\shaderapidx8.dll 
	if exist $(TargetPath) copy $(TargetPath) ..\..\..\bin\shaderapidx8.dll 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "shaderdx8_ihvtest - Win32 Release"
# Name "shaderdx8_ihvtest - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CMaterialSystemStats.cpp
# End Source File
# Begin Source File

SOURCE=.\ColorFormatDX8.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\crtmemdebug.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\imageloader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\mathlib.cpp
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

SOURCE=..\..\Public\utlsymbol.cpp
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

SOURCE=.\dynamicib.h
# End Source File
# Begin Source File

SOURCE=.\dynamicvb.h
# End Source File
# Begin Source File

SOURCE=.\IMeshDX8.h
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

SOURCE=.\TextureDX8.h
# End Source File
# Begin Source File

SOURCE=.\VertexShaderDX8.h
# End Source File
# End Group
# Begin Group "Vertex Shaders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\basetimeslightmapwet.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__BASET="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\basetimeslightmapwet.vsh
InputName=basetimeslightmapwet

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__BASET="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\basetimeslightmapwet.vsh
InputName=basetimeslightmapwet

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\bumpmappedenvmap.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__BUMPM="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\bumpmappedenvmap.vsh
InputName=bumpmappedenvmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__BUMPM="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\bumpmappedenvmap.vsh
InputName=bumpmappedenvmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\bumpmappedlightmap.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__BUMPMA="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\bumpmappedlightmap.vsh
InputName=bumpmappedlightmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__BUMPMA="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\bumpmappedlightmap.vsh
InputName=bumpmappedlightmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cable.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__CABLE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\cable.vsh
InputName=cable

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__CABLE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\cable.vsh
InputName=cable

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\colorstreamtest.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__COLOR="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\colorstreamtest.vsh
InputName=colorstreamtest

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__COLOR="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\colorstreamtest.vsh
InputName=colorstreamtest

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\debugfbtexture.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__DEBUG="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\debugfbtexture.vsh
InputName=debugfbtexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__DEBUG="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\debugfbtexture.vsh
InputName=debugfbtexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\debugtangentspace.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__DEBUGT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\debugtangentspace.vsh
InputName=debugtangentspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__DEBUGT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\debugtangentspace.vsh
InputName=debugtangentspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\eyes.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__EYES_="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\eyes.vsh
InputName=eyes

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__EYES_="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\eyes.vsh
InputName=eyes

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fillrate.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__FILLR="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\fillrate.vsh
InputName=fillrate

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__FILLR="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\fillrate.vsh
InputName=fillrate

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glowwarp.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__GLOWW="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\glowwarp.vsh
InputName=glowwarp

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__GLOWW="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\glowwarp.vsh
InputName=glowwarp

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hotspotglow.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__HOTSP="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\hotspotglow.vsh
InputName=hotspotglow

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__HOTSP="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\hotspotglow.vsh
InputName=hotspotglow

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jellyfish.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__JELLY="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\jellyfish.vsh
InputName=jellyfish

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__JELLY="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\jellyfish.vsh
InputName=jellyfish

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightingonly.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightingonly.vsh
InputName=lightingonly

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightingonly.vsh
InputName=lightingonly

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTM="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric.vsh
InputName=lightmappedgeneric

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTM="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric.vsh
InputName=lightmappedgeneric

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_basetexture.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMA="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_basetexture.vsh
InputName=lightmappedgeneric_basetexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMA="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_basetexture.vsh
InputName=lightmappedgeneric_basetexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_bumpmappedenvmap.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAP="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_bumpmappedenvmap.vsh
InputName=lightmappedgeneric_bumpmappedenvmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAP="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_bumpmappedenvmap.vsh
InputName=lightmappedgeneric_bumpmappedenvmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_bumpmappedenvmap_ps14.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPP="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_bumpmappedenvmap_ps14.vsh
InputName=lightmappedgeneric_bumpmappedenvmap_ps14

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPP="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_bumpmappedenvmap_ps14.vsh
InputName=lightmappedgeneric_bumpmappedenvmap_ps14

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_bumpmappedlightmap.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_bumpmappedlightmap.vsh
InputName=lightmappedgeneric_bumpmappedlightmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_bumpmappedlightmap.vsh
InputName=lightmappedgeneric_bumpmappedlightmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_detail.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPED="..\..\..\devtools\bin\vsh_prep.pl"	"macros.vsh"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_detail.vsh
InputName=lightmappedgeneric_detail

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPED="..\..\..\devtools\bin\vsh_prep.pl"	"macros.vsh"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_detail.vsh
InputName=lightmappedgeneric_detail

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_detailvertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPEDG="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_detailvertexcolor.vsh
InputName=lightmappedgeneric_detailvertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPEDG="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_detailvertexcolor.vsh
InputName=lightmappedgeneric_detailvertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_envmap.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPEDGE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmap.vsh
InputName=lightmappedgeneric_envmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPEDGE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmap.vsh
InputName=lightmappedgeneric_envmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_envmapcameraspace.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPEDGEN="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmapcameraspace.vsh
InputName=lightmappedgeneric_envmapcameraspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPEDGEN="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmapcameraspace.vsh
InputName=lightmappedgeneric_envmapcameraspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_envmapcameraspacevertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPEDGENE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmapcameraspacevertexcolor.vsh
InputName=lightmappedgeneric_envmapcameraspacevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPEDGENE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmapcameraspacevertexcolor.vsh
InputName=lightmappedgeneric_envmapcameraspacevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_envmapsphere.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPEDGENER="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmapsphere.vsh
InputName=lightmappedgeneric_envmapsphere

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPEDGENER="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmapsphere.vsh
InputName=lightmappedgeneric_envmapsphere

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_envmapspherevertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPEDGENERI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmapspherevertexcolor.vsh
InputName=lightmappedgeneric_envmapspherevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPEDGENERI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmapspherevertexcolor.vsh
InputName=lightmappedgeneric_envmapspherevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_envmapvertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPEDGENERIC="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmapvertexcolor.vsh
InputName=lightmappedgeneric_envmapvertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPEDGENERIC="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_envmapvertexcolor.vsh
InputName=lightmappedgeneric_envmapvertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_lightingonly.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPEDGENERIC_="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_lightingonly.vsh
InputName=lightmappedgeneric_lightingonly

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPEDGENERIC_="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_lightingonly.vsh
InputName=lightmappedgeneric_lightingonly

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_vertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPEDGENERIC_V="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_vertexcolor.vsh
InputName=lightmappedgeneric_vertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPEDGENERIC_V="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedgeneric_vertexcolor.vsh
InputName=lightmappedgeneric_vertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lightmappedtranslucenttexture.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LIGHTMAPPEDT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedtranslucenttexture.vsh
InputName=lightmappedtranslucenttexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LIGHTMAPPEDT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"lightmappedgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\lightmappedtranslucenttexture.vsh
InputName=lightmappedtranslucenttexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\localspecular.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__LOCAL="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\localspecular.vsh
InputName=localspecular

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__LOCAL="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\localspecular.vsh
InputName=localspecular

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\particlesphere.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__PARTI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\particlesphere.vsh
InputName=particlesphere

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__PARTI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\particlesphere.vsh
InputName=particlesphere

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\predator.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__PREDA="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\predator.vsh
InputName=predator

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__PREDA="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\predator.vsh
InputName=predator

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\predator_envmap.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__PREDAT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\predator_envmap.vsh
InputName=predator_envmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__PREDAT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\predator_envmap.vsh
InputName=predator_envmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\predator_envmaptimesenvmapmask.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__PREDATO="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\predator_envmaptimesenvmapmask.vsh
InputName=predator_envmaptimesenvmapmask

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__PREDATO="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\predator_envmaptimesenvmapmask.vsh
InputName=predator_envmaptimesenvmapmask

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\shadowmodel.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__SHADO="..\..\..\devtools\bin\vsh_prep.pl"	"macros.vsh"	
# Begin Custom Build
InputPath=.\shadowmodel.vsh
InputName=shadowmodel

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__SHADO="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\shadowmodel.vsh
InputName=shadowmodel

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\teeth.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__TEETH="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\teeth.vsh
InputName=teeth

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__TEETH="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\teeth.vsh
InputName=teeth

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLIT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric.vsh
InputName=unlitgeneric

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLIT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric.vsh
InputName=unlitgeneric

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_detail.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITG="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detail.vsh
InputName=unlitgeneric_detail

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITG="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detail.vsh
InputName=unlitgeneric_detail

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_detailenvmap.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmap.vsh
InputName=unlitgeneric_detailenvmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmap.vsh
InputName=unlitgeneric_detailenvmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_detailenvmapcameraspace.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGEN="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmapcameraspace.vsh
InputName=unlitgeneric_detailenvmapcameraspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGEN="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmapcameraspace.vsh
InputName=unlitgeneric_detailenvmapcameraspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_detailenvmapcameraspacevertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmapcameraspacevertexcolor.vsh
InputName=unlitgeneric_detailenvmapcameraspacevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmapcameraspacevertexcolor.vsh
InputName=unlitgeneric_detailenvmapcameraspacevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_detailenvmapsphere.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENER="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmapsphere.vsh
InputName=unlitgeneric_detailenvmapsphere

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENER="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmapsphere.vsh
InputName=unlitgeneric_detailenvmapsphere

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_detailenvmapspherevertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmapspherevertexcolor.vsh
InputName=unlitgeneric_detailenvmapspherevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmapspherevertexcolor.vsh
InputName=unlitgeneric_detailenvmapspherevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_detailenvmapvertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmapvertexcolor.vsh
InputName=unlitgeneric_detailenvmapvertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailenvmapvertexcolor.vsh
InputName=unlitgeneric_detailenvmapvertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_detailvertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC_="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailvertexcolor.vsh
InputName=unlitgeneric_detailvertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC_="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_detailvertexcolor.vsh
InputName=unlitgeneric_detailvertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_envmap.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC_E="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmap.vsh
InputName=unlitgeneric_envmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC_E="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmap.vsh
InputName=unlitgeneric_envmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_envmapcameraspace.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC_EN="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmapcameraspace.vsh
InputName=unlitgeneric_envmapcameraspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC_EN="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmapcameraspace.vsh
InputName=unlitgeneric_envmapcameraspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_envmapcameraspacevertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC_ENV="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmapcameraspacevertexcolor.vsh
InputName=unlitgeneric_envmapcameraspacevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC_ENV="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmapcameraspacevertexcolor.vsh
InputName=unlitgeneric_envmapcameraspacevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_envmapsphere.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC_ENVM="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmapsphere.vsh
InputName=unlitgeneric_envmapsphere

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC_ENVM="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmapsphere.vsh
InputName=unlitgeneric_envmapsphere

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_envmapspherevertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC_ENVMA="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmapspherevertexcolor.vsh
InputName=unlitgeneric_envmapspherevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC_ENVMA="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmapspherevertexcolor.vsh
InputName=unlitgeneric_envmapspherevertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_envmapvertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC_ENVMAP="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmapvertexcolor.vsh
InputName=unlitgeneric_envmapvertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC_ENVMAP="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_envmapvertexcolor.vsh
InputName=unlitgeneric_envmapvertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_lightingonly.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC_L="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_lightingonly.vsh
InputName=unlitgeneric_lightingonly

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC_L="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_lightingonly.vsh
InputName=unlitgeneric_lightingonly

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_lightingonlyworld.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC_LI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_lightingonlyworld.vsh
InputName=unlitgeneric_lightingonlyworld

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC_LI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_lightingonlyworld.vsh
InputName=unlitgeneric_lightingonlyworld

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unlitgeneric_vertexcolor.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__UNLITGENERIC_V="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_vertexcolor.vsh
InputName=unlitgeneric_vertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__UNLITGENERIC_V="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"unlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\unlitgeneric_vertexcolor.vsh
InputName=unlitgeneric_vertexcolor

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitenvmappedtexture.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitenvmappedtexture.vsh
InputName=vertexlitenvmappedtexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitenvmappedtexture.vsh
InputName=vertexlitenvmappedtexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitenvmappedtexture_cameraspace.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEX="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitenvmappedtexture_cameraspace.vsh
InputName=vertexlitenvmappedtexture_cameraspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEX="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitenvmappedtexture_cameraspace.vsh
InputName=vertexlitenvmappedtexture_cameraspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitenvmappedtexture_spheremap.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXL="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitenvmappedtexture_spheremap.vsh
InputName=vertexlitenvmappedtexture_spheremap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXL="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitenvmappedtexture_spheremap.vsh
InputName=vertexlitenvmappedtexture_spheremap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_detail.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXLI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_detail.vsh
InputName=vertexlitgeneric_detail

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXLI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_detail.vsh
InputName=vertexlitgeneric_detail

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_detailenvmap.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXLIT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_detailenvmap.vsh
InputName=vertexlitgeneric_detailenvmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXLIT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_detailenvmap.vsh
InputName=vertexlitgeneric_detailenvmap

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_detailenvmapcameraspace.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXLITG="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_detailenvmapcameraspace.vsh
InputName=vertexlitgeneric_detailenvmapcameraspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXLITG="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_detailenvmapcameraspace.vsh
InputName=vertexlitgeneric_detailenvmapcameraspace

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_detailenvmapsphere.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXLITGE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_detailenvmapsphere.vsh
InputName=vertexlitgeneric_detailenvmapsphere

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXLITGE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_detailenvmapsphere.vsh
InputName=vertexlitgeneric_detailenvmapsphere

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_diffbumptimesbase.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXLITGEN="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_diffbumptimesbase.vsh
InputName=vertexlitgeneric_diffbumptimesbase

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXLITGEN="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_diffbumptimesbase.vsh
InputName=vertexlitgeneric_diffbumptimesbase

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_envmappedbumpmap_nolighting.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXLITGENE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_envmappedbumpmap_nolighting.vsh
InputName=vertexlitgeneric_envmappedbumpmap_nolighting

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXLITGENE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_envmappedbumpmap_nolighting.vsh
InputName=vertexlitgeneric_envmappedbumpmap_nolighting

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXLITGENER="..\..\..\devtools\bin\vsh_prep.pl"	"macros.vsh"	
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14.vsh
InputName=VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXLITGENER="..\..\..\devtools\bin\vsh_prep.pl"	"macros.vsh"	
# Begin Custom Build
InputPath=.\VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14.vsh
InputName=VertexLitGeneric_EnvmappedBumpmap_NoLighting_ps14

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_envmappedbumpmap_vertlit.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXLITGENERI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_envmappedbumpmap_vertlit.vsh
InputName=vertexlitgeneric_envmappedbumpmap_vertlit

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXLITGENERI="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_envmappedbumpmap_vertlit.vsh
InputName=vertexlitgeneric_envmappedbumpmap_vertlit

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_selfillumonly.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXLITGENERIC="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_selfillumonly.vsh
InputName=vertexlitgeneric_selfillumonly

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXLITGENERIC="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\vertexlitgeneric_selfillumonly.vsh
InputName=vertexlitgeneric_selfillumonly

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vertexlittexture.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__VERTEXLITT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build
InputPath=.\vertexlittexture.vsh
InputName=vertexlittexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__VERTEXLITT="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	"vertexlitgeneric_inc.vsh"	
# Begin Custom Build - ..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh
InputPath=.\vertexlittexture.vsh
InputName=vertexlittexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\waterwarp.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__WATER="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\waterwarp.vsh
InputName=waterwarp

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__WATER="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\waterwarp.vsh
InputName=waterwarp

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\worldtexture.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__WORLD="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\worldtexture.vsh
InputName=worldtexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__WORLD="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\worldtexture.vsh
InputName=worldtexture

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\worldvertexalpha.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__WORLDV="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\worldvertexalpha.vsh
InputName=worldvertexalpha

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__WORLDV="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\worldvertexalpha.vsh
InputName=worldvertexalpha

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\worldvertextransition.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__WORLDVE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\worldvertextransition.vsh
InputName=worldvertextransition

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__WORLDVE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\worldvertextransition.vsh
InputName=worldvertextransition

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\writeenvmapmasktoalphabuffer.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__WRITE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\writeenvmapmasktoalphabuffer.vsh
InputName=writeenvmapmasktoalphabuffer

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__WRITE="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\writeenvmapmasktoalphabuffer.vsh
InputName=writeenvmapmasktoalphabuffer

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\writezwithheightclipplane.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__WRITEZ="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\writezwithheightclipplane.vsh
InputName=writezwithheightclipplane

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__WRITEZ="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\writezwithheightclipplane.vsh
InputName=writezwithheightclipplane

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\writezwithheightclipplane_world.vsh

!IF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Release"

USERDEP__WRITEZW="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\writezwithheightclipplane_world.vsh
InputName=writezwithheightclipplane_world

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ELSEIF  "$(CFG)" == "shaderdx8_ihvtest - Win32 Debug"

USERDEP__WRITEZW="macros.vsh"	"..\..\..\devtools\bin\vsh_prep.pl"	
# Begin Custom Build
InputPath=.\writezwithheightclipplane_world.vsh
InputName=writezwithheightclipplane_world

"vshtmp\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl $(InputName).vsh

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Vertex Shader include files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\LightmappedGeneric_inc.vsh
# End Source File
# Begin Source File

SOURCE=.\macros.vsh
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_inc.vsh
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_inc.vsh
# End Source File
# End Group
# Begin Source File

SOURCE=.\old.vsh
# End Source File
# Begin Source File

SOURCE=..\..\devtools\bin\vsh_prep.pl
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\DX8SDK\lib\d3dx8.lib
# End Source File
# Begin Source File

SOURCE=..\..\DX8SDK\lib\d3d8.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
