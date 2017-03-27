# Microsoft Developer Studio Project File - Name="hlfaceposer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=hlfaceposer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hlfaceposer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hlfaceposer.mak" CFG="hlfaceposer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hlfaceposer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "hlfaceposer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Src/utils/hlfaceposer", TCVCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hlfaceposer - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GR /Zi /O2 /I "../../utils/hlfaceposer" /I "../../hlfaceposer" /I "../mxtk/include" /I "../../public" /I "../../common" /I "../common" /I "../hlmviewer/src" /I "../../game_shared" /I "../SAPI51/Include" /I "./lipsinc" /I "../phonemeextractor" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /D "_WIN32" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ole32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib winmm.lib Msimg32.lib /nologo /entry:"mainCRTStartup" /subsystem:windows /map /debug /machine:I386 /nodefaultlib:"MSVCRT" /nodefaultlib:"LIBC" /libpath:"../sapi51/lib/i386" /libpath:"..\..\..\src\lib\common" /libpath:"..\..\..\src\lib\public" /libpath:"./lipsinc"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetPath=.\Release\hlfaceposer.exe
TargetName=hlfaceposer
InputPath=.\Release\hlfaceposer.exe
SOURCE="$(InputPath)"

"..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist "..\..\..\bin\$(TargetName).exe" attrib -r "..\..\..\bin\$(TargetName).exe" 
	if exist "$(TargetPath)" copy "$(TargetPath)" ..\..\..\bin 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "hlfaceposer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hlfaceposer___Win32_Debug0"
# PROP BASE Intermediate_Dir "hlfaceposer___Win32_Debug0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /ZI /Od /I "../../utils/hlfaceposer" /I "../../hlfaceposer" /I "../mxtk/include" /I "../../public" /I "../../common" /I "../common" /I "../hlmviewer/src" /I "../../game_shared" /I "../SAPI51/Include" /I "./lipsinc" /I "../phonemeextractor" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /D "_WIN32" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ole32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib winmm.lib Msimg32.lib /nologo /entry:"mainCRTStartup" /subsystem:windows /debug /machine:I386 /nodefaultlib:"LIBC" /nodefaultlib:"MSVCRT" /pdbtype:sept /libpath:"../sapi51/lib/i386" /libpath:"..\..\..\src\lib\common" /libpath:"..\..\..\src\lib\public" /libpath:"./lipsinc"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetPath=.\Debug\hlfaceposer.exe
TargetName=hlfaceposer
InputPath=.\Debug\hlfaceposer.exe
SOURCE="$(InputPath)"

"..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist "..\..\..\bin\$(TargetName).exe" attrib -r "..\..\..\bin\$(TargetName).exe" 
	if exist "$(TargetPath)" copy "$(TargetPath)" ..\..\..\bin 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "hlfaceposer - Win32 Release"
# Name "hlfaceposer - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Choreo Widgets"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\ChoreoActorWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ChoreoActorWidget.h
# End Source File
# Begin Source File

SOURCE=.\ChoreoChannelWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ChoreoChannelWidget.h
# End Source File
# Begin Source File

SOURCE=.\ChoreoEventWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ChoreoEventWidget.h
# End Source File
# Begin Source File

SOURCE=.\ChoreoGlobalEventWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ChoreoGlobalEventWidget.h
# End Source File
# Begin Source File

SOURCE=.\ChoreoWidget.cpp
# End Source File
# Begin Source File

SOURCE=.\ChoreoWidget.h
# End Source File
# Begin Source File

SOURCE=.\ChoreoWidgetDrawHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\ChoreoWidgetDrawHelper.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ActorProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\ActorProperties.h
# End Source File
# Begin Source File

SOURCE=.\addsoundentry.cpp
# End Source File
# Begin Source File

SOURCE=.\addsoundentry.h
# End Source File
# Begin Source File

SOURCE=.\basedialogparams.cpp
# End Source File
# Begin Source File

SOURCE=.\basedialogparams.h
# End Source File
# Begin Source File

