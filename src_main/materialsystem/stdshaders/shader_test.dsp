# Microsoft Developer Studio Project File - Name="shader_test" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=shader_test - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "shader_test.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "shader_test.mak" CFG="shader_test - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "shader_test - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "shader_test - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "shader_test"
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "shader_test - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "shader_test___Win32_Release"
# PROP BASE Intermediate_Dir "shader_test___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "shader_test___Win32_Release"
# PROP Intermediate_Dir "shader_test___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "SHADER_TEST_DLL_EXPORT" /D "_WIN32" /FD /c
# ADD CPP /nologo /G6 /W4 /Zi /Ox /Ot /Ow /Og /Oi /Op /Gf /Gy /I "..\..\common" /I "..\..\public" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "SHADER_TEST_DLL_EXPORT" /D "_WIN32" /Fo"Release_test/" /Fd"Release_test/" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Release_test/shader_test.bsc"
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /pdb:"Release_test/shader_test.pdb" /machine:I386 /out:"Release_test/shader_test.dll" /implib:"Release_test/shader_test.lib" /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Release_test
TargetPath=.\Release_test\shader_test.dll
InputPath=.\Release_test\shader_test.dll
SOURCE="$(InputPath)"

"..\..\..\bin\shader_test.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\shader_test.dll attrib -r ..\..\..\bin\shader_test.dll 
	copy $(TargetPath) ..\..\..\bin\shader_test.dll 
	if exist $(TargetDir)\shader_test.map copy $(TargetDir)\shader_test.map ..\..\..\bin\shader_test.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "shader_test - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "shader_test___Win32_Debug"
# PROP BASE Intermediate_Dir "shader_test___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "shader_test___Win32_Debug"
# PROP Intermediate_Dir "shader_test___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "SHADER_TEST_DLL_EXPORT" /D "_WIN32" /FR"Debug/" /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /Op /I "..\..\common" /I "..\..\public" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D fopen=dont_use_fopen /D "SHADER_TEST_DLL_EXPORT" /D "_WIN32" /FR"Debug_test/" /Fo"Debug_test/" /Fd"Debug_test/" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug_test/shader_test.bsc"
LINK32=link.exe
# ADD BASE LINK32 tier0.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# ADD LINK32 /nologo /subsystem:windows /dll /pdb:"Debug_test/shader_test.pdb" /debug /machine:I386 /out:"Debug_test/shader_test.dll" /implib:"Debug_test/shader_test.lib" /pdbtype:sept /libpath:"..\..\lib\common\\" /libpath:"..\..\lib\public\\"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Publishing to target directory (..\..\..\bin\)...
TargetDir=.\Debug_test
TargetPath=.\Debug_test\shader_test.dll
InputPath=.\Debug_test\shader_test.dll
SOURCE="$(InputPath)"

"..\..\..\bin\shader_test.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\shader_test.dll attrib -r ..\..\..\bin\shader_test.dll 
	copy $(TargetPath) ..\..\..\bin\shader_test.dll 
	if exist $(TargetDir)\shader_test.map copy $(TargetDir)\shader_test.map ..\..\..\bin\shader_test.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "shader_test - Win32 Release"
# Name "shader_test - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BaseVSShader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=.\vertexlitgeneric.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\vmatrix.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Shaders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\VertexLit_and_unlit_Generic_test_ps20.fxc

!IF  "$(CFG)" == "shader_test - Win32 Release"

USERDEP__VERTE="..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"	
# Begin Custom Build
InputPath=.\VertexLit_and_unlit_Generic_test_ps20.fxc
InputName=VertexLit_and_unlit_Generic_test_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build

!ELSEIF  "$(CFG)" == "shader_test - Win32 Debug"

USERDEP__VERTE="..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"	
# Begin Custom Build
InputPath=.\VertexLit_and_unlit_Generic_test_ps20.fxc
InputName=VertexLit_and_unlit_Generic_test_ps20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\VertexLit_and_unlit_Generic_test_vs20.fxc

!IF  "$(CFG)" == "shader_test - Win32 Release"

USERDEP__VERTEX="..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"	
# Begin Custom Build
InputPath=.\VertexLit_and_unlit_Generic_test_vs20.fxc
InputName=VertexLit_and_unlit_Generic_test_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build

!ELSEIF  "$(CFG)" == "shader_test - Win32 Debug"

USERDEP__VERTEX="..\..\devtools\bin\fxc.exe"	"..\..\devtools\bin\fxc_prep.pl"	
# Begin Custom Build
InputPath=.\VertexLit_and_unlit_Generic_test_vs20.fxc
InputName=VertexLit_and_unlit_Generic_test_vs20

"fxctmp9\$(InputName).inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\devtools\bin\perl ..\..\devtools\bin\fxc_prep.pl -dx9 $(InputName).fxc

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\shader_test.cpp
# End Source File
# Begin Source File

SOURCE=.\shader_test.txt
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\shaderlib.lib
# End Source File
# End Target
# End Project
