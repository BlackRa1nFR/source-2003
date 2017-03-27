# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=HLMaster - Win32 Release
!MESSAGE No configuration specified.  Defaulting to HLMaster - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "HLMaster - Win32 Release" && "$(CFG)" !=\
 "HLMaster - Win32 Debug" && "$(CFG)" != "MasterTest - Win32 Release" &&\
 "$(CFG)" != "MasterTest - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "HLModMaster.mak" CFG="HLMaster - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HLMaster - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "HLMaster - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "MasterTest - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MasterTest - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "HLMaster - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "HLMaster - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\HLModMaster.exe" ".\hlmaster.exe"

CLEAN : 
	-@erase "$(INTDIR)\HLModMaster.obj"
	-@erase "$(INTDIR)\HLModMaster.res"
	-@erase "$(INTDIR)\HLModMasterDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\Token.obj"
	-@erase "$(OUTDIR)\HLModMaster.exe"
	-@erase ".\hlmaster.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_MBCS" /Fp"$(INTDIR)/HLModMaster.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/HLModMaster.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/HLModMaster.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 wsock32.lib /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=wsock32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/HLModMaster.pdb" /machine:I386 /out:"$(OUTDIR)/HLModMaster.exe"\
 
LINK32_OBJS= \
	"$(INTDIR)\HLModMaster.obj" \
	"$(INTDIR)\HLModMaster.res" \
	"$(INTDIR)\HLModMasterDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\Token.obj"

"$(OUTDIR)\HLModMaster.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
TargetPath=.\Release\HLModMaster.exe
InputPath=.\Release\HLModMaster.exe
SOURCE=$(InputPath)

"hlmaster.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   copy "$(TargetPath)" d:\quiver\hlmaster.exe

# End Custom Build

!ELSEIF  "$(CFG)" == "HLMaster - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\HLModMaster.exe" ".\hlmaster.exe"

CLEAN : 
	-@erase "$(INTDIR)\HLModMaster.obj"
	-@erase "$(INTDIR)\HLModMaster.res"
	-@erase "$(INTDIR)\HLModMasterDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\Token.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\HLModMaster.exe"
	-@erase "$(OUTDIR)\HLModMaster.ilk"
	-@erase "$(OUTDIR)\HLModMaster.pdb"
	-@erase ".\hlmaster.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_MBCS" /Fp"$(INTDIR)/HLModMaster.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/"\
 /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/HLModMaster.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/HLModMaster.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 wsock32.lib /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=wsock32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/HLModMaster.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/HLModMaster.exe" 
LINK32_OBJS= \
	"$(INTDIR)\HLModMaster.obj" \
	"$(INTDIR)\HLModMaster.res" \
	"$(INTDIR)\HLModMasterDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\Token.obj"

"$(OUTDIR)\HLModMaster.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
TargetPath=.\Debug\HLModMaster.exe
InputPath=.\Debug\HLModMaster.exe
SOURCE=$(InputPath)

"hlmaster.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   copy "$(TargetPath)" d:\quiver\hlmaster.exe

# End Custom Build

!ELSEIF  "$(CFG)" == "MasterTest - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MasterTest\Release"
# PROP BASE Intermediate_Dir "MasterTest\Release"
# PROP BASE Target_Dir "MasterTest"
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MasterTest\Release"
# PROP Intermediate_Dir "MasterTest\Release"
# PROP Target_Dir "MasterTest"
OUTDIR=.\MasterTest\Release
INTDIR=.\MasterTest\Release

ALL : "$(OUTDIR)\MasterTest.exe"

CLEAN : 
	-@erase "$(INTDIR)\HLMasterAsyncSocket.obj"
	-@erase "$(INTDIR)\MasterTest.obj"
	-@erase "$(INTDIR)\MasterTest.pch"
	-@erase "$(INTDIR)\MasterTest.res"
	-@erase "$(INTDIR)\MasterTestDlg.obj"
	-@erase "$(INTDIR)\MessageBuffer.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\UTIL.obj"
	-@erase "$(OUTDIR)\MasterTest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_MBCS" /Fp"$(INTDIR)/MasterTest.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\MasterTest\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/MasterTest.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/MasterTest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/MasterTest.pdb" /machine:I386 /out:"$(OUTDIR)/MasterTest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\HLMasterAsyncSocket.obj" \
	"$(INTDIR)\MasterTest.obj" \
	"$(INTDIR)\MasterTest.res" \
	"$(INTDIR)\MasterTestDlg.obj" \
	"$(INTDIR)\MessageBuffer.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\UTIL.obj"