SOURCE=.\ChannelProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\ChannelProperties.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\checksum_crc.cpp
# End Source File
# Begin Source File

SOURCE=.\choiceproperties.cpp
# End Source File
# Begin Source File

SOURCE=.\choiceproperties.h
# End Source File
# Begin Source File

SOURCE=.\ChoreoView.cpp
# End Source File
# Begin Source File

SOURCE=.\ChoreoView.h
# End Source File
# Begin Source File

SOURCE=.\ChoreoViewColors.h
# End Source File
# Begin Source File

SOURCE=.\CloseCaptionManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ControlPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\ControlPanel.h
# End Source File
# Begin Source File

SOURCE=.\EditPhrase.cpp
# End Source File
# Begin Source File

SOURCE=.\EditPhrase.h
# End Source File
# Begin Source File

SOURCE=.\EventProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\EventProperties.h
# End Source File
# Begin Source File

SOURCE=.\expclass.cpp
# End Source File
# Begin Source File

SOURCE=.\expclass.h
# End Source File
# Begin Source File

SOURCE=.\expression.cpp
# End Source File
# Begin Source File

SOURCE=.\expression.h
# End Source File
# Begin Source File

SOURCE=.\ExpressionProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\ExpressionProperties.h
# End Source File
# Begin Source File

SOURCE=.\expressions.cpp
# End Source File
# Begin Source File

SOURCE=.\expressions.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\expressionsample.h
# End Source File
# Begin Source File

SOURCE=.\ExpressionTool.cpp
# End Source File
# Begin Source File

SOURCE=.\ExpressionTool.h
# End Source File
# Begin Source File

SOURCE=.\faceposer_models.cpp
# End Source File
# Begin Source File

SOURCE=.\faceposer_models.h
# End Source File
# Begin Source File

SOURCE=.\faceposertoolwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\faceposertoolwindow.h
# End Source File
# Begin Source File

SOURCE=.\FacePoserWorkspace.cpp
# End Source File
# Begin Source File

SOURCE=.\FlexPanel.cpp
# End Source File
# Begin Source File

SOURCE=.\FlexPanel.h
# End Source File
# Begin Source File

SOURCE=.\GestureTool.cpp
# End Source File
# Begin Source File

SOURCE=.\GestureTool.h
# End Source File
# Begin Source File

SOURCE=.\GlobalEventProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\GlobalEventProperties.h
# End Source File
# Begin Source File

SOURCE=.\hlfaceposer.cpp
# End Source File
# Begin Source File

SOURCE=.\hlfaceposer.h
# End Source File
# Begin Source File

SOURCE=.\ICloseCaptionManager.h
# End Source File
# Begin Source File

SOURCE=.\ifaceposersound.h
# End Source File
# Begin Source File

SOURCE=.\ifaceposerworkspace.h
# End Source File
# Begin Source File

SOURCE=.\InputProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\InputProperties.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\interval.cpp
# End Source File
# Begin Source File

SOURCE=.\matsyswin.cpp
# End Source File
# Begin Source File

SOURCE=.\matsyswin.h
# End Source File
# Begin Source File

SOURCE=.\mdlviewer.cpp
# End Source File
# Begin Source File

SOURCE=.\mdlviewer.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\mempool.cpp
# End Source File
# Begin Source File

SOURCE=.\mxbitmapbutton.cpp
# End Source File
# Begin Source File

SOURCE=.\mxbitmapbutton.h
# End Source File
# Begin Source File

SOURCE=.\mxbitmaptools.cpp
# End Source File
# Begin Source File

SOURCE=.\mxbitmaptools.h
# End Source File
# Begin Source File

SOURCE=.\mxbitmapwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\mxbitmapwindow.h
# End Source File
# Begin Source File

SOURCE=.\mxexpressionslider.cpp
# End Source File
# Begin Source File

SOURCE=.\mxexpressionslider.h
# End Source File
# Begin Source File

SOURCE=.\mxExpressionTab.cpp
# End Source File
# Begin Source File

