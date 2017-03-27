# Microsoft Developer Studio Project File - Name="stdshader_dbg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=stdshader_dbg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "stdshader_dbg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "stdshader_dbg.mak" CFG="stdshader_dbg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "stdshader_dbg - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "stdshader_dbg - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "stdshader_dbg"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_dbg"
# PROP BASE Intermediate_Dir "Release_dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_dbg"
# PROP Intermediate_Dir "Release_dbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DBG_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DBG_DLL_EXPORT" /D "_WIN32" /Fo"Release_dbg/" /Fd"Release_dbg/" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Release_dbg/stdshader_dbg.bsc"
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /pdb:"Release_dbg/stdshader_dbg.pdb" /machine:I386 /out:"Release_dbg/stdshader_dbg.dll" /implib:"Release_dbg/stdshader_dbg.lib" /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Release_dbg
TargetPath=.\Release_dbg\stdshader_dbg.dll
InputPath=.\Release_dbg\stdshader_dbg.dll
SOURCE="$(InputPath)"

"..\..\..\bin\stdshader_dbg.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\stdshader_dbg.dll attrib -r ..\..\..\bin\stdshader_dbg.dll 
	copy $(TargetPath) ..\..\..\bin\stdshader_dbg.dll 
	if exist $(TargetDir)\stdshader_dbg.map copy $(TargetDir)\stdshader_dbg.map ..\..\..\bin\stdshader_dbg.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_dbg"
# PROP BASE Intermediate_Dir "Debug_dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_dbg"
# PROP Intermediate_Dir "Debug_dbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DBG_DLL_EXPORT" /D "_WIN32" /FR"Debug/" /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DBG_DLL_EXPORT" /D "_WIN32" /FR"Debug_dbg/" /Fo"Debug_dbg/" /Fd"Debug_dbg/" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug_dbg/stdshader_dbg.bsc"
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /pdb:"Debug_dbg/stdshader_dbg.pdb" /debug /machine:I386 /out:"Debug_dbg/stdshader_dbg.dll" /implib:"Debug_dbg/stdshader_dbg.lib" /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Debug_dbg
TargetPath=.\Debug_dbg\stdshader_dbg.dll
InputPath=.\Debug_dbg\stdshader_dbg.dll
SOURCE="$(InputPath)"

"..\..\..\bin\stdshader_dbg.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\stdshader_dbg.dll attrib -r ..\..\..\bin\stdshader_dbg.dll 
	copy $(TargetPath) ..\..\..\bin\stdshader_dbg.dll 
	if exist $(TargetDir)\stdshader_dbg.map copy $(TargetDir)\stdshader_dbg.map ..\..\..\bin\stdshader_dbg.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "stdshader_dbg - Win32 Release"
# Name "stdshader_dbg - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BaseVSShader.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseVSShader.h
# End Source File
# Begin Source File

SOURCE=.\debugbumpedlightmap.cpp
# End Source File
# Begin Source File

SOURCE=.\debugbumpedvertexlit.cpp
# End Source File
# Begin Source File

SOURCE=.\debugfbtexture.cpp
# End Source File
# Begin Source File

SOURCE=.\debuglightingonly.cpp
# End Source File
# Begin Source File

SOURCE=.\debuglightmap.cpp
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

SOURCE=.\debugunlit.cpp
# End Source File
# Begin Source File

SOURCE=.\debugvertexlit.cpp
# End Source File
# Begin Source File

SOURCE=.\fillrate.cpp
# End Source File
# Begin Source File

SOURCE=.\fillrate_dx6.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\vmatrix.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Shaders"
# Begin Source File

