# Microsoft Developer Studio Project File - Name="Worldcraft" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Worldcraft - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "worldcraft.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "worldcraft.mak" CFG="Worldcraft - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Worldcraft - Win32 SDK Release" (based on "Win32 (x86) Application")
!MESSAGE "Worldcraft - Win32 SDK Debug" (based on "Win32 (x86) Application")
!MESSAGE "Worldcraft - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Worldcraft - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/worldcraft", SDSBAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Worldcraft - Win32 SDK Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Worldcraft___Win32_SDK_Release"
# PROP BASE Intermediate_Dir "Worldcraft___Win32_SDK_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "SDK Release"
# PROP Intermediate_Dir "SDK Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GR /GX /O2 /Ob2 /I "..\common" /I "..\engine" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "VALVE" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GR /GX /O2 /Ob2 /I "..\public" /I "..\common" /I "..\engine" /I "..\common\MaterialSystem" /I "..\common\ShaderDLL" /I "..\common\iostream" /D "NDEBUG" /D "SDK_BUILD" /D "_WIN32" /D "_WINDOWS" /D "_MBCS" /D "VALVE" /D "IMAGE_LOADER_NO_DXTC" /D "PROTECTED_THINGS_DISABLE" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "VALVE"
# ADD RSC /l 0x409 /d "NDEBUG" /d "VALVE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib opengl32.lib glu32.lib rpcrt4.lib /nologo /subsystem:windows /profile /machine:I386 /out:".\Valve Release\wc.exe"
# ADD LINK32 winmm.lib opengl32.lib glu32.lib rpcrt4.lib /nologo /subsystem:windows /machine:I386 /out:".\SDK Release\wc.exe" /libpath:"..\lib\common" /libpath:"..\lib\public"
# SUBTRACT LINK32 /profile

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 SDK Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Worldcraft___Win32_SDK_Debug"
# PROP BASE Intermediate_Dir "Worldcraft___Win32_SDK_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "SDK Debug"
# PROP Intermediate_Dir "SDK Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\common" /I "..\engine" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "VALVE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /GR /GX /ZI /Od /I "..\public" /I "..\common" /I "..\engine" /I "..\common\MaterialSystem" /I "..\common\ShaderDLL" /I "..\common\iostream" /D "_DEBUG" /D "_AFXDLL" /D "SDK_BUILD" /D "_WIN32" /D "_WINDOWS" /D "_MBCS" /D "VALVE" /D "IMAGE_LOADER_NO_DXTC" /D "PROTECTED_THINGS_DISABLE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL" /d "VALVE"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL" /d "VALVE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib opengl32.lib glu32.lib rpcrt4.lib /nologo /subsystem:windows /profile /debug /machine:I386 /out:".\Valve Debug\wc.exe"
# ADD LINK32 winmm.lib opengl32.lib glu32.lib rpcrt4.lib /nologo /subsystem:windows /profile /debug /machine:I386 /out:".\SDK Debug\wc.exe" /libpath:"..\lib\common" /libpath:"..\lib\public"

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Worldcraft___Win32_Release"
# PROP BASE Intermediate_Dir "Worldcraft___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Worldcraft___Win32_Release"
# PROP Intermediate_Dir "Worldcraft___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MT /W3 /GR /GX /O2 /Ob2 /I "..\public" /I "..\common" /I "..\engine" /I "..\common\MaterialSystem" /I "..\game_shared" /D "NDEBUG" /D "USE_MATERIAL_SYSTEM" /D "_WIN32" /D "_WINDOWS" /D "_MBCS" /D "VALVE" /D "IMAGE_LOADER_NO_DXTC" /D "PROTECTED_THINGS_DISABLE" /Yu"stdafx.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /G6 /MT /W3 /GR /GX /O2 /Ob2 /I "..\public" /I "..\common" /I "..\engine" /I "..\common\MaterialSystem" /I "..\game_shared" /D "NDEBUG" /D "USE_MATERIAL_SYSTEM" /D "_WIN32" /D "_WINDOWS" /D "_MBCS" /D "VALVE" /D "IMAGE_LOADER_NO_DXTC" /D "PROTECTED_THINGS_DISABLE" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "VALVE"
# ADD RSC /l 0x409 /d "NDEBUG" /d "VALVE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib opengl32.lib glu32.lib rpcrt4.lib /nologo /subsystem:windows /machine:I386 /out:".\Valve Release\wc.exe" /libpath:"..\lib\common" /libpath:"..\lib\public"
# SUBTRACT BASE LINK32 /profile /debug
# ADD LINK32 winmm.lib opengl32.lib glu32.lib rpcrt4.lib /nologo /subsystem:windows /machine:I386 /out:".\Valve Release\wc.exe" /libpath:"..\lib\common" /libpath:"..\lib\public"
# SUBTRACT LINK32 /profile /debug
# Begin Custom Build - Copying executable...
TargetPath=.\Valve Release\wc.exe
InputPath=.\Valve Release\wc.exe
SOURCE="$(InputPath)"

