# Microsoft Developer Studio Project File - Name="vview" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=vview - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vview.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vview.mak" CFG="vview - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vview - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "vview - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/utils/vview", VEVBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vview - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /O2 /I "..\common" /I "..\..\Public" /I "..\..\common" /D "_WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 opengl32.lib winmm.lib s3tc.lib /nologo /stack:0x200000 /subsystem:windows /machine:I386 /libpath:"..\..\..\src\lib\public" /libpath:"..\..\..\src\lib\common"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying to bin...
TargetDir=.\Release
TargetPath=.\Release\vview.exe
TargetName=vview
InputPath=.\Release\vview.exe
SOURCE="$(InputPath)"

"..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\$(TargetName).exe attrib -r ..\..\..\bin\$(TargetName).exe 
	copy $(TargetPath) ..\..\..\bin 
	if exist $(TargetDir)\vview.map copy $(TargetDir)\vview.map ..\..\..\bin\vview.map 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vview - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /W4 /Gm /ZI /Od /I "..\common" /I "..\..\Public" /I "..\..\common" /D "_WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 opengl32.lib winmm.lib s3tc.lib /nologo /stack:0x200000 /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\src\lib\public" /libpath:"..\..\..\src\lib\common"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Copying
TargetDir=.\Debug
TargetPath=.\Debug\vview.exe
TargetName=vview
InputPath=.\Debug\vview.exe
SOURCE="$(InputPath)"

"..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\..\bin\$(TargetName).exe attrib -r ..\..\..\bin\$(TargetName).exe 
	copy $(TargetPath) ..\..\..\bin 
	if exist $(TargetDir)\vview.map copy $(TargetDir)\vview.map ..\..\..\bin\vview.map 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "vview - Win32 Release"
# Name "vview - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\bsprender.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.cpp
# End Source File
# Begin Source File

SOURCE=.\cmodel.cpp
# End Source File
# Begin Source File

SOURCE=.\engine.cpp
# End Source File
# Begin Source File

SOURCE=.\font_gl.cpp
# End Source File
# Begin Source File

SOURCE=.\gl_image.cpp
# End Source File
# Begin Source File

SOURCE=.\gl_model.cpp
# End Source File
# Begin Source File

SOURCE=.\gl_rsurf.cpp
# End Source File
# Begin Source File

SOURCE=.\glapp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\ImageLoader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\Mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\common\polylib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\TGALoader.cpp
# End Source File
# Begin Source File

SOURCE=..\common\threads.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\vview.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\BSPFILE.H
# End Source File
# Begin Source File

SOURCE=..\..\Public\bspflags.h
# End Source File
# Begin Source File

SOURCE=.\bsprender.h
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.h
# End Source File
# Begin Source File

SOURCE=.\cmodel.h
# End Source File
# Begin Source File

SOURCE=.\cmodel_engine.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\commonmacros.h
# End Source File
# Begin Source File

SOURCE=.\engine.h
# End Source File
# Begin Source File

SOURCE=.\font_gl.h
# End Source File
# Begin Source File

SOURCE=.\gl_image.h
# End Source File
# Begin Source File

SOURCE=.\gl_model.h
# End Source File
# Begin Source File

SOURCE=.\gl_rsurf.h
# End Source File
# Begin Source File

SOURCE=.\glapp.h
# End Source File
# Begin Source File

SOURCE=.\map_entity.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\MATHLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\Public\plane.h
# End Source File
# Begin Source File

SOURCE=..\common\polylib.h
# End Source File
# Begin Source File

SOURCE=..\common\qfiles.h
# End Source File
# Begin Source File

SOURCE=..\common\threads.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\wadtypes.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\worldsize.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
