# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=QMAPEXP - Win32 Hybrid
!MESSAGE No configuration specified.  Defaulting to QMAPEXP - Win32 Hybrid.
!ENDIF 

!IF "$(CFG)" != "QMAPEXP - Win32 Release" && "$(CFG)" !=\
 "QMAPEXP - Win32 Hybrid"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "QMAPEXP.MAK" CFG="QMAPEXP - Win32 Hybrid"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "QMAPEXP - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "QMAPEXP - Win32 Hybrid" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "QMAPEXP - Win32 Hybrid"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "QMAPEXP - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : ".\QMAPEXP.dle"

CLEAN : 
	-@erase "$(INTDIR)\QMAPEXP.obj"
	-@erase "$(INTDIR)\QMAPEXP.res"
	-@erase "$(OUTDIR)\QMAPEXP.exp"
	-@erase "$(OUTDIR)\QMAPEXP.lib"
	-@erase ".\QMAPEXP.dle"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /c
# SUBTRACT CPP /YX
CPP_PROJ=/nologo /G5 /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG"\
 /D "_WINDOWS" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/QMAPEXP.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/QMAPEXP.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386 /out:"QMAPEXP.dle"
LINK32_FLAGS=comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/QMAPEXP.pdb"\
 /machine:I386 /def:".\QMAPEXP.def" /out:"QMAPEXP.dle"\
 /implib:"$(OUTDIR)/QMAPEXP.lib" 
DEF_FILE= \
	".\QMAPEXP.def"
LINK32_OBJS= \
	"$(INTDIR)\QMAPEXP.obj" \
	"$(INTDIR)\QMAPEXP.res" \
	"C:\USERS\Phil\MAXSDK\LIB\CORE.LIB" \
	"C:\USERS\Phil\MAXSDK\LIB\GEOM.LIB" \
	"C:\USERS\Phil\MAXSDK\LIB\GFX.LIB" \
	"C:\USERS\Phil\MAXSDK\LIB\MESH.LIB" \
	"C:\USERS\Phil\MAXSDK\LIB\UTIL.LIB"

".\QMAPEXP.dle" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "QMAPEXP - Win32 Hybrid"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Hybrid"
# PROP BASE Intermediate_Dir "Hybrid"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Hybrid"
# PROP Intermediate_Dir "Hybrid"
# PROP Target_Dir ""
OUTDIR=.\Hybrid
INTDIR=.\Hybrid

ALL : ".\QMAPEXP.dle"

CLEAN : 
	-@erase "$(INTDIR)\QMAPEXP.obj"
	-@erase "$(INTDIR)\QMAPEXP.res"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\QMAPEXP.exp"
	-@erase "$(OUTDIR)\QMAPEXP.lib"
	-@erase "$(OUTDIR)\QMAPEXP.pdb"
	-@erase ".\QMAPEXP.dle"
	-@erase ".\QMAPEXP.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /G5 /MDd /W3 /Gm /GX /Zi /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G5 /MD /W3 /Gm /GX /Zi /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /c
# SUBTRACT CPP /YX
CPP_PROJ=/nologo /G5 /MD /W3 /Gm /GX /Zi /Od /I "..\..\include" /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Hybrid/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/QMAPEXP.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/QMAPEXP.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\..\..\exe\stdplugs\QMAPEXP.dle"
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"QMAPEXP.dle"
LINK32_FLAGS=comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/QMAPEXP.pdb" /debug\
 /machine:I386 /def:".\QMAPEXP.def" /out:"QMAPEXP.dle"\
 /implib:"$(OUTDIR)/QMAPEXP.lib" 
DEF_FILE= \
	".\QMAPEXP.def"
LINK32_OBJS= \
	"$(INTDIR)\QMAPEXP.obj" \
	"$(INTDIR)\QMAPEXP.res" \
	"C:\USERS\Phil\MAXSDK\LIB\CORE.LIB" \
	"C:\USERS\Phil\MAXSDK\LIB\GEOM.LIB" \
	"C:\USERS\Phil\MAXSDK\LIB\GFX.LIB" \
	"C:\USERS\Phil\MAXSDK\LIB\MESH.LIB" \
	"C:\USERS\Phil\MAXSDK\LIB\UTIL.LIB"

".\QMAPEXP.dle" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "QMAPEXP - Win32 Release"
# Name "QMAPEXP - Win32 Hybrid"