"..\..\bin\wc.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\bin\wc.exe attrib -r ..\..\bin\wc.exe 
	if exist "$(TargetPath)" copy "$(TargetPath)" ..\..\bin 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Worldcraft___Win32_Debug"
# PROP BASE Intermediate_Dir "Worldcraft___Win32_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Worldcraft___Win32_Debug"
# PROP Intermediate_Dir "Worldcraft___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MTd /W3 /Gm /Gi /GR /GX /ZI /Od /I "..\common\shaderdll" /I "..\common\ShaderDLL" /I "..\common\iostream" /I "..\public" /I "..\common" /I "..\engine" /I "..\common\MaterialSystem" /I "..\game_shared" /D "_DEBUG" /D "USE_MATERIAL_SYSTEM" /D "_WIN32" /D "_WINDOWS" /D "_MBCS" /D "VALVE" /D "IMAGE_LOADER_NO_DXTC" /D "PROTECTED_THINGS_DISABLE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /Gi /GR /GX /ZI /Od /I "..\common\shaderdll" /I "..\common\ShaderDLL" /I "..\common\iostream" /I "..\public" /I "..\common" /I "..\engine" /I "..\common\MaterialSystem" /I "..\game_shared" /D "_DEBUG" /D "USE_MATERIAL_SYSTEM" /D "_WIN32" /D "_WINDOWS" /D "_MBCS" /D "VALVE" /D "IMAGE_LOADER_NO_DXTC" /D "PROTECTED_THINGS_DISABLE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "VALVE"
# ADD RSC /l 0x409 /d "_DEBUG" /d "VALVE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib rpcrt4.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\Valve Debug\wc.exe" /libpath:"..\lib\common" /libpath:"..\lib\public"
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 winmm.lib rpcrt4.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\Valve Debug\wc.exe" /libpath:"..\lib\common" /libpath:"..\lib\public"
# SUBTRACT LINK32 /profile
# Begin Custom Build
TargetPath=.\Valve Debug\wc.exe
InputPath=.\Valve Debug\wc.exe
SOURCE="$(InputPath)"

"..\..\bin\wc.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist ..\..\bin\wc.exe attrib -r ..\..\bin\wc.exe 
	if exist "$(TargetPath)" copy "$(TargetPath)" ..\..\bin 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "Worldcraft - Win32 SDK Release"
# Name "Worldcraft - Win32 SDK Debug"
# Name "Worldcraft - Win32 Release"
# Name "Worldcraft - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Group "Map classes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MapAlignedBox.cpp
# End Source File
# Begin Source File

SOURCE=.\MapAlignedBox.h
# End Source File
# Begin Source File

SOURCE=.\MapAnimator.cpp
# End Source File
# Begin Source File

SOURCE=.\MapAnimator.h
# End Source File
# Begin Source File

SOURCE=.\MapAtom.h
# End Source File
# Begin Source File

SOURCE=.\MapAxisHandle.cpp
# End Source File
# Begin Source File

SOURCE=.\MapAxisHandle.h
# End Source File
# Begin Source File

SOURCE=.\Mapclass.cpp
# End Source File
# Begin Source File

SOURCE=.\mapclass.h
# End Source File
# Begin Source File

SOURCE=.\MapDecal.cpp
# End Source File
# Begin Source File

SOURCE=.\MapDecal.h
# End Source File
# Begin Source File

SOURCE=.\MapDisp.cpp
# End Source File
# Begin Source File

SOURCE=.\MapDisp.h
# End Source File
# Begin Source File

SOURCE=.\mapdoc.h
# End Source File
# Begin Source File

SOURCE=.\MapEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\MapEntity.h
# End Source File
# Begin Source File

SOURCE=.\MapFace.cpp
# End Source File
# Begin Source File

SOURCE=.\MapFace.h
# End Source File
# Begin Source File

SOURCE=.\MapGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\MapGroup.h
# End Source File
# Begin Source File

SOURCE=.\MapHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\MapKeyFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\MapKeyFrame.h
# End Source File
# Begin Source File

SOURCE=.\MapLight.cpp
# End Source File
# Begin Source File

SOURCE=.\MapLight.h
# End Source File
# Begin Source File

SOURCE=.\MapLightCone.cpp
# End Source File
# Begin Source File

SOURCE=.\MapLightCone.h
# End Source File
# Begin Source File

SOURCE=.\MapLine.cpp
# End Source File
# Begin Source File

SOURCE=.\MapLine.h
# End Source File
# Begin Source File

SOURCE=.\MapOverlay.cpp
# End Source File
# Begin Source File

SOURCE=.\MapOverlay.h
# End Source File
# Begin Source File

SOURCE=.\MapPoint.cpp
# End Source File
# Begin Source File

SOURCE=.\MapPoint.h
# End Source File
# Begin Source File