SOURCE=.\mxexpressiontab.h
# End Source File
# Begin Source File

SOURCE=.\mxexpressiontray.cpp
# End Source File
# Begin Source File

SOURCE=.\mxexpressiontray.h
# End Source File
# Begin Source File

SOURCE=.\mxstatuswindow.cpp
# End Source File
# Begin Source File

SOURCE=.\mxstatuswindow.h
# End Source File
# Begin Source File

SOURCE=.\PhonemeConverter.cpp
# End Source File
# Begin Source File

SOURCE=.\PhonemeConverter.h
# End Source File
# Begin Source File

SOURCE=.\PhonemeEditor.cpp
# End Source File
# Begin Source File

SOURCE=.\PhonemeEditor.h
# End Source File
# Begin Source File

SOURCE=.\PhonemeEditor_CloseCaption.cpp
# End Source File
# Begin Source File

SOURCE=.\PhonemeEditorColors.h
# End Source File
# Begin Source File

SOURCE=.\PhonemeProperties.cpp
# End Source File
# Begin Source File

SOURCE=.\PhonemeProperties.h
# End Source File
# Begin Source File

SOURCE=.\RampTool.cpp
# End Source File
# Begin Source File

SOURCE=.\RampTool.h
# End Source File
# Begin Source File

SOURCE=.\SceneRampTool.cpp
# End Source File
# Begin Source File

SOURCE=.\SceneRampTool.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\sentence.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\sentence.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\SoundEmitterSystemBase.cpp
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\SoundEmitterSystemBase.h
# End Source File
# Begin Source File

SOURCE=.\soundlookup.cpp
# End Source File
# Begin Source File

SOURCE=.\soundlookup.h
# End Source File
# Begin Source File

SOURCE=.\tabwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\tabwindow.h
# End Source File
# Begin Source File

SOURCE=.\TimelineItem.cpp
# End Source File
# Begin Source File

SOURCE=.\TimelineItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\utlsymbol.cpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\hlfaceposer.rc
# End Source File
# Begin Source File

SOURCE=.\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\icon2.ico
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Shared Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\game_shared\bone_setup.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\characterset.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\CollisionUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\FileSystem_Tools.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\ImageLoader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\iscenetokenprocessor.h
# End Source File
# Begin Source File

SOURCE=..\..\public\KeyValues.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\Mathlib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\physdll.cpp
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\physmesh.cpp
# End Source File
# Begin Source File

SOURCE=..\common\scriplib.cpp
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\studio_flex.cpp
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\studio_render.cpp
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\studio_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\ViewerSettings.cpp
# End Source File
# End Group
# Begin Group "Shared Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Public\amd3dx.h
# End Source File
# Begin Source File

SOURCE=..\..\public\basehandle.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\bone_setup.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\BSPFILE.H
# End Source File
# Begin Source File

SOURCE=..\..\Public\bspflags.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\bumpvects.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\characterset.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\checksum_crc.h
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\cmodel.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\CollisionUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\public\Color.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\commonmacros.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\const.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vphysics\constraints.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\convar.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\dbg.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\fasttimer.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_helpers.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\FileSystem_Tools.h
# End Source File
# Begin Source File

SOURCE=..\..\public\gametrace.h
# End Source File
# Begin Source File

SOURCE=..\..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\ICommandLine.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\engine\IEngineSound.h
# End Source File
# Begin Source File

SOURCE=..\..\public\ihandleentity.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\IKeyValuesSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\ImageLoader.h
# End Source File
# Begin Source File

SOURCE=..\..\public\materialsystem\imaterial.h
# End Source File
# Begin Source File

SOURCE=..\..\public\materialsystem\imaterialproxyfactory.h
# End Source File
# Begin Source File

SOURCE=..\..\public\materialsystem\imaterialsystem.h
# End Source File
# Begin Source File

SOURCE=..\..\public\materialsystem\imaterialsystemhardwareconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\public\materialsystem\imaterialvar.h
# End Source File
# Begin Source File

SOURCE=..\..\public\materialsystem\imesh.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\interface.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\interval.h
# End Source File
# Begin Source File