!IF  "$(CFG)" == "QMAPEXP - Win32 Release"

!ELSEIF  "$(CFG)" == "QMAPEXP - Win32 Hybrid"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\QMAPEXP.rc

"$(INTDIR)\QMAPEXP.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\QMAPEXP.cpp

!IF  "$(CFG)" == "QMAPEXP - Win32 Release"

DEP_CPP_QMAPE=\
	"..\QEntHlp\qhelpers.h"\
	"C:\USERS\Phil\MAXSDK\include\acolor.h"\
	"C:\USERS\Phil\MAXSDK\include\animtbl.h"\
	"C:\USERS\Phil\MAXSDK\include\assert1.h"\
	"C:\USERS\Phil\MAXSDK\include\bitarray.h"\
	"C:\USERS\Phil\MAXSDK\include\box2.h"\
	"C:\USERS\Phil\MAXSDK\include\box3.h"\
	"C:\USERS\Phil\MAXSDK\include\channels.h"\
	"C:\USERS\Phil\MAXSDK\include\cmdmode.h"\
	"C:\USERS\Phil\MAXSDK\include\color.h"\
	"C:\USERS\Phil\MAXSDK\include\control.h"\
	"C:\USERS\Phil\MAXSDK\include\coreexp.h"\
	"C:\USERS\Phil\MAXSDK\include\custcont.h"\
	"C:\USERS\Phil\MAXSDK\include\dbgprint.h"\
	"C:\USERS\Phil\MAXSDK\include\dpoint3.h"\
	"C:\USERS\Phil\MAXSDK\include\evuser.h"\
	"C:\USERS\Phil\MAXSDK\include\gencam.h"\
	"C:\USERS\Phil\MAXSDK\include\genhier.h"\
	"C:\USERS\Phil\MAXSDK\include\genlight.h"\
	"C:\USERS\Phil\MAXSDK\include\genshape.h"\
	"C:\USERS\Phil\MAXSDK\include\geom.h"\
	"C:\USERS\Phil\MAXSDK\include\geomlib.h"\
	"C:\USERS\Phil\MAXSDK\include\gfx.h"\
	"C:\USERS\Phil\MAXSDK\include\gfxlib.h"\
	"C:\USERS\Phil\MAXSDK\include\hold.h"\
	"C:\USERS\Phil\MAXSDK\include\impapi.h"\
	"C:\USERS\Phil\MAXSDK\include\impexp.h"\
	"C:\USERS\Phil\MAXSDK\include\imtl.h"\
	"C:\USERS\Phil\MAXSDK\include\inode.h"\
	"C:\USERS\Phil\MAXSDK\include\interval.h"\
	"C:\USERS\Phil\MAXSDK\include\iparamb.h"\
	"C:\USERS\Phil\MAXSDK\include\ipoint2.h"\
	"C:\USERS\Phil\MAXSDK\include\ipoint3.h"\
	"C:\USERS\Phil\MAXSDK\include\lockid.h"\
	"C:\USERS\Phil\MAXSDK\include\matrix2.h"\
	"C:\USERS\Phil\MAXSDK\include\matrix3.h"\
	"C:\USERS\Phil\MAXSDK\include\maxapi.h"\
	"C:\USERS\Phil\MAXSDK\include\maxcom.h"\
	"C:\USERS\Phil\MAXSDK\include\maxtypes.h"\
	"C:\USERS\Phil\MAXSDK\include\mesh.h"\
	"C:\USERS\Phil\MAXSDK\include\meshlib.h"\
	"C:\USERS\Phil\MAXSDK\include\mouseman.h"\
	"C:\USERS\Phil\MAXSDK\include\mtl.h"\
	"C:\USERS\Phil\MAXSDK\include\nametab.h"\
	"C:\USERS\Phil\MAXSDK\include\nurbs.h"\
	"C:\USERS\Phil\MAXSDK\include\nurbslib.h"\
	"C:\USERS\Phil\MAXSDK\include\nurbsobj.h"\
	"C:\USERS\Phil\MAXSDK\include\object.h"\
	"C:\USERS\Phil\MAXSDK\include\objmode.h"\
	"C:\USERS\Phil\MAXSDK\include\patch.h"\
	"C:\USERS\Phil\MAXSDK\include\patchlib.h"\
	"C:\USERS\Phil\MAXSDK\include\patchobj.h"\
	"C:\USERS\Phil\MAXSDK\include\plugin.h"\
	"C:\USERS\Phil\MAXSDK\include\point2.h"\
	"C:\USERS\Phil\MAXSDK\include\point3.h"\
	"C:\USERS\Phil\MAXSDK\include\ptrvec.h"\
	"C:\USERS\Phil\MAXSDK\include\quat.h"\
	"C:\USERS\Phil\MAXSDK\include\ref.h"\
	"C:\USERS\Phil\MAXSDK\include\render.h"\
	"C:\USERS\Phil\MAXSDK\include\rtclick.h"\
	"C:\USERS\Phil\MAXSDK\include\sceneapi.h"\
	"C:\USERS\Phil\MAXSDK\include\snap.h"\
	"C:\USERS\Phil\MAXSDK\include\soundobj.h"\
	"C:\USERS\Phil\MAXSDK\include\stack3.h"\
	"C:\USERS\Phil\MAXSDK\include\strclass.h"\
	"C:\USERS\Phil\MAXSDK\include\tab.h"\
	"C:\USERS\Phil\MAXSDK\include\trig.h"\
	"C:\USERS\Phil\MAXSDK\include\triobj.h"\
	"C:\USERS\Phil\MAXSDK\include\units.h"\
	"C:\USERS\Phil\MAXSDK\include\utilexp.h"\
	"C:\USERS\Phil\MAXSDK\include\utillib.h"\
	"C:\USERS\Phil\MAXSDK\include\vedge.h"\
	"C:\USERS\Phil\MAXSDK\include\winutil.h"\
	{$(INCLUDE)}"\..\include\Max.h"\
	{$(INCLUDE)}"\..\include\StdMat.h"\
	{$(INCLUDE)}"\export.h"\
	{$(INCLUDE)}"\hitdata.h"\
	{$(INCLUDE)}"\ioapi.h"\
	{$(INCLUDE)}"\plugapi.h"\
	{$(INCLUDE)}"\strbasic.h"\
	