SOURCE=.\MapPointHandle.cpp
# End Source File
# Begin Source File

SOURCE=.\MapPointHandle.h
# End Source File
# Begin Source File

SOURCE=.\MapQuadBounds.cpp
# End Source File
# Begin Source File

SOURCE=.\MapQuadBounds.h
# End Source File
# Begin Source File

SOURCE=.\MapSideList.cpp
# End Source File
# Begin Source File

SOURCE=.\MapSideList.h
# End Source File
# Begin Source File

SOURCE=.\MapSolid.cpp
# End Source File
# Begin Source File

SOURCE=.\MapSolid.h
# End Source File
# Begin Source File

SOURCE=.\MapSphere.cpp
# End Source File
# Begin Source File

SOURCE=.\MapSphere.h
# End Source File
# Begin Source File

SOURCE=.\MapSprite.cpp
# End Source File
# Begin Source File

SOURCE=.\MapStudioModel.cpp
# End Source File
# Begin Source File

SOURCE=.\MapStudioModel.h
# End Source File
# Begin Source File

SOURCE=.\MapWorld.cpp
# End Source File
# Begin Source File

SOURCE=.\MapWorld.h
# End Source File
# End Group
# Begin Group "Tools"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Box3D.cpp
# End Source File
# Begin Source File

SOURCE=.\Box3D.h
# End Source File
# Begin Source File

SOURCE=.\Tool3D.cpp
# End Source File
# Begin Source File

SOURCE=.\Tool3D.h
# End Source File
# Begin Source File

SOURCE=.\ToolAxisHandle.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolAxisHandle.h
# End Source File
# Begin Source File

SOURCE=.\ToolBlock.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolBlock.h
# End Source File
# Begin Source File

SOURCE=.\ToolCamera.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolCamera.h
# End Source File
# Begin Source File

SOURCE=.\ToolClipper.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolClipper.h
# End Source File
# Begin Source File

SOURCE=.\ToolCordon.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolCordon.h
# End Source File
# Begin Source File

SOURCE=.\ToolDecal.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolDecal.h
# End Source File
# Begin Source File

SOURCE=.\ToolDisplace.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolDisplace.h
# End Source File
# Begin Source File

SOURCE=.\ToolEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolEntity.h
# End Source File
# Begin Source File

SOURCE=.\ToolInterface.h
# End Source File
# Begin Source File

SOURCE=.\ToolMagnify.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolMagnify.h
# End Source File
# Begin Source File

SOURCE=.\ToolManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolManager.h
# End Source File
# Begin Source File

SOURCE=.\ToolMaterial.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolMaterial.h
# End Source File
# Begin Source File

SOURCE=.\ToolMorph.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolMorph.h
# End Source File
# Begin Source File

SOURCE=.\ToolOverlay.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolOverlay.h
# End Source File
# Begin Source File

SOURCE=.\ToolPath.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolPath.h
# End Source File
# Begin Source File

SOURCE=.\ToolPickAngles.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolPickAngles.h
# End Source File
# Begin Source File

SOURCE=.\ToolPickEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolPickEntity.h
# End Source File
# Begin Source File

SOURCE=.\ToolPickFace.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolPickFace.h
# End Source File
# Begin Source File

SOURCE=.\ToolPointHandle.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolPointHandle.h
# End Source File
# Begin Source File

SOURCE=.\ToolSelection.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolSelection.h
# End Source File
# Begin Source File

SOURCE=.\ToolSphere.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolSphere.h
# End Source File
# End Group
# Begin Group "Texture/materials system"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DummyTexture.cpp
# End Source File
# Begin Source File

SOURCE=.\DummyTexture.h
# End Source File
# Begin Source File

SOURCE=.\IEditorTexture.h
# End Source File
# Begin Source File

SOURCE=..\Public\ImageLoader.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Material.cpp
# End Source File
# Begin Source File

SOURCE=.\Material.h
# End Source File
# Begin Source File

SOURCE=.\Texture.cpp
# End Source File
# Begin Source File

SOURCE=.\Texture.h
# End Source File
# Begin Source File

SOURCE=.\TextureSystem.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\TGALoader.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\WADTexture.cpp
# End Source File
# Begin Source File

SOURCE=.\WADTexture.h
# End Source File
# End Group
# Begin Group "Shell"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Shell.cpp
# End Source File
# Begin Source File

SOURCE=.\ShellMessageWnd.cpp
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ArchDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ArchDlg.h
# End Source File
# Begin Source File

SOURCE=.\DispDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DispDlg.h
# End Source File
# Begin Source File

SOURCE=.\EditGroups.cpp
# End Source File
# Begin Source File

SOURCE=.\EditPathDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EditPathDlg.h
# End Source File
# Begin Source File

SOURCE=.\EditPathNodeDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EditPathNodeDlg.h
# End Source File
# Begin Source File

SOURCE=.\EditPrefabDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EditPrefabDlg.h
# End Source File
# Begin Source File

