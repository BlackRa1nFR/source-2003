@rem
@rem If necessary, do setlocal.
@rem Then start the build.
@rem

@if "%build_level%"=="" setlocal
@call start_build %1 %2 %3


@rem
@rem Choose debug or release build
@rem
if "%build_type%"=="debug" goto build_debug
goto build_release


:build_release

%MSDEV% tier0/tier0.dsp %CONFIG%"tier0 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vstdlib/vstdlib.dsp %CONFIG%"vstdlib - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% unitlib/unitlib.dsp %CONFIG%"unitlib - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/vgui_surfacelib/vgui_surfacelib.dsp %CONFIG%"vgui_surfacelib - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/src/vgui_dll.dsp %CONFIG%"vgui_dll - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/controls/vgui_controls.dsp %CONFIG%"vgui_controls - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/game_controls/game_controls.dsp %CONFIG%"game_controls - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vguimatsurface\vguimatsurface.dsp %CONFIG%"vguimatsurface - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% gameui\gameui.dsp %CONFIG%"gameui - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% engine\engine.dsp %CONFIG%"engine - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

@rem Client DLLs

%MSDEV% cl_dll\client.dsp %CONFIG%"client - Win32 Release TF2" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll\client.dsp %CONFIG%"client - Win32 Release HL2" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll\client.dsp %CONFIG%"client - Win32 Release HL1" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll\client.dsp %CONFIG%"client - Win32 Release CounterStrike" %build_target%
if errorlevel 1 set BUILD_ERROR=1

@rem Tracker projects

%MSDEV% Tracker\ServerBrowser\ServerBrowser.dsp %CONFIG%"ServerBrowser - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

@rem GameUI
%MSDEV% GameUI\GameUI.dsp %CONFIG%"GameUI - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done


:build_debug

%MSDEV% tier0/tier0.dsp %CONFIG%"tier0 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vstdlib/vstdlib.dsp %CONFIG%"vstdlib - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% unitlib/unitlib.dsp %CONFIG%"unitlib - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/vgui_surfacelib/vgui_surfacelib.dsp %CONFIG%"vgui_surfacelib - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/src/vgui_dll.dsp %CONFIG%"vgui_dll - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/controls/vgui_controls.dsp %CONFIG%"vgui_controls - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/game_controls/game_controls.dsp %CONFIG%"game_controls - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vguimatsurface\vguimatsurface.dsp %CONFIG%"vguimatsurface - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% gameui\gameui.dsp %CONFIG%"gameui - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% engine\engine.dsp %CONFIG%"engine - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

@rem Client DLLs

%MSDEV% cl_dll\client.dsp %CONFIG%"client - Win32 Debug TF2" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll\client.dsp %CONFIG%"client - Win32 Debug HL2" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll\client.dsp %CONFIG%"client - Win32 Debug HL1" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll\client.dsp %CONFIG%"client - Win32 Debug CounterStrike" %build_target%
if errorlevel 1 set BUILD_ERROR=1

@rem Tracker projects
%MSDEV% Tracker\ServerBrowser\ServerBrowser.dsp %CONFIG%"ServerBrowser - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

@rem GameUI
%MSDEV% GameUI\GameUI.dsp %CONFIG%"GameUI - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1


goto done


@rem
@rem All done
@rem
:done
call end_build