# Microsoft Developer Studio Project File - Name="scenemanager" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=scenemanager - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "scenemanager.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "scenemanager.mak" CFG="scenemanager - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "scenemanager - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "scenemanager - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "scenemanager"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "scenemanager - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"cbase.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\common" /I "..\..\Public" /I "..\..\Common" /I "..\mxtk\include" /I "..\..\game_shared" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /Yu"cbase.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 Winmm.lib Msimg32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"mainCRTStartup" /subsystem:windows /machine:I386 /nodefaultlib:"LIBCD"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
TargetPath=.\Release\scenemanager.exe
TargetName=scenemanager
InputPath=.\Release\scenemanager.exe
SOURCE="$(InputPath)"

"..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist "..\..\..\bin\$(TargetName).exe" attrib -r "..\..\..\bin\$(TargetName).exe" 
	if exist "$(TargetPath)" copy "$(TargetPath)" ..\..\..\bin 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "scenemanager - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"cbase.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\common" /I "..\..\Public" /I "..\..\Common" /I "..\mxtk\include" /I "..\..\game_shared" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PROTECTED_THINGS_DISABLE" /Yu"cbase.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Winmm.lib Msimg32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"mainCRTStartup" /subsystem:windows /debug /machine:I386 /pdbtype:sept
# Begin Custom Build
TargetPath=.\Debug\scenemanager.exe
TargetName=scenemanager
InputPath=.\Debug\scenemanager.exe
SOURCE="$(InputPath)"

"..\..\..\bin\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if exist "..\..\..\bin\$(TargetName).exe" attrib -r "..\..\..\bin\$(TargetName).exe" 
	if exist "$(TargetPath)" copy "$(TargetPath)" ..\..\..\bin 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "scenemanager - Win32 Release"
# Name "scenemanager - Win32 Debug"
# Begin Group "Precompile Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cbase.cpp
# ADD CPP /Yc"cbase.h"
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\basedialogparams.cpp
# End Source File
# Begin Source File

SOURCE=.\basedialogparams.h
# End Source File
# Begin Source File

SOURCE=.\cbase.h
# End Source File
# Begin Source File

SOURCE=..\..\public\characterset.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\drawhelper.cpp
# End Source File
# Begin Source File

SOURCE=.\drawhelper.h
# End Source File
# Begin Source File

SOURCE=.\fileloaderthread.cpp
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_helpers.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\public\filesystem_tools.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ifileloader.h
# End Source File
# Begin Source File

SOURCE=.\inputproperties.cpp
# End Source File
# Begin Source File

SOURCE=.\inputproperties.h
# End Source File
# Begin Source File

SOURCE=..\..\public\interface.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\interval.cpp
# End Source File
# Begin Source File

SOURCE=.\itreeitem.cpp
# End Source File
# Begin Source File

SOURCE=.\itreeitem.h
# End Source File
# Begin Source File

SOURCE=..\..\public\KeyValues.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\public\mathlib.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\memoverride.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\public\mempool.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\multiplerequest.cpp
# End Source File
# Begin Source File

SOURCE=.\multiplerequest.h
# End Source File
# Begin Source File

SOURCE=.\project.cpp
# End Source File
# Begin Source File

SOURCE=.\project.h
# End Source File
# Begin Source File

SOURCE=.\scene.cpp
# End Source File
# Begin Source File

SOURCE=.\scene.h
# End Source File
# Begin Source File

SOURCE=.\scenemanager.cpp
# End Source File
# Begin Source File

SOURCE=.\scenemanager_tools.cpp
# End Source File
# Begin Source File

SOURCE=.\scenemanager_tools.h
# End Source File
# Begin Source File

SOURCE=..\common\scriplib.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\soundbrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\soundbrowser.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\SoundEmitterSystemBase.cpp
# End Source File
# Begin Source File

SOURCE=.\soundentry.cpp
# End Source File
# Begin Source File

SOURCE=.\soundentry.h
# End Source File
# Begin Source File

SOURCE=.\soundproperties.cpp
# End Source File
# Begin Source File

SOURCE=.\soundproperties.h
# End Source File
# Begin Source File

SOURCE=.\soundproperties_multiple.cpp
# End Source File
# Begin Source File

SOURCE=.\soundproperties_multiple.h
# End Source File
# Begin Source File

SOURCE=.\statuswindow.cpp
# End Source File
# Begin Source File

SOURCE=.\statuswindow.h
# End Source File
# Begin Source File