SOURCE=.\EntityHelpDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EntityHelpDlg.h
# End Source File
# Begin Source File

SOURCE=.\EntityReportDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EntityReportDlg.h
# End Source File
# Begin Source File

SOURCE=.\FaceEdit_DispPage.cpp
# End Source File
# Begin Source File

SOURCE=.\FaceEdit_MaterialPage.cpp
# End Source File
# Begin Source File

SOURCE=.\FaceEditSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\FilterControl.cpp
# End Source File
# Begin Source File

SOURCE=.\GotoBrushDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\GotoBrushDlg.h
# End Source File
# Begin Source File

SOURCE=.\MapAnimationDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\MapAnimationDlg.h
# End Source File
# Begin Source File

SOURCE=.\MapCheckDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\MapCheckDlg.h
# End Source File
# Begin Source File

SOURCE=.\MapErrorsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\MapInfoDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\materialdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\materialdlg.h
# End Source File
# Begin Source File

SOURCE=.\NewDocType.cpp
# End Source File
# Begin Source File

SOURCE=.\NewKeyValue.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\OP_Entity.cpp
# End Source File
# Begin Source File

SOURCE=.\OP_Flags.cpp
# End Source File
# Begin Source File

SOURCE=.\OP_Groups.cpp
# End Source File
# Begin Source File

SOURCE=.\OP_Input.cpp
# End Source File
# Begin Source File

SOURCE=.\OP_Model.cpp
# End Source File
# Begin Source File

SOURCE=.\OP_Output.cpp
# End Source File
# Begin Source File

SOURCE=.\OPTBuild.cpp
# End Source File
# Begin Source File

SOURCE=.\OPTConfigs.cpp
# End Source File
# Begin Source File

SOURCE=.\OPTGeneral.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\OPTTextures.cpp
# End Source File
# Begin Source File

SOURCE=.\OPTView2D.cpp
# End Source File
# Begin Source File

SOURCE=.\OPTView3D.cpp
# End Source File
# Begin Source File

SOURCE=.\PasteSpecialDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PasteSpecialDlg.h
# End Source File
# Begin Source File

SOURCE=.\PrefabsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PrefabsDlg.h
# End Source File
# Begin Source File

SOURCE=.\ProgDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ReplaceTexDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ReplaceTexDlg.h
# End Source File
# Begin Source File

SOURCE=.\RunMapCfgDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\RunMapCfgDlg.h
# End Source File
# Begin Source File

SOURCE=.\RunMapExpertDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\RunMapExpertDlg.h
# End Source File
# Begin Source File

SOURCE=.\ScaleVerticesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ScaleVerticesDlg.h
# End Source File
# Begin Source File

SOURCE=.\SearchReplaceDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SearchReplaceDlg.h
# End Source File
# Begin Source File

SOURCE=.\SelectEntityDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SelectEntityDlg.h
# End Source File
# Begin Source File

SOURCE=.\SelectModeDlgBar.cpp
# End Source File
# Begin Source File

SOURCE=.\StartupDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StartupDlg.h
# End Source File
# Begin Source File

SOURCE=.\StrDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StrDlg.h
# End Source File
# Begin Source File

SOURCE=.\TextureBar.cpp
# End Source File
# Begin Source File

SOURCE=.\texturebrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\TransformDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\TransformDlg.h
# End Source File
# Begin Source File

SOURCE=.\UndoWarningDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\WorldcraftBar.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ActiveItemList.h
# End Source File
# Begin Source File

SOURCE=.\AngleBox.cpp
# End Source File
# Begin Source File

SOURCE=.\AngleBox.h
# End Source File
# Begin Source File

SOURCE=.\AutoSelCombo.cpp
# End Source File
# Begin Source File

SOURCE=.\AutoSelCombo.h
# End Source File
# Begin Source File

SOURCE=.\Axes2.cpp
# End Source File
# Begin Source File

SOURCE=.\Axes2.h
# End Source File
# Begin Source File

SOURCE=.\BlockArray.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\bone_setup.cpp

!IF  "$(CFG)" == "Worldcraft - Win32 SDK Release"

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 SDK Debug"

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 Release"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\BoundBox.cpp
# End Source File
# Begin Source File

SOURCE=.\BoundBox.h
# End Source File
# Begin Source File

SOURCE=.\Brushops.cpp
# End Source File
# Begin Source File

SOURCE=.\BrushOps.h
# End Source File
# Begin Source File

SOURCE=.\bsplighting.cpp
# End Source File
# Begin Source File

SOURCE=.\bsplighting.h
# End Source File
# Begin Source File

SOURCE=.\bsplightingthread.cpp
# End Source File
# Begin Source File

SOURCE=.\bsplightingthread.h
# End Source File
# Begin Source File

SOURCE=..\common\builddisp.cpp

!IF  "$(CFG)" == "Worldcraft - Win32 SDK Release"

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 SDK Debug"

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 Release"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\builddisp.h
# End Source File
# Begin Source File