"$(INTDIR)\QMAPEXP.obj" : $(SOURCE) $(DEP_CPP_QMAPE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "QMAPEXP - Win32 Hybrid"

DEP_CPP_QMAPE=\
	"..\QEntHlp\qhelpers.h"\
	"C:\USERS\Phil\MAXSDK\include\acolor.h"\
	"C:\USERS\Phil\MAXSDK\include\animtbl.h"\
	"C:\USERS\Phil\MAXSDK\include\assert1.h"\
	"C:\USERS\Phil\MAXSDK\include\bitarray.h"\
	"C:\USERS\Phil\MAXSDK\include\box2.h"\
	"C:\USERS\Phil\MAXSDK\include\box3.h"\
	"C:\USERS\Phil\MAXSDK\include\channels.h"\
	"C:\USERS\Phil\MAXSDK\include\cmdmode.h"\
	"C:\USERS\Phil\MAXSDK\include\color.h"\
	"C:\USERS\Phil\MAXSDK\include\control.h"\
	"C:\USERS\Phil\MAXSDK\include\coreexp.h"\
	"C:\USERS\Phil\MAXSDK\include\custcont.h"\
	"C:\USERS\Phil\MAXSDK\include\dbgprint.h"\
	"C:\USERS\Phil\MAXSDK\include\dpoint3.h"\
	"C:\USERS\Phil\MAXSDK\include\evuser.h"\
	"C:\USERS\Phil\MAXSDK\include\genhier.h"\
	"C:\USERS\Phil\MAXSDK\include\genshape.h"\
	"C:\USERS\Phil\MAXSDK\include\geom.h"\
	"C:\USERS\Phil\MAXSDK\include\geomlib.h"\
	"C:\USERS\Phil\MAXSDK\include\gfx.h"\
	"C:\USERS\Phil\MAXSDK\include\gfxlib.h"\
	"C:\USERS\Phil\MAXSDK\include\hold.h"\
	"C:\USERS\Phil\MAXSDK\include\imtl.h"\
	"C:\USERS\Phil\MAXSDK\include\inode.h"\
	"C:\USERS\Phil\MAXSDK\include\interval.h"\
	"C:\USERS\Phil\MAXSDK\include\ipoint2.h"\
	"C:\USERS\Phil\MAXSDK\include\ipoint3.h"\
	"C:\USERS\Phil\MAXSDK\include\lockid.h"\
	"C:\USERS\Phil\MAXSDK\include\matrix2.h"\
	"C:\USERS\Phil\MAXSDK\include\matrix3.h"\
	"C:\USERS\Phil\MAXSDK\include\maxapi.h"\
	"C:\USERS\Phil\MAXSDK\include\maxcom.h"\
	"C:\USERS\Phil\MAXSDK\include\maxtypes.h"\
	"C:\USERS\Phil\MAXSDK\include\mesh.h"\
	"C:\USERS\Phil\MAXSDK\include\meshlib.h"\
	"C:\USERS\Phil\MAXSDK\include\mouseman.h"\
	"C:\USERS\Phil\MAXSDK\include\mtl.h"\
	"C:\USERS\Phil\MAXSDK\include\nametab.h"\
	"C:\USERS\Phil\MAXSDK\include\nurbs.h"\
	"C:\USERS\Phil\MAXSDK\include\nurbslib.h"\
	"C:\USERS\Phil\MAXSDK\include\object.h"\
	"C:\USERS\Phil\MAXSDK\include\patch.h"\
	"C:\USERS\Phil\MAXSDK\include\patchlib.h"\
	"C:\USERS\Phil\MAXSDK\include\plugin.h"\
	"C:\USERS\Phil\MAXSDK\include\point2.h"\
	"C:\USERS\Phil\MAXSDK\include\point3.h"\
	"C:\USERS\Phil\MAXSDK\include\ptrvec.h"\
	"C:\USERS\Phil\MAXSDK\include\quat.h"\
	"C:\USERS\Phil\MAXSDK\include\ref.h"\
	"C:\USERS\Phil\MAXSDK\include\rtclick.h"\
	"C:\USERS\Phil\MAXSDK\include\sceneapi.h"\
	"C:\USERS\Phil\MAXSDK\include\snap.h"\
	"C:\USERS\Phil\MAXSDK\include\stack3.h"\
	"C:\USERS\Phil\MAXSDK\include\strclass.h"\
	"C:\USERS\Phil\MAXSDK\include\tab.h"\
	"C:\USERS\Phil\MAXSDK\include\trig.h"\
	"C:\USERS\Phil\MAXSDK\include\units.h"\
	"C:\USERS\Phil\MAXSDK\include\utilexp.h"\
	"C:\USERS\Phil\MAXSDK\include\utillib.h"\
	"C:\USERS\Phil\MAXSDK\include\vedge.h"\
	"C:\USERS\Phil\MAXSDK\include\winutil.h"\
	{$(INCLUDE)}"\..\include\Max.h"\
	{$(INCLUDE)}"\..\include\StdMat.h"\
	{$(INCLUDE)}"\export.h"\
	{$(INCLUDE)}"\hitdata.h"\
	{$(INCLUDE)}"\ioapi.h"\
	{$(INCLUDE)}"\plugapi.h"\
	{$(INCLUDE)}"\strbasic.h"\
	

"$(INTDIR)\QMAPEXP.obj" : $(SOURCE) $(DEP_CPP_QMAPE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\QMAPEXP.def

!IF  "$(CFG)" == "QMAPEXP - Win32 Release"

!ELSEIF  "$(CFG)" == "QMAPEXP - Win32 Hybrid"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=C:\USERS\Phil\MAXSDK\LIB\CORE.LIB

!IF  "$(CFG)" == "QMAPEXP - Win32 Release"

!ELSEIF  "$(CFG)" == "QMAPEXP - Win32 Hybrid"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=C:\USERS\Phil\MAXSDK\LIB\GEOM.LIB

!IF  "$(CFG)" == "QMAPEXP - Win32 Release"

!ELSEIF  "$(CFG)" == "QMAPEXP - Win32 Hybrid"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=C:\USERS\Phil\MAXSDK\LIB\GFX.LIB

!IF  "$(CFG)" == "QMAPEXP - Win32 Release"

!ELSEIF  "$(CFG)" == "QMAPEXP - Win32 Hybrid"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=C:\USERS\Phil\MAXSDK\LIB\MESH.LIB

!IF  "$(CFG)" == "QMAPEXP - Win32 Release"

!ELSEIF  "$(CFG)" == "QMAPEXP - Win32 Hybrid"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=C:\USERS\Phil\MAXSDK\LIB\UTIL.LIB

!IF  "$(CFG)" == "QMAPEXP - Win32 Release"

!ELSEIF  "$(CFG)" == "QMAPEXP - Win32 Hybrid"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