SOURCE=.\vertexlit_lighting_only_ps20.fxc
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__0=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_lighting_only_ps20.fxc
InputName=vertexlit_lighting_only_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__0=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_lighting_only_ps20.fxc
InputName=vertexlit_lighting_only_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLit_and_unlit_Generic_vs20.fxc
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__1=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\VertexLit_and_unlit_Generic_vs20.fxc
InputName=VertexLit_and_unlit_Generic_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__1=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\VertexLit_and_unlit_Generic_vs20.fxc
InputName=VertexLit_and_unlit_Generic_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\BumpmappedLightmap.vsh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__2=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedLightmap.vsh
InputName=BumpmappedLightmap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__2=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedLightmap.vsh
InputName=BumpmappedLightmap

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\DebugTangentSpace.vsh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__3=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\DebugTangentSpace.vsh
InputName=DebugTangentSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__3=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\DebugTangentSpace.vsh
InputName=DebugTangentSpace

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\fillrate.vsh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__4=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\fillrate.vsh
InputName=fillrate

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__4=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\fillrate.vsh
InputName=fillrate

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightingOnly.vsh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__5=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightingOnly.vsh
InputName=LightingOnly

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__5=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightingOnly.vsh
InputName=LightingOnly

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_LightingOnly.vsh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__6=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_LightingOnly.vsh
InputName=LightmappedGeneric_LightingOnly

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__6=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_LightingOnly.vsh
InputName=LightmappedGeneric_LightingOnly

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\unlitgeneric.vsh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__7=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\unlitgeneric.vsh
InputName=unlitgeneric

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__7=	"UnlitGeneric_inc.vsh"	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\unlitgeneric.vsh
InputName=unlitgeneric

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_LightingOnly.vsh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__8=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_LightingOnly.vsh
InputName=UnlitGeneric_LightingOnly

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__8=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_LightingOnly.vsh
InputName=UnlitGeneric_LightingOnly

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DiffBumpTimesBase.vsh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__9=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase.vsh
InputName=VertexLitGeneric_DiffBumpTimesBase

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__9=	"macros.vsh"	"..\..\devtools\bin\vsa9.exe"	"..\..\devtools\bin\vsh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpTimesBase.vsh
InputName=VertexLitGeneric_DiffBumpTimesBase

"vshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\vsh_prep.pl -dx9 $(InputName).vsh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\BumpmappedLightmap_LightingOnly_OverBright1.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__10=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedLightmap_LightingOnly_OverBright1.psh
InputName=BumpmappedLightmap_LightingOnly_OverBright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__10=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedLightmap_LightingOnly_OverBright1.psh
InputName=BumpmappedLightmap_LightingOnly_OverBright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\BumpmappedLightmap_LightingOnly_OverBright2.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__11=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedLightmap_LightingOnly_OverBright2.psh
InputName=BumpmappedLightmap_LightingOnly_OverBright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__11=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\BumpmappedLightmap_LightingOnly_OverBright2.psh
InputName=BumpmappedLightmap_LightingOnly_OverBright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\fillrate.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__12=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\fillrate.psh
InputName=fillrate

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__12=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\fillrate.psh
InputName=fillrate

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_LightingOnly_Overbright1.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__13=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_LightingOnly_Overbright1.psh
InputName=LightmappedGeneric_LightingOnly_Overbright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__13=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_LightingOnly_Overbright1.psh
InputName=LightmappedGeneric_LightingOnly_Overbright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\LightmappedGeneric_LightingOnly_Overbright2.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__14=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_LightingOnly_Overbright2.psh
InputName=LightmappedGeneric_LightingOnly_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__14=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\LightmappedGeneric_LightingOnly_Overbright2.psh
InputName=LightmappedGeneric_LightingOnly_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\unlitgeneric.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__15=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\unlitgeneric.psh
InputName=unlitgeneric

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__15=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\unlitgeneric.psh
InputName=unlitgeneric

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\UnlitGeneric_NoTexture.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__16=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_NoTexture.psh
InputName=UnlitGeneric_NoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__16=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\UnlitGeneric_NoTexture.psh
InputName=UnlitGeneric_NoTexture

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright1.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__17=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright1.psh
InputName=VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__17=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright1.psh
InputName=VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright2.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__18=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright2.psh
InputName=VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__18=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright2.psh
InputName=VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_lightingonly_overbright1.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__19=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\vertexlitgeneric_lightingonly_overbright1.psh
InputName=vertexlitgeneric_lightingonly_overbright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__19=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\vertexlitgeneric_lightingonly_overbright1.psh
InputName=vertexlitgeneric_lightingonly_overbright1

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric_lightingonly_overbright2.psh
!IF  "$(CFG)" == "stdshader_dbg - Win32 Release"
USERDEP__20=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\vertexlitgeneric_lightingonly_overbright2.psh
InputName=vertexlitgeneric_lightingonly_overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dbg - Win32 Debug"
USERDEP__20=	"..\..\devtools\bin\psa9.exe"	"..\..\devtools\bin\psh_prep.pl"
# Begin Custom Build
InputPath=.\vertexlitgeneric_lightingonly_overbright2.psh
InputName=vertexlitgeneric_lightingonly_overbright2

"pshtmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\psh_prep.pl -dx9 $(InputName).psh

# End Custom Build
!ENDIF
# End Source File
# End Group
# Begin Source File

SOURCE=.\stdshader_dbg.cpp
# End Source File
# Begin Source File

SOURCE=.\stdshader_dbg.txt
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