SOURCE=.\BuildNum.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Camera.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Camera.h
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=..\Public\ChunkFile.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Public\ChunkFile.h
# End Source File
# Begin Source File

SOURCE=.\clipcode.cpp
# End Source File
# Begin Source File

SOURCE=.\clipcode.h
# End Source File
# Begin Source File

SOURCE=.\Clock.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\CollisionUtils.cpp

!IF  "$(CFG)" == "Worldcraft - Win32 SDK Release"

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 SDK Debug"

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 Release"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Worldcraft - Win32 Debug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Public\CollisionUtils.h
# End Source File
# Begin Source File

SOURCE=.\ControlBarIDs.h
# End Source File
# Begin Source File

SOURCE=.\CreateArch.cpp
# End Source File
# Begin Source File

SOURCE=.\CullTreeNode.cpp
# End Source File
# Begin Source File

SOURCE=.\CullTreeNode.h
# End Source File
# Begin Source File

SOURCE=.\CustomMessages.h
# End Source File
# Begin Source File

SOURCE=..\common\disp_common.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\disp_common.h
# End Source File
# Begin Source File

SOURCE=..\common\disp_powerinfo.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\DispManager.cpp
# End Source File
# Begin Source File

SOURCE=.\DispManager.h
# End Source File
# Begin Source File

SOURCE=.\DispMapImageFilter.cpp
# End Source File
# Begin Source File

SOURCE=.\DispMapImageFilter.h
# End Source File
# Begin Source File

SOURCE=.\DispPaint.cpp
# End Source File
# Begin Source File

SOURCE=.\DispPaint.h
# End Source File
# Begin Source File

SOURCE=.\DispSew.cpp
# End Source File
# Begin Source File

SOURCE=.\DispSew.h
# End Source File
# Begin Source File

SOURCE=.\DispSubdiv.cpp
# End Source File
# Begin Source File

SOURCE=.\DispSubdiv.h
# End Source File
# Begin Source File

SOURCE=.\DynamicDialogWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\DynamicDialogWnd.h
# End Source File
# Begin Source File

SOURCE=.\EditGameClass.cpp
# End Source File
# Begin Source File

SOURCE=.\EditGameClass.h
# End Source File
# Begin Source File

SOURCE=.\EditGameConfigs.cpp
# End Source File
# Begin Source File

SOURCE=.\EditGameConfigs.h
# End Source File
# Begin Source File

SOURCE=.\EditGroups.h
# End Source File
# Begin Source File

SOURCE=.\EntityConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\EntityConnection.h
# End Source File
# Begin Source File

SOURCE=.\Error3d.h
# End Source File
# Begin Source File

SOURCE=.\FaceEdit_DispPage.h
# End Source File
# Begin Source File

SOURCE=.\FaceEdit_MaterialPage.h
# End Source File
# Begin Source File

SOURCE=.\FaceEditSheet.h
# End Source File
# Begin Source File

SOURCE=.\FileSystem_WC.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\FileSystem_WC.h
# End Source File
# Begin Source File

SOURCE=.\FilterControl.h
# End Source File
# Begin Source File

SOURCE=.\GameData.cpp
# End Source File
# Begin Source File

SOURCE=.\GameData.h
# End Source File
# Begin Source File

SOURCE=.\GamePalette.cpp
# End Source File
# Begin Source File

SOURCE=.\GamePalette.h
# End Source File
# Begin Source File

SOURCE=.\GDClass.cpp
# End Source File
# Begin Source File

SOURCE=.\GDVar.cpp
# End Source File
# Begin Source File

SOURCE=.\GDVar.h
# End Source File
# Begin Source File

SOURCE=.\Gizmo.cpp
# ADD CPP /Yu
# End Source File
# Begin Source File

SOURCE=.\Gizmo.h
# End Source File
# Begin Source File

SOURCE=.\GlobalFunctions.h
# End Source File
# Begin Source File

SOURCE=.\GroupList.cpp
# End Source File
# Begin Source File

SOURCE=.\GroupList.h
# End Source File
# Begin Source File

SOURCE=.\HelperFactory.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Helpid.h
# End Source File
# Begin Source File

SOURCE=.\History.cpp
# End Source File
# Begin Source File

SOURCE=.\History.h
# End Source File
# Begin Source File

SOURCE=.\ibsplighting.h
# End Source File
# Begin Source File

SOURCE=.\IconComboBox.cpp
# End Source File
# Begin Source File

SOURCE=.\IconComboBox.h
# End Source File
# Begin Source File

SOURCE=.\InputOutput.cpp
# End Source File
# Begin Source File

SOURCE=..\Public\interface.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\ivraddll.h
# End Source File
# Begin Source File

SOURCE=.\Keyboard.cpp
# End Source File
# Begin Source File

SOURCE=.\Keyboard.h
# End Source File
# Begin Source File