"$(OUTDIR)\MasterTest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MasterTest - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MasterTest\Debug"
# PROP BASE Intermediate_Dir "MasterTest\Debug"
# PROP BASE Target_Dir "MasterTest"
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MasterTest\Debug"
# PROP Intermediate_Dir "MasterTest\Debug"
# PROP Target_Dir "MasterTest"
OUTDIR=.\MasterTest\Debug
INTDIR=.\MasterTest\Debug

ALL : "$(OUTDIR)\MasterTest.exe" ".\cms.exe"

CLEAN : 
	-@erase "$(INTDIR)\HLMasterAsyncSocket.obj"
	-@erase "$(INTDIR)\MasterTest.obj"
	-@erase "$(INTDIR)\MasterTest.pch"
	-@erase "$(INTDIR)\MasterTest.res"
	-@erase "$(INTDIR)\MasterTestDlg.obj"
	-@erase "$(INTDIR)\MessageBuffer.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\UTIL.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\MasterTest.exe"
	-@erase "$(OUTDIR)\MasterTest.ilk"
	-@erase "$(OUTDIR)\MasterTest.pdb"
	-@erase ".\cms.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_MBCS" /Fp"$(INTDIR)/MasterTest.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\MasterTest\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/MasterTest.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/MasterTest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/MasterTest.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/MasterTest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\HLMasterAsyncSocket.obj" \
	"$(INTDIR)\MasterTest.obj" \
	"$(INTDIR)\MasterTest.res" \
	"$(INTDIR)\MasterTestDlg.obj" \
	"$(INTDIR)\MessageBuffer.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\UTIL.obj"

"$(OUTDIR)\MasterTest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
TargetPath=.\MasterTest\Debug\MasterTest.exe
InputPath=.\MasterTest\Debug\MasterTest.exe
SOURCE=$(InputPath)

"cms.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   attrib -r d:\quiver\cms.exe
   copy "$(TargetPath)" d:\quiver\cms.exe

# End Custom Build

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

# Name "HLMaster - Win32 Release"
# Name "HLMaster - Win32 Debug"

!IF  "$(CFG)" == "HLMaster - Win32 Release"

!ELSEIF  "$(CFG)" == "HLMaster - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\StdAfx.h

!IF  "$(CFG)" == "HLMaster - Win32 Release"

!ELSEIF  "$(CFG)" == "HLMaster - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Resource.h

!IF  "$(CFG)" == "HLMaster - Win32 Release"

!ELSEIF  "$(CFG)" == "HLMaster - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Token.cpp
DEP_CPP_TOKEN=\
	".\ClientMaster\..\Token.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\Token.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\HLModMasterDlg.cpp
DEP_CPP_HLMOD=\
	".\ClientMaster\..\Token.h"\
	".\HLModMaster.h"\
	".\HLModMasterDlg.h"\
	".\HLModMasterProtocol.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\HLModMasterDlg.obj" : $(SOURCE) $(DEP_CPP_HLMOD) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\HLModMaster.cpp