SOURCE=.\tabwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\tabwindow.h
# End Source File
# Begin Source File

SOURCE=..\..\public\utlbuffer.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\public\utlsymbol.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\vcdfile.cpp
# End Source File
# Begin Source File

SOURCE=.\vcdfile.h
# End Source File
# Begin Source File

SOURCE=.\vssproperties.cpp
# End Source File
# Begin Source File

SOURCE=.\vssproperties.h
# End Source File
# Begin Source File

SOURCE=.\wavebrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\wavebrowser.h
# End Source File
# Begin Source File

SOURCE=.\wavefile.cpp
# End Source File
# Begin Source File

SOURCE=.\wavefile.h
# End Source File
# Begin Source File

SOURCE=.\waveproperties.cpp
# End Source File
# Begin Source File

SOURCE=.\waveproperties.h
# End Source File
# Begin Source File

SOURCE=.\workspace.cpp
# End Source File
# Begin Source File

SOURCE=.\workspace.h
# End Source File
# Begin Source File

SOURCE=.\workspacebrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\workspacebrowser.h
# End Source File
# Begin Source File

SOURCE=.\workspacemanager.cpp
# End Source File
# Begin Source File

SOURCE=.\workspacemanager.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\ico00003.ico
# End Source File
# Begin Source File

SOURCE=.\ico00004.ico
# End Source File
# Begin Source File

SOURCE=.\ico00005.ico
# End Source File
# Begin Source File

SOURCE=.\ico00006.ico
# End Source File
# Begin Source File

SOURCE=.\ico00007.ico
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\project1.ico
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\scenemanager.rc
# End Source File
# Begin Source File

SOURCE=.\vcd1.ico
# End Source File
# Begin Source File

SOURCE=.\wav1.ico
# End Source File
# Begin Source File

SOURCE=.\workspac.ico
# End Source File
# End Group
# Begin Group "Public Headers"

# PROP Default_Filter "*.h;*.cpp"
# Begin Source File

SOURCE=..\..\Public\amd3dx.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\basetypes.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\characterset.h
# End Source File
# Begin Source File

SOURCE=..\common\cmdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\commonmacros.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\convar.h
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\dbg.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\ExpressionSample.h
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

SOURCE=..\..\public\filesystem_tools.h
# End Source File
# Begin Source File

SOURCE=..\..\public\appframework\IAppSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\ichoreoeventcallback.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\engine\IEngineSound.h
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

SOURCE=..\..\game_shared\iscenetokenprocessor.h
# End Source File
# Begin Source File

SOURCE=..\..\public\KeyValues.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\MATHLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\public\tier0\mem.h
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

SOURCE=..\mxtk\include\mx\mx.h
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

SOURCE=..\mxtk\include\mx\mxlistview.h
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

SOURCE=..\..\public\tier0\platform.h
# End Source File
# Begin Source File

SOURCE=..\..\public\processor_detect.h
# End Source File
# Begin Source File

SOURCE=..\..\public\protected_things.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\random.h
# End Source File
# Begin Source File

SOURCE=..\common\scriplib.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\sharedInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\SoundEmitterSystemBase.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\soundflags.h
# End Source File
# Begin Source File

SOURCE=..\..\public\string_t.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\strtools.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\UtlBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\utldict.h
# End Source File
# Begin Source File

SOURCE=..\..\public\utllinkedlist.h
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

SOURCE=..\..\Public\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\Public\vector2d.h
# End Source File
# Begin Source File

SOURCE=..\..\public\vstdlib\vstdlib.h
# End Source File
# End Group
# Begin Group "Choreography"

# PROP Default_Filter "*.h;*.cpp"
# Begin Source File

SOURCE=..\..\game_shared\choreoactor.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreoactor.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreochannel.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreochannel.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreoevent.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreoevent.h
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreoscene.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\game_shared\choreoscene.h
# End Source File
# End Group
# Begin Group "Audio"

# PROP Default_Filter "*.h; *.cpp"
# Begin Source File

SOURCE=.\audiowaveoutput.h
# End Source File
# Begin Source File

SOURCE=.\iscenemanagersound.h
# End Source File
# Begin Source File

SOURCE=.\riff.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\riff.h
# End Source File
# Begin Source File

SOURCE=..\..\public\sentence.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Public\sentence.h
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

SOURCE=..\..\lib\common\mxtk.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\tier0.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\public\vstdlib.lib
# End Source File
# End Target
# End Project