SOURCE=..\common\KeyFrame\keyframe.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\KeyFrame\keyframe.h
# End Source File
# Begin Source File

SOURCE=.\KeyValues.cpp
# End Source File
# Begin Source File

SOURCE=.\KeyValues.h
# End Source File
# Begin Source File

SOURCE=.\ListBoxEx.cpp
# End Source File
# Begin Source File

SOURCE=.\ListBoxEx.h
# End Source File
# Begin Source File

SOURCE=.\LoadSave_MAP.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadSave_RMF.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\MakePrefabLibrary.cpp
# End Source File
# Begin Source File

SOURCE=.\MapDefs.h
# End Source File
# Begin Source File

SOURCE=.\Mapdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\MapErrorsDlg.h
# End Source File
# Begin Source File

SOURCE=.\MapInfoDlg.h
# End Source File
# Begin Source File

SOURCE=.\MapPath.cpp
# End Source File
# Begin Source File

SOURCE=.\MapPath.h
# End Source File
# Begin Source File

SOURCE=.\MapView.cpp
# End Source File
# Begin Source File

SOURCE=.\MapView.h
# End Source File
# Begin Source File

SOURCE=.\MapView2D.cpp
# End Source File
# Begin Source File

SOURCE=.\MapView2D.h
# End Source File
# Begin Source File

SOURCE=.\MapView3D.cpp
# End Source File
# Begin Source File

SOURCE=.\mapview3d.h
# End Source File
# Begin Source File

SOURCE=.\materialproxyfactory_wc.cpp
# End Source File
# Begin Source File

SOURCE=.\materialproxyfactory_wc.h
# End Source File
# Begin Source File

SOURCE=..\Public\Mathlib.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\MDIClientWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\MDIClientWnd.h
# End Source File
# Begin Source File

SOURCE=.\MessageWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\MessageWnd.h
# End Source File
# Begin Source File

SOURCE=.\misc.cpp
# End Source File
# Begin Source File

SOURCE=.\ModelFactory.h
# End Source File
# Begin Source File

SOURCE=.\MyCheckListBox.h
# End Source File
# Begin Source File

SOURCE=.\NewDocType.h
# End Source File
# Begin Source File

SOURCE=.\NewKeyValue.h
# End Source File
# Begin Source File

SOURCE=.\Noise.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectBar.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectBar.h
# End Source File
# Begin Source File

SOURCE=.\ObjectPage.h
# End Source File
# Begin Source File

SOURCE=.\ObjectProperties.h
# End Source File
# Begin Source File

SOURCE=.\OP_ComplexObject.h
# End Source File
# Begin Source File

SOURCE=.\OP_Entity.h
# End Source File
# Begin Source File

SOURCE=.\OP_Flags.h
# End Source File
# Begin Source File

SOURCE=.\OP_Groups.h
# End Source File
# Begin Source File

SOURCE=.\OP_Input.h
# End Source File
# Begin Source File

SOURCE=.\OP_Model.h
# End Source File
# Begin Source File

SOURCE=.\OP_Output.h
# End Source File
# Begin Source File

SOURCE=.\optbuild.h
# End Source File
# Begin Source File

SOURCE=.\OPTConfigs.h
# End Source File
# Begin Source File

SOURCE=.\OPTGeneral.h
# End Source File
# Begin Source File

SOURCE=.\OptionProperties.h
# End Source File
# Begin Source File

SOURCE=.\Options.cpp
# End Source File
# Begin Source File

SOURCE=.\Options.h
# End Source File
# Begin Source File

SOURCE=.\OPTTextures.h
# End Source File
# Begin Source File

SOURCE=.\OPTView2D.h
# End Source File
# Begin Source File

SOURCE=.\OPTView3D.h
# End Source File
# Begin Source File

SOURCE=.\osver.cpp
# End Source File
# Begin Source File

SOURCE=.\PakDoc.h
# End Source File
# Begin Source File

SOURCE=.\PakFrame.h
# End Source File
# Begin Source File

SOURCE=.\PakViewDirec.h
# End Source File
# Begin Source File

SOURCE=.\PakViewFiles.h
# End Source File
# Begin Source File

SOURCE=.\Prefab3D.cpp
# End Source File
# Begin Source File

SOURCE=.\Prefab3d.h
# End Source File
# Begin Source File

SOURCE=.\Prefabs.cpp
# End Source File
# Begin Source File

SOURCE=.\Prefabs.h
# End Source File
# Begin Source File

SOURCE=.\ProcessWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcessWnd.h
# End Source File
# Begin Source File

SOURCE=.\progdlg.h
# End Source File
# Begin Source File

SOURCE=.\Render2D.cpp
# End Source File
# Begin Source File

SOURCE=.\Render2D.h
# End Source File
# Begin Source File

SOURCE=.\Render3D.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Render3D.h
# End Source File
# Begin Source File

SOURCE=.\Render3DMS.cpp
# End Source File
# Begin Source File