DEP_CPP_HLMODM=\
	".\HLModMaster.h"\
	".\HLModMasterDlg.h"\
	".\HLModMasterProtocol.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\HLModMaster.obj" : $(SOURCE) $(DEP_CPP_HLMODM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\HLModMaster.rc
DEP_RSC_HLMODMA=\
	".\res\HLMaster.ico"\
	".\res\HLMaster.rc2"\
	

"$(INTDIR)\HLModMaster.res" : $(SOURCE) $(DEP_RSC_HLMODMA) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\HLModMasterProtocol.h

!IF  "$(CFG)" == "HLMaster - Win32 Release"

!ELSEIF  "$(CFG)" == "HLMaster - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\HLModMasterDlg.h

!IF  "$(CFG)" == "HLMaster - Win32 Release"

!ELSEIF  "$(CFG)" == "HLMaster - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\HLModMaster.h

!IF  "$(CFG)" == "HLMaster - Win32 Release"

!ELSEIF  "$(CFG)" == "HLMaster - Win32 Debug"

!ENDIF 

# End Source File
# End Target
################################################################################
# Begin Target

# Name "MasterTest - Win32 Release"
# Name "MasterTest - Win32 Debug"

!IF  "$(CFG)" == "MasterTest - Win32 Release"

!ELSEIF  "$(CFG)" == "MasterTest - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\MasterTest\MasterTest.cpp
DEP_CPP_MASTE=\
	".\MasterTest\MasterTest.h"\
	".\MasterTest\MasterTestDlg.h"\
	".\MasterTest\StdAfx.h"\
	

"$(INTDIR)\MasterTest.obj" : $(SOURCE) $(DEP_CPP_MASTE) "$(INTDIR)"\
 "$(INTDIR)\MasterTest.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\MasterTest\MasterTestDlg.cpp
DEP_CPP_MASTER=\
	".\MasterTest\..\HLMasterProtocol.h"\
	".\MasterTest\HLMasterAsyncSocket.h"\
	".\MasterTest\MasterTest.h"\
	".\MasterTest\MasterTestDlg.h"\
	".\MasterTest\MessageBuffer.h"\
	".\MasterTest\StdAfx.h"\
	

"$(INTDIR)\MasterTestDlg.obj" : $(SOURCE) $(DEP_CPP_MASTER) "$(INTDIR)"\
 "$(INTDIR)\MasterTest.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\MasterTest\StdAfx.cpp
DEP_CPP_STDAF=\
	".\MasterTest\StdAfx.h"\
	

!IF  "$(CFG)" == "MasterTest - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS"\
 /Fp"$(INTDIR)/MasterTest.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\MasterTest.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "MasterTest - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_MBCS" /Fp"$(INTDIR)/MasterTest.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\MasterTest.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MasterTest\MasterTest.rc
DEP_RSC_MASTERT=\
	".\MasterTest\res\MasterTest.ico"\
	".\MasterTest\res\MasterTest.rc2"\
	

!IF  "$(CFG)" == "MasterTest - Win32 Release"


"$(INTDIR)\MasterTest.res" : $(SOURCE) $(DEP_RSC_MASTERT) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/MasterTest.res" /i "MasterTest" /d "NDEBUG"\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "MasterTest - Win32 Debug"


"$(INTDIR)\MasterTest.res" : $(SOURCE) $(DEP_RSC_MASTERT) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/MasterTest.res" /i "MasterTest" /d "_DEBUG"\
 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MasterTest\MessageBuffer.cpp
DEP_CPP_MESSA=\
	".\MasterTest\MasterTest.h"\
	".\MasterTest\MessageBuffer.h"\
	".\MasterTest\StdAfx.h"\
	

"$(INTDIR)\MessageBuffer.obj" : $(SOURCE) $(DEP_CPP_MESSA) "$(INTDIR)"\
 "$(INTDIR)\MasterTest.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\MasterTest\HLMasterAsyncSocket.cpp
DEP_CPP_HLMAS=\
	".\MasterTest\..\HLMasterProtocol.h"\
	".\MasterTest\HLMasterAsyncSocket.h"\
	".\MasterTest\MasterTest.h"\
	".\MasterTest\MasterTestDlg.h"\
	".\MasterTest\MessageBuffer.h"\
	".\MasterTest\StdAfx.h"\
	

"$(INTDIR)\HLMasterAsyncSocket.obj" : $(SOURCE) $(DEP_CPP_HLMAS) "$(INTDIR)"\
 "$(INTDIR)\MasterTest.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\MasterTest\UTIL.cpp
DEP_CPP_UTIL_=\
	".\MasterTest\MasterTest.h"\
	".\MasterTest\StdAfx.h"\
	

"$(INTDIR)\UTIL.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"\
 "$(INTDIR)\MasterTest.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
