# Microsoft Developer Studio Project File - Name="particle" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=particle - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "particle.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "particle.mak" CFG="particle - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "particle - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "particle - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/utils/particle", PQQCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "particle - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /Oy- /I "." /I "../common" /I "../matsysapp" /I "..\..\common" /I "..\..\common\materialsystem" /I "..\..\engine" /I "..\..\engine\move" /I "..\..\cl_dll" /I "..\..\public" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "WIN_ERROR" /D "PARTICLEPROTOTYPE_APP" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 opengl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "particle___Win32_Debug"
# PROP BASE Intermediate_Dir "particle___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "particle___Win32_Debug"
# PROP Intermediate_Dir "particle___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /I "../common" /I "../matsysapp" /I "..\..\common" /I "..\..\common\materialsystem" /I "..\..\engine" /I "..\..\engine\move" /I "..\..\cl_dll" /I "..\..\public" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "WIN_ERROR" /D "PARTICLEPROTOTYPE_APP" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 opengl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "particle - Win32 Release"
# Name "particle - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\cl_dll\hl2_hud\c_ar2_explosion.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\c_baseentity_stub.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\c_baseparticleentity.cpp
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\c_movie_explosion.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\c_particle_smokegrenade.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\c_perftest.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\c_smoke_trail.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\c_SmokeStack.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\c_spheretest.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\c_steamjet.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Public\client_class.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\counter.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Public\datatable_recv.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Public\fasttimer.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Public\interface.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Public\Mathlib.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\matsysapp\matsysapp.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Public\MemPool.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\object_bucket.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\particle.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\particle_prototype.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\particle_proxies.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\particlemgr.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\particles_simple.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\ParticleSphereRenderer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\propfile.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od /Yc"glquake.h"

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

# ADD CPP /Yc"glquake.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tweakpropdlg.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Public\vallocator.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Public\vmatrix.cpp

!IF  "$(CFG)" == "particle - Win32 Release"

# ADD CPP /Od
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "particle - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\cl_dll\c_particlebaseeffect.h
# End Source File
# Begin Source File

SOURCE=.\counter.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\fasttimer.h
# End Source File
# Begin Source File

SOURCE=.\helpers.h
# End Source File
# Begin Source File

SOURCE=..\..\common\MaterialSystem\MaterialSystem_Config.h
# End Source File
# Begin Source File

SOURCE=..\matsysapp\matsysapp.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\particle_prototype.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\particle_util.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\particledraw.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\particlemgr.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\particles_simple.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\ParticleSphereRenderer.h
# End Source File
# Begin Source File

SOURCE=.\tweakpropdlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "External Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Public\amd3dx.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\arraylist.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\baselist.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\bitbuf.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\BSPFILE.H
# End Source File
# Begin Source File

SOURCE=..\..\Public\bspflags.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\c_baseentity.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\c_smoke_trail.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\cache.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\cache_user.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\cdll_int.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\cl_dll.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\CLIENT.H
# End Source File
# Begin Source File

SOURCE=..\..\Public\client_class.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\client_command.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\cliententitylist.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\CMD.H
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\cmodel.h
# End Source File
# Begin Source File

SOURCE=..\..\common\COMMON.H
# End Source File
# Begin Source File

SOURCE=..\..\Public\commonmacros.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\conprint.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\const.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\convar.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\custom.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\CVAR.H
# End Source File
# Begin Source File

SOURCE=..\..\engine\datatable.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\datatable_common.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\datatable_recv.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\dlight.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\entity.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\entity_state.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\event_flags.h
# End Source File
# Begin Source File

SOURCE=..\..\common\event_system.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\fastlinklist.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\GL_MODEL.H
# End Source File
# Begin Source File

SOURCE=..\..\engine\GLQUAKE.H
# End Source File
# Begin Source File

SOURCE=..\..\common\hlshell.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\icliententitylist.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\iclientsound.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\icvar.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\iefx.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\ifileaccess.h
# End Source File
# Begin Source File

SOURCE=..\..\common\MaterialSystem\IMaterial.h
# End Source File
# Begin Source File

SOURCE=..\..\common\MaterialSystem\IMaterialProxyFactory.h
# End Source File
# Begin Source File

SOURCE=..\..\common\MaterialSystem\IMaterialSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\common\MaterialSystem\IMaterialVar.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\imovehelper.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\info.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\iprediction.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\ishadersystem.h
# End Source File
# Begin Source File

SOURCE=..\..\common\MaterialSystem\ITexture.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\itriangle.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\ivrenderview.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\linklist.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\MATHLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\Public\mouthinfo.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\NET.H
# End Source File
# Begin Source File

SOURCE=..\..\Public\netadr.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\object_bucket.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\packed_entity.h
# End Source File
# Begin Source File

SOURCE=..\..\common\packedentityarray.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\move\physent_client.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\plane.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\PROGS.H
# End Source File
# Begin Source File

SOURCE=..\..\Public\propfile.h
# End Source File
# Begin Source File

SOURCE=..\..\common\QUAKEDEF.H
# End Source File
# Begin Source File

SOURCE=..\..\engine\RENDER.H
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\smoke_fog_overlay.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\..\engine\sysexternal.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\touchlist.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\usercmd.h
# End Source File
# Begin Source File

SOURCE=..\..\cl_dll\util_vector.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vallocator.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\view_shared.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vmatrix.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vplane.h
# End Source File
# Begin Source File

SOURCE=..\..\common\vstring.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\worldsize.h
# End Source File
# End Group
# End Target
# End Project
