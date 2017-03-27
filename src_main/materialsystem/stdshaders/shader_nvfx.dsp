# Microsoft Developer Studio Project File - Name="shader_nvfx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=shader_nvfx - Win32 Debug ps20
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "shader_nvfx.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "shader_nvfx.mak" CFG="shader_nvfx - Win32 Debug ps20"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "shader_nvfx - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "shader_nvfx - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "shader_nvfx - Win32 Release ps20" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "shader_nvfx - Win32 Debug ps20" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "shader_nvfx"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "shader_nvfx - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "release_nvfx"
# PROP BASE Intermediate_Dir "release_nvfx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "release_nvfx"
# PROP Intermediate_Dir "release_nvfx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "shader_nvfx_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /I "fxctmp9_nv3x" /D "NDEBUG" /D "PS20A" /D "NV3X" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "shader_nvfx_DLL_EXPORT" /D "_WIN32" /FD /c
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
TargetDir=.\release_nvfx
TargetPath=.\release_nvfx\shader_nvfx.dll
InputPath=.\release_nvfx\shader_nvfx.dll
SOURCE="$(InputPath)"

"..\..\..\bin\shader_nvfx.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\shader_nvfx.dll attrib -r ..\..\..\bin\shader_nvfx.dll 
	copy $(TargetPath) ..\..\..\bin\shader_nvfx.dll 
	if exist $(TargetDir)\shader_nvfx.map copy $(TargetDir)\shader_nvfx.map ..\..\..\bin\shader_nvfx.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "debug_nvfx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "debug_nvfx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "shader_nvfx_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /I "fxctmp9_nv3x" /D "_DEBUG" /D "PS20A" /D "NV3X" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "shader_nvfx_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"debug_nvfx/shader_nvfx.bsc"
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /pdb:"debug_nvfx/shader_nvfx.pdb" /debug /machine:I386 /out:"debug_nvfx/shader_nvfx.dll" /implib:"debug_nvfx/shader_nvfx.lib" /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\debug_nvfx
TargetPath=.\debug_nvfx\shader_nvfx.dll
InputPath=.\debug_nvfx\shader_nvfx.dll
SOURCE="$(InputPath)"

"..\..\..\bin\shader_nvfx.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\shader_nvfx.dll attrib -r ..\..\..\bin\shader_nvfx.dll 
	copy $(TargetPath) ..\..\..\bin\shader_nvfx.dll 
	if exist $(TargetDir)\shader_nvfx.map copy $(TargetDir)\shader_nvfx.map ..\..\..\bin\shader_nvfx.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "shader_nvfx___Win32_Release_ps20"
# PROP BASE Intermediate_Dir "shader_nvfx___Win32_Release_ps20"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "shader_nvfx___Win32_Release_ps20"
# PROP Intermediate_Dir "shader_nvfx___Win32_Release_ps20"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "NV3X" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "shader_nvfx_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /I "fxctmp9_nv3x_ps20" /D "NDEBUG" /D "NV3X" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "shader_nvfx_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\shader_nvfx___Win32_Release_ps20
TargetPath=.\shader_nvfx___Win32_Release_ps20\shader_nvfx.dll
InputPath=.\shader_nvfx___Win32_Release_ps20\shader_nvfx.dll
SOURCE="$(InputPath)"

"..\..\..\bin\shader_nvfx_ps20.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\shader_nvfx_ps20.dll attrib -r ..\..\..\bin\shader_nvfx_ps20.dll 
	copy $(TargetPath) ..\..\..\bin\shader_nvfx_ps20.dll 
	if exist $(TargetDir)\shader_nvfx_ps20.map copy $(TargetDir)\shader_nvfx.map ..\..\..\bin\shader_nvfx_ps20.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "shader_nvfx___Win32_Debug_ps20"