SOURCE=..\..\public\irecipientfilter.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\IStudioRender.h
# End Source File
# Begin Source File

SOURCE=..\..\public\materialsystem\itexture.h
# End Source File
# Begin Source File

SOURCE=..\..\public\KeyValues.h
# End Source File
# Begin Source File

SOURCE=.\mapentities.h
# End Source File
# Begin Source File

SOURCE=..\..\public\materialsystem\materialsystem_config.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\MATHLIB.H
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\matsyswin.h
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\mdlviewer.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\mem.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memalloc.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memdbgoff.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memdbgon.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\MemPool.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\mouthinfo.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mx.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxBmp.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxButton.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxCheckBox.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxChoice.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxChooseColor.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxEvent.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxFileDialog.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxGlWindow.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxGroupBox.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxImage.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxInit.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxLabel.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxLineEdit.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxLinkedList.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxListBox.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxMatSysWindow.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxMenu.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxMenuBar.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxMessageBox.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxpath.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxPcx.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxPopupMenu.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxProgressBar.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxRadioButton.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxScrollbar.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxSlider.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxstring.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxTab.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxTga.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxToggleButton.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxToolTip.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxTreeView.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxWidget.h
# End Source File
# Begin Source File

SOURCE=..\mxtk\include\mx\mxWindow.h
# End Source File
# Begin Source File

SOURCE=..\..\public\networkvar.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\npcevent.h
# End Source File
# Begin Source File

SOURCE=..\phonemeextractor\phonemeextractor.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\phyfile.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\physdll.h
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\physmesh.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\plane.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\platform.h
# End Source File
# Begin Source File

SOURCE=..\..\public\protected_things.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\random.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\s3_intrf.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\sapi.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\sapiddk.h
# End Source File
# Begin Source File

SOURCE=..\common\scriplib.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\sharedInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\soundflags.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\Spddkhlp.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\spdebug.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\sperror.h
# End Source File
# Begin Source File

SOURCE=..\sapi51\Include\sphelper.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\string_t.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\studio.h
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\studio_render.h
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\StudioModel.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\TGALoader.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\utldict.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\utllinkedlist.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlMemory.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\utlrbtree.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\utlsymbol.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlVector.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vcollide.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vcollide_parse.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vector2d.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vector4d.h
# End Source File
# Begin Source File

SOURCE=..\hlmviewer\src\ViewerSettings.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vphysics_interface.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\vprof.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\vstdlib.h
# End Source File
# End Group
# Begin Group "Choreography"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=..\..\game_shared\choreoactor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreoactor.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreochannel.cpp
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreochannel.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreoevent.cpp
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreoevent.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreoscene.cpp
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreoscene.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\ichoreoeventcallback.h
# End Source File
# End Group
# Begin Group "Audio Code"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\AudioWaveOutput.h
# End Source File
# Begin Source File

SOURCE=.\riff.cpp
# End Source File
# Begin Source File

SOURCE=.\riff.h
# End Source File
# Begin Source File

SOURCE=.\snd_audio_source.cpp
# End Source File
# Begin Source File

SOURCE=.\snd_audio_source.h
# End Source File
# Begin Source File

SOURCE=.\snd_wave_mixer.cpp
# End Source File
# Begin Source File

SOURCE=.\snd_wave_mixer.h
# End Source File
# Begin Source File

SOURCE=.\snd_wave_mixer_adpcm.cpp
# End Source File
# Begin Source File

SOURCE=.\snd_wave_mixer_adpcm.h
# End Source File
# Begin Source File

SOURCE=.\snd_wave_mixer_private.h
# End Source File
# Begin Source File

SOURCE=.\snd_wave_source.cpp
# End Source File
# Begin Source File

SOURCE=.\snd_wave_source.h
# End Source File
# Begin Source File

SOURCE=.\sound.cpp
# End Source File
# Begin Source File

SOURCE=.\sound.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\cbase.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\common\mxtk.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\common\s3tc.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# End Target
# End Project