SOURCE=.\Render3DMS.h
# End Source File
# Begin Source File

SOURCE=.\RenderUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\RenderUtils.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\RichEditCtrlEx.cpp
# End Source File
# Begin Source File

SOURCE=.\RichEditCtrlEx.h
# End Source File
# Begin Source File

SOURCE=..\Public\rope_physics.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\RunCommands.cpp
# End Source File
# Begin Source File

SOURCE=.\RunCommands.h
# End Source File
# Begin Source File

SOURCE=.\RunMap.cpp
# End Source File
# Begin Source File

SOURCE=.\RunMap.h
# End Source File
# Begin Source File

SOURCE=.\SaveInfo.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\SaveInfo.h
# End Source File
# Begin Source File

SOURCE=.\SearchBox.h
# End Source File
# Begin Source File

SOURCE=.\Shell.h
# End Source File
# Begin Source File

SOURCE=..\Public\simple_physics.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\Splash.h
# End Source File
# Begin Source File

SOURCE=.\Sprite.cpp
# End Source File
# Begin Source File

SOURCE=.\SSolid.cpp
# End Source File
# Begin Source File

SOURCE=.\SSolid.h
# End Source File
# Begin Source File

SOURCE=.\StatusBarIDs.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\StockSolids.cpp
# End Source File
# Begin Source File

SOURCE=.\StockSolids.h
# End Source File
# Begin Source File

SOURCE=.\StudioModel.cpp
# End Source File
# Begin Source File

SOURCE=.\StudioModel.h
# End Source File
# Begin Source File

SOURCE=.\TargetNameCombo.cpp
# End Source File
# Begin Source File

SOURCE=.\TargetNameCombo.h
# End Source File
# Begin Source File

SOURCE=.\TextureBar.h
# End Source File
# Begin Source File

SOURCE=.\TextureBox.cpp
# End Source File
# Begin Source File

SOURCE=.\TextureBox.h
# End Source File
# Begin Source File

SOURCE=.\texturebrowser.h
# End Source File
# Begin Source File

SOURCE=.\TextureConverter.cpp
# End Source File
# Begin Source File

SOURCE=.\TextureConverter.h
# End Source File
# Begin Source File

SOURCE=.\TextureWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\TextureWindow.h
# End Source File
# Begin Source File

SOURCE=.\TitleWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\TitleWnd.h
# End Source File
# Begin Source File

SOURCE=..\Public\TokenReader.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Tooldefs.h
# End Source File
# Begin Source File

SOURCE=.\Undo.h
# End Source File
# Begin Source File

SOURCE=.\UndoWarningDlg.h
# End Source File
# Begin Source File

SOURCE=..\Public\UtlBuffer.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ViewerSettings.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ViewerSettings.h
# End Source File
# Begin Source File

SOURCE=.\VisGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\VisGroup.h
# End Source File
# Begin Source File

SOURCE=..\Public\vmatrix.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\wndTex.h
# End Source File
# Begin Source File

SOURCE=.\Worldcraft.cpp
# End Source File
# Begin Source File

SOURCE=.\Worldcraft.h
# End Source File
# Begin Source File

SOURCE=.\Worldcraft.rc
# End Source File
# Begin Source File

SOURCE=.\worldcraft_mathlib.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\worldcraft_mathlib.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\afx_idi_.ico
# End Source File
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\res\connect1.ico
# End Source File
# Begin Source File

SOURCE=.\res\connect_grey.ico
# End Source File
# Begin Source File

SOURCE=.\res\connectbad_grey.ico
# End Source File
# Begin Source File

SOURCE=.\res\cube.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cubechec.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cur00001.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00002.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursor2.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursor3.cur
# End Source File
# Begin Source File

SOURCE=.\res\dispedit.bmp
# End Source File
# Begin Source File

SOURCE=.\res\entity.cur
# End Source File
# Begin Source File

SOURCE=.\res\eyedropper.cur
# End Source File
# Begin Source File

SOURCE=.\res\eyedropper.ico
# End Source File
# Begin Source File

SOURCE=.\facepaint.cur
# End Source File
# Begin Source File

SOURCE=.\res\Forge.ico
# End Source File
# Begin Source File

SOURCE=.\res\ForgeDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\forgemap.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gizmoaxis.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gizmorotate.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gizmoscale.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gizmotranslate.bmp
# End Source File
# Begin Source File

SOURCE=.\res\handclos.cur
# End Source File
# Begin Source File

SOURCE=.\res\handcurs.cur
# End Source File
# Begin Source File

SOURCE=.\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00003.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\input.ico
# End Source File
# Begin Source File

SOURCE=.\res\input1.ico
# End Source File
# Begin Source File

SOURCE=.\res\input_grey.ico
# End Source File
# Begin Source File

SOURCE=.\res\inputbad.ico
# End Source File
# Begin Source File