# PROP BASE Intermediate_Dir "shader_nvfx___Win32_Debug_ps20"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "shader_nvfx___Win32_Debug_ps20"
# PROP Intermediate_Dir "shader_nvfx___Win32_Debug_ps20"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "NV3X" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "shader_nvfx_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /I "fxctmp9_nv3x_ps20" /D "_DEBUG" /D "NV3X" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "shader_nvfx_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"debug_nvfx/shader_nvfx.bsc"
# ADD BSC32 /nologo /o"debug_nvfx/shader_nvfx.bsc"
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /pdb:"debug_nvfx/shader_nvfx.pdb" /debug /machine:I386 /out:"debug_nvfx/shader_nvfx.dll" /implib:"debug_nvfx/shader_nvfx.lib" /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 /nologo /subsystem:windows /dll /pdb:"debug_nvfx/shader_nvfx.pdb" /debug /machine:I386 /out:"debug_nvfx/shader_nvfx.dll" /implib:"debug_nvfx/shader_nvfx.lib" /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\debug_nvfx
TargetPath=.\debug_nvfx\shader_nvfx.dll
InputPath=.\debug_nvfx\shader_nvfx.dll
SOURCE="$(InputPath)"

"..\..\..\bin\shader_nvfx_ps20.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\shader_nvfx_ps20.dll attrib -r ..\..\..\bin\shader_nvfx_ps20.dll 
	copy $(TargetPath) ..\..\..\bin\shader_nvfx_ps20.dll 
	if exist $(TargetDir)\shader_nvfx_ps20.map copy $(TargetDir)\shader_nvfx.map ..\..\..\bin\shader_nvfx_ps20.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "shader_nvfx - Win32 Release"
# Name "shader_nvfx - Win32 Debug"
# Name "shader_nvfx - Win32 Release ps20"
# Name "shader_nvfx - Win32 Debug ps20"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
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

SOURCE=.\hsv.cpp
# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=.\refract.cpp
# End Source File
# Begin Source File

SOURCE=.\shadow.cpp
# End Source File
# Begin Source File

SOURCE=.\shatteredglass.cpp
# End Source File
# Begin Source File

SOURCE=.\sky.cpp
# End Source File
# Begin Source File

SOURCE=.\teeth.cpp
# End Source File
# Begin Source File

SOURCE=.\unlitgeneric.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\vmatrix.cpp
# End Source File
# Begin Source File

SOURCE=.\water.cpp
# End Source File
# Begin Source File

SOURCE=.\worldvertexalpha.cpp
# End Source File
# Begin Source File

SOURCE=.\worldvertextransition.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\common_fxc.h
# End Source File
# Begin Source File

SOURCE=.\common_hlsl_cpp_consts.h
# End Source File
# Begin Source File

SOURCE=.\common_ps_fxc.h
# End Source File
# Begin Source File

SOURCE=.\common_vs_fxc.h
# End Source File
# End Group
# Begin Group "Shaders"
# Begin Source File

