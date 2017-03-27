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

%MSDEV% materialsystem/shaderempty/shaderempty.dsp %CONFIG%"shaderempty - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

rem %MSDEV% materialsystem/shaderdx8/shaderdx8.dsp %CONFIG%"shaderdx8 - Win32 Release DX9" %build_target%
rem if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderdx8/shaderdx8.dsp %CONFIG%"shaderdx8 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/materialsystem.dsp %CONFIG%"MaterialSystem - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% StudioRender/StudioRender.dsp %CONFIG%"StudioRender - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% engine/engine.dsp %CONFIG%"engine - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll/client.dsp %CONFIG%"client - Win32 Release HL2" %build_target%
if errorlevel 1 set BUILD_ERROR=1

rem %MSDEV% cl_dll/client.dsp %CONFIG%"client - Win32 Release TF2" %build_target%
rem if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll/client.dsp %CONFIG%"client - Win32 Release HL1" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll/client.dsp %CONFIG%"client - Win32 Release CounterStrike" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vbsp/vbsp.dsp %CONFIG%"vbsp - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% worldcraft/worldcraft.dsp %CONFIG%"Worldcraft - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/mxtk/msvc/mxToolKitWin32.dsp %CONFIG%"mxToolKitWin32 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\hlmviewer\src\hlmv.dsp %CONFIG%"hlmv - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/studiomdl/studiomdl.dsp %CONFIG%"studiomdl - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\hlfaceposer\hlfaceposer.dsp %CONFIG%"hlfaceposer - win32 release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/vgui_surfacelib/vgui_surfacelib.dsp %CONFIG%"vgui_surfacelib - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/src/vgui_dll.dsp %CONFIG%"vgui_dll - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vguimatsurface\vguimatsurface.dsp %CONFIG%"vguimatsurface - win32 release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done


:build_debug
%MSDEV% materialsystem/shaderempty/shaderempty.dsp %CONFIG%"shaderempty - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

rem %MSDEV% materialsystem/shaderdx8/shaderdx8.dsp %CONFIG%"shaderdx8 - Win32 Debug DX9" %build_target%
rem if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderdx8/shaderdx8.dsp %CONFIG%"shaderdx8 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/materialsystem.dsp %CONFIG%"MaterialSystem - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% StudioRender/StudioRender.dsp %CONFIG%"StudioRender - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% engine/engine.dsp %CONFIG%"engine - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll/client.dsp %CONFIG%"client - Win32 Debug HL2" %build_target%
if errorlevel 1 set BUILD_ERROR=1

rem %MSDEV% cl_dll/client.dsp %CONFIG%"client - Win32 Debug TF2" %build_target%
rem if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll/client.dsp %CONFIG%"client - Win32 Debug HL1" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% cl_dll/client.dsp %CONFIG%"client - Win32 Debug CounterStrike" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vbsp/vbsp.dsp %CONFIG%"vbsp - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% worldcraft/worldcraft.dsp %CONFIG%"Worldcraft - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/mxtk/msvc/mxToolKitWin32.dsp %CONFIG%"mxToolKitWin32 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\hlmviewer\src\hlmv.dsp %CONFIG%"hlmv - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/studiomdl/studiomdl.dsp %CONFIG%"studiomdl - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\hlfaceposer\hlfaceposer.dsp %CONFIG%"hlfaceposer - win32 debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/vgui_surfacelib/vgui_surfacelib.dsp %CONFIG%"vgui_surfacelib - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/src/vgui_dll.dsp %CONFIG%"vgui_dll - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vguimatsurface\vguimatsurface.dsp %CONFIG%"vguimatsurface - win32 debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done


@rem
@rem All done
@rem
:done
call end_build