SOURCE=.\res\inputbad_grey.ico
# End Source File
# Begin Source File

SOURCE=.\res\magnify.cur
# End Source File
# Begin Source File

SOURCE=.\res\mapdoc_n.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mapedit.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mapedit256.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mapeditt.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mapopera.bmp
# End Source File
# Begin Source File

SOURCE=.\res\newsplash.BMP
# End Source File
# Begin Source File

SOURCE=.\res\newtoolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\otputbad.ico
# End Source File
# Begin Source File

SOURCE=.\res\ouput.ico
# End Source File
# Begin Source File

SOURCE=.\res\output_grey.ico
# End Source File
# Begin Source File

SOURCE=.\res\outputbad.ico
# End Source File
# Begin Source File

SOURCE=.\res\outputbad_grey.ico
# End Source File
# Begin Source File

SOURCE=.\res\paktypes.bmp
# End Source File
# Begin Source File

SOURCE=.\res\seltools.bmp
# End Source File
# Begin Source File

SOURCE=.\res\set_u.bmp
# End Source File
# Begin Source File

SOURCE=.\res\smartclick.cur
# End Source File
# Begin Source File

SOURCE=.\res\splash7b.BMP
# End Source File
# Begin Source File

SOURCE=.\res\texturew.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolbar1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\treeroot.bmp
# End Source File
# Begin Source File

SOURCE=.\res\visgroup.bmp
# End Source File
# Begin Source File

SOURCE=.\res\wcsplash.bmp
# End Source File
# End Group
# Begin Group "Common Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Public\amd3dx.h
# End Source File
# Begin Source File

SOURCE=..\Public\arraystack.h
# End Source File
# Begin Source File

SOURCE=..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\game_shared\bone_setup.h
# End Source File
# Begin Source File

SOURCE=..\Public\BSPFILE.H
# End Source File
# Begin Source File

SOURCE=..\Public\bspflags.h
# End Source File
# Begin Source File

SOURCE=.\BuildNum.h
# End Source File
# Begin Source File

SOURCE=..\Public\bumpvects.h
# End Source File
# Begin Source File

SOURCE=..\engine\cache.h
# End Source File
# Begin Source File

SOURCE=..\Public\cache_user.h
# End Source File
# Begin Source File

SOURCE=..\Public\cmodel.h
# End Source File
# Begin Source File

SOURCE=..\Public\const.h
# End Source File
# Begin Source File

SOURCE=.\EntityDefs.h
# End Source File
# Begin Source File

SOURCE=..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=.\GDClass.h
# End Source File
# Begin Source File

SOURCE=.\HelperFactory.h
# End Source File
# Begin Source File

SOURCE=.\HelperInfo.h
# End Source File
# Begin Source File

SOURCE=..\Public\ImageLoader.h
# End Source File
# Begin Source File

SOURCE=.\InputOutput.h
# End Source File
# Begin Source File

SOURCE=..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=.\MapSprite.h
# End Source File
# Begin Source File

SOURCE=..\Public\MATHLIB.H
# End Source File
# Begin Source File

SOURCE=..\Public\NTree.h
# End Source File
# Begin Source File

SOURCE=.\osver.h
# End Source File
# Begin Source File

SOURCE=..\Public\plane.h
# End Source File
# Begin Source File

SOURCE=..\Public\s3_intrf.h
# End Source File
# Begin Source File

SOURCE=.\ShellMessageWnd.h
# End Source File
# Begin Source File

SOURCE=.\Sprite.h
# End Source File
# Begin Source File

SOURCE=..\Public\studio.h
# End Source File
# Begin Source File

SOURCE=.\TextureSystem.h
# End Source File
# Begin Source File

SOURCE=..\Public\TGALoader.h
# End Source File
# Begin Source File

SOURCE=..\Public\TokenReader.h
# End Source File
# Begin Source File

SOURCE=..\Public\UtlBuffer.h
# End Source File
# Begin Source File

SOURCE=..\Public\utllinkedlist.h
# End Source File
# Begin Source File

SOURCE=..\Public\UtlMemory.h
# End Source File
# Begin Source File

SOURCE=..\Public\utlrbtree.h
# End Source File
# Begin Source File

SOURCE=..\Public\UtlVector.h
# End Source File
# Begin Source File

SOURCE=..\Public\vector.h
# End Source File
# Begin Source File

SOURCE=..\Public\vector2d.h
# End Source File
# Begin Source File

SOURCE=..\Public\vector4d.h
# End Source File
# Begin Source File

SOURCE=..\Public\vmatrix.h
# End Source File
# Begin Source File

SOURCE=..\Public\vplane.h
# End Source File
# Begin Source File

SOURCE=..\Public\wadtypes.h
# End Source File
# Begin Source File

SOURCE=.\WorldcraftBar.h
# End Source File
# Begin Source File

SOURCE=..\Public\worldsize.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\whatsnew.txt
# End Source File
# Begin Source File

SOURCE=..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