SOURCE=.\hsv_ps20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__0=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\hsv_ps20.fxc
InputName=hsv_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__0=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\hsv_ps20.fxc
InputName=hsv_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__0=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\hsv_ps20.fxc
InputName=hsv_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__0=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\hsv_ps20.fxc
InputName=hsv_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\vertexlit_and_unlit_generic_vs20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__1=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_vs20.fxc
InputName=vertexlit_and_unlit_generic_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__1=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_vs20.fxc
InputName=vertexlit_and_unlit_generic_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__1=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_vs20.fxc
InputName=vertexlit_and_unlit_generic_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__1=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_vs20.fxc
InputName=vertexlit_and_unlit_generic_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\vertexlit_and_unlit_generic_ps20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__2=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_ps20.fxc
InputName=vertexlit_and_unlit_generic_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__2=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_ps20.fxc
InputName=vertexlit_and_unlit_generic_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__2=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_ps20.fxc
InputName=vertexlit_and_unlit_generic_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__2=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_ps20.fxc
InputName=vertexlit_and_unlit_generic_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_vs20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__3=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_vs20.fxc
InputName=lightmappedgeneric_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__3=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_vs20.fxc
InputName=lightmappedgeneric_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__3=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_vs20.fxc
InputName=lightmappedgeneric_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__3=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_vs20.fxc
InputName=lightmappedgeneric_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_ps20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__4=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_ps20.fxc
InputName=lightmappedgeneric_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__4=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_ps20.fxc
InputName=lightmappedgeneric_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__4=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_ps20.fxc
InputName=lightmappedgeneric_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__4=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_ps20.fxc
InputName=lightmappedgeneric_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\shadow_vs20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__5=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_vs20.fxc
InputName=shadow_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__5=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_vs20.fxc
InputName=shadow_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__5=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_vs20.fxc
InputName=shadow_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__5=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_vs20.fxc
InputName=shadow_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\shadow_ps20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__6=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_ps20.fxc
InputName=shadow_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__6=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_ps20.fxc
InputName=shadow_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__6=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_ps20.fxc
InputName=shadow_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__6=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_ps20.fxc
InputName=shadow_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\ShatteredGlass_ps20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__7=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_ps20.fxc
InputName=ShatteredGlass_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__7=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_ps20.fxc
InputName=ShatteredGlass_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__7=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_ps20.fxc
InputName=ShatteredGlass_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__7=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_ps20.fxc
InputName=ShatteredGlass_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\ShatteredGlass_vs20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__8=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_vs20.fxc
InputName=ShatteredGlass_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__8=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_vs20.fxc
InputName=ShatteredGlass_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__8=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_vs20.fxc
InputName=ShatteredGlass_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__8=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_vs20.fxc
InputName=ShatteredGlass_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\teeth_vs20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__9=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\teeth_vs20.fxc
InputName=teeth_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__9=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\teeth_vs20.fxc
InputName=teeth_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__9=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\teeth_vs20.fxc
InputName=teeth_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__9=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\teeth_vs20.fxc
InputName=teeth_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Water_vs20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__10=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_vs20.fxc
InputName=Water_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__10=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_vs20.fxc
InputName=Water_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__10=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_vs20.fxc
InputName=Water_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__10=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_vs20.fxc
InputName=Water_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Water_ps20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__11=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_ps20.fxc
InputName=Water_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__11=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_ps20.fxc
InputName=Water_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__11=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_ps20.fxc
InputName=Water_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__11=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_ps20.fxc
InputName=Water_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\screenspaceeffect_vs20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__12=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\screenspaceeffect_vs20.fxc
InputName=screenspaceeffect_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__12=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\screenspaceeffect_vs20.fxc
InputName=screenspaceeffect_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__12=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\screenspaceeffect_vs20.fxc
InputName=screenspaceeffect_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__12=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\screenspaceeffect_vs20.fxc
InputName=screenspaceeffect_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Refract_vs20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__13=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_vs20.fxc
InputName=Refract_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__13=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_vs20.fxc
InputName=Refract_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__13=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_vs20.fxc
InputName=Refract_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__13=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_vs20.fxc
InputName=Refract_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Refract_ps20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__14=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_ps20.fxc
InputName=Refract_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__14=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_ps20.fxc
InputName=Refract_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__14=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_ps20.fxc
InputName=Refract_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__14=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_ps20.fxc
InputName=Refract_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WaterCheap_vs20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__15=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_vs20.fxc
InputName=WaterCheap_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__15=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_vs20.fxc
InputName=WaterCheap_vs20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__15=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_vs20.fxc
InputName=WaterCheap_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__15=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_vs20.fxc
InputName=WaterCheap_vs20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WaterCheap_ps20.fxc
!IF  "$(CFG)" == "shader_nvfx - Win32 Release"
USERDEP__16=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_ps20.fxc
InputName=WaterCheap_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug"
USERDEP__16=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_ps20.fxc
InputName=WaterCheap_ps20

"fxctmp9_nv3x\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x -ps20a $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Release ps20"
USERDEP__16=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_ps20.fxc
InputName=WaterCheap_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "shader_nvfx - Win32 Debug ps20"
USERDEP__16=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_ps20.fxc
InputName=WaterCheap_ps20

"fxctmp9_nv3x_ps20\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 -nv3x $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\devtools\bin\fxc_prep.pl
# End Source File
# Begin Source File

SOURCE=.\shader_nvfx.cpp
# End Source File
# Begin Source File

SOURCE=.\shader_nvfx.txt
# End Source File
# Begin Source File

SOURCE=..\..\devtools\bin\updateshaders.pl
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
