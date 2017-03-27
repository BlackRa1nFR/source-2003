# Microsoft Developer Studio Project File - Name="stdshader_dx9" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=stdshader_dx9 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "stdshader_dx9.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "stdshader_dx9.mak" CFG="stdshader_dx9 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "stdshader_dx9 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "stdshader_dx9 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "stdshader_dx9"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_dx9"
# PROP BASE Intermediate_Dir "Release_dx9"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_dx9"
# PROP Intermediate_Dir "Release_dx9"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX9_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /I "fxctmp9" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX9_DLL_EXPORT" /D "_WIN32" /FD /c
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
TargetDir=.\Release_dx9
TargetPath=.\Release_dx9\stdshader_dx9.dll
InputPath=.\Release_dx9\stdshader_dx9.dll
SOURCE="$(InputPath)"

"..\..\..\bin\stdshader_dx9.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\stdshader_dx9.dll attrib -r ..\..\..\bin\stdshader_dx9.dll 
	copy $(TargetPath) ..\..\..\bin\stdshader_dx9.dll 
	if exist $(TargetDir)\stdshader_dx9.map copy $(TargetDir)\stdshader_dx9.map ..\..\..\bin\stdshader_dx9.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug_dx9"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug_dx9"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX9_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /I "fxctmp9" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "STDSHADER_DX9_DLL_EXPORT" /D "_WIN32" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug_dx9/stdshader_dx9.bsc"
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /pdb:"Debug_dx9/stdshader_dx9.pdb" /debug /machine:I386 /out:"Debug_dx9/stdshader_dx9.dll" /implib:"Debug_dx9/stdshader_dx9.lib" /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Debug_dx9
TargetPath=.\Debug_dx9\stdshader_dx9.dll
InputPath=.\Debug_dx9\stdshader_dx9.dll
SOURCE="$(InputPath)"

"..\..\..\bin\stdshader_dx9.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\stdshader_dx9.dll attrib -r ..\..\..\bin\stdshader_dx9.dll 
	copy $(TargetPath) ..\..\..\bin\stdshader_dx9.dll 
	if exist $(TargetDir)\stdshader_dx9.map copy $(TargetDir)\stdshader_dx9.map ..\..\..\bin\stdshader_dx9.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "stdshader_dx9 - Win32 Release"
# Name "stdshader_dx9 - Win32 Debug"
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
# ADD CPP /I "fxctmp9"
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
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__0=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\hsv_ps20.fxc
InputName=hsv_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__0=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\hsv_ps20.fxc
InputName=hsv_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\vertexlit_and_unlit_generic_vs20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__1=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_vs20.fxc
InputName=vertexlit_and_unlit_generic_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__1=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_vs20.fxc
InputName=vertexlit_and_unlit_generic_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\vertexlit_and_unlit_generic_ps20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__2=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_ps20.fxc
InputName=vertexlit_and_unlit_generic_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__2=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\vertexlit_and_unlit_generic_ps20.fxc
InputName=vertexlit_and_unlit_generic_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_vs20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__3=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_vs20.fxc
InputName=lightmappedgeneric_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__3=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_vs20.fxc
InputName=lightmappedgeneric_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\lightmappedgeneric_ps20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__4=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_ps20.fxc
InputName=lightmappedgeneric_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__4=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\lightmappedgeneric_ps20.fxc
InputName=lightmappedgeneric_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\shadow_vs20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__5=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_vs20.fxc
InputName=shadow_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__5=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_vs20.fxc
InputName=shadow_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\shadow_ps20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__6=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_ps20.fxc
InputName=shadow_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__6=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\shadow_ps20.fxc
InputName=shadow_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\ShatteredGlass_ps20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__7=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_ps20.fxc
InputName=ShatteredGlass_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__7=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_ps20.fxc
InputName=ShatteredGlass_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\ShatteredGlass_vs20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__8=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_vs20.fxc
InputName=ShatteredGlass_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__8=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\ShatteredGlass_vs20.fxc
InputName=ShatteredGlass_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\teeth_vs20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__9=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\teeth_vs20.fxc
InputName=teeth_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__9=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\teeth_vs20.fxc
InputName=teeth_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Water_vs20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__10=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_vs20.fxc
InputName=Water_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__10=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_vs20.fxc
InputName=Water_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Water_ps20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__11=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_ps20.fxc
InputName=Water_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__11=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Water_ps20.fxc
InputName=Water_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\screenspaceeffect_vs20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__12=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\screenspaceeffect_vs20.fxc
InputName=screenspaceeffect_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__12=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\screenspaceeffect_vs20.fxc
InputName=screenspaceeffect_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Refract_vs20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__13=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_vs20.fxc
InputName=Refract_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__13=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_vs20.fxc
InputName=Refract_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\Refract_ps20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__14=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_ps20.fxc
InputName=Refract_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__14=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\Refract_ps20.fxc
InputName=Refract_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WaterCheap_vs20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__15=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_vs20.fxc
InputName=WaterCheap_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__15=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_vs20.fxc
InputName=WaterCheap_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WaterCheap_ps20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__16=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_ps20.fxc
InputName=WaterCheap_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__16=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WaterCheap_ps20.fxc
InputName=WaterCheap_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldVertexTransition_vs20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__17=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_vs20.fxc
InputName=WorldVertexTransition_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__17=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_vs_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_vs20.fxc
InputName=WorldVertexTransition_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# Begin Source File

SOURCE=.\WorldVertexTransition_ps20.fxc
!IF  "$(CFG)" == "stdshader_dx9 - Win32 Release"
USERDEP__18=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_ps20.fxc
InputName=WorldVertexTransition_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ELSEIF  "$(CFG)" == "stdshader_dx9 - Win32 Debug"
USERDEP__18=	"common_fxc.h"	"common_hlsl_cpp_consts.h"	"common_ps_fxc.h"	"..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"
# Begin Custom Build
InputPath=.\WorldVertexTransition_ps20.fxc
InputName=WorldVertexTransition_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build
!ENDIF
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\devtools\bin\fxc_prep.pl
# End Source File
# Begin Source File

SOURCE=.\stdshader_dx9.cpp
# End Source File
# Begin Source File

SOURCE=.\stdshader_dx9.txt
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
