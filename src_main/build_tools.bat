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

%MSDEV% utils/vmpi/vmpi.dsp %CONFIG%"vmpi - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vbspinfo/vbspinfo.dsp %CONFIG%"vbspinfo - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vbsp/vbsp.dsp %CONFIG%"vbsp - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vrad/vrad.dsp %CONFIG%"vrad - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vrad_launcher/vrad_launcher.dsp %CONFIG%"vrad_launcher - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vvis/vvis.dsp %CONFIG%"vvis - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vvis_launcher/vvis_launcher.dsp %CONFIG%"vvis_launcher - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

rem Comment this out because it doesn't build
rem %MSDEV% utils/sentencelength/sentencelength.dsp %CONFIG%"sentencelength - Win32 Release" %build_target%
rem if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/nvtristriplib/nvtristriplib.dsp %CONFIG%"NVTriStripLib - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/studiomdl/studiomdl.dsp %CONFIG%"studiomdl - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\hlmviewer\src\hlmv.dsp %CONFIG%"hlmv - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\hlfaceposer\hlfaceposer.dsp %CONFIG%"hlfaceposer - win32 release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\scenemanager\scenemanager.dsp %CONFIG%"scenemanager - win32 release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\phonemeextractor\phonemeextractor.dsp %CONFIG%"phonemeextractor - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\phonemeextractor\phonemeextractor.dsp %CONFIG%"phonemeextractor - Win32 Release IMS" %build_target%
if errorlevel 1 set BUILD_ERROR=1

@REM This doesn't need to be built, but the MRM versions do
@REM Add them when they are in sourcesafe
@REM %MSDEV% utils/smdlexp/smdlexp.dsw %CONFIG%"smdlexp - Win32 Release" %build_target%

%MSDEV% utils/newdat/newdat.dsp %CONFIG%"newdat - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vtex/vtex.dsp %CONFIG%"vtex - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vtex/vtex_dll.dsp %CONFIG%"vtex_dll - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/unittest/unittest.dsp %CONFIG%"unittest - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/bsppack/bsppack.dsp %CONFIG%"bsppack - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/bspzip/bspzip.dsp %CONFIG%"bspzip - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/height2normal/height2normal.dsp %CONFIG%"height2normal - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vtf2tga/vtf2tga.dsp %CONFIG%"vtf2tga - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/bugreporter/bugreporter.dsp %CONFIG%"bugreporter - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done


:build_debug

%MSDEV% utils/vmpi/vmpi.dsp %CONFIG%"vmpi - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vbspinfo/vbspinfo.dsp %CONFIG%"vbspinfo - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vbsp/vbsp.dsp %CONFIG%"vbsp - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vrad/vrad.dsp %CONFIG%"vrad - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vrad_launcher/vrad_launcher.dsp %CONFIG%"vrad_launcher - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vvis/vvis.dsp %CONFIG%"vvis - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vvis_launcher/vvis_launcher.dsp %CONFIG%"vvis_launcher - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

rem Comment this out because it doesn't build
rem %MSDEV% utils/sentencelength/sentencelength.dsp %CONFIG%"sentencelength - Win32 Debug" %build_target%
rem if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/nvtristriplib/nvtristriplib.dsp %CONFIG%"NVTriStripLib - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/studiomdl/studiomdl.dsp %CONFIG%"studiomdl - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\hlmviewer\src\hlmv.dsp %CONFIG%"hlmv - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\hlfaceposer\hlfaceposer.dsp %CONFIG%"hlfaceposer - win32 debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\scenemanager\scenemanager.dsp %CONFIG%"scenemanager - win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\phonemeextractor\phonemeextractor.dsp %CONFIG%"phonemeextractor - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils\phonemeextractor\phonemeextractor.dsp %CONFIG%"phonemeextractor - Win32 Debug IMS" %build_target%
if errorlevel 1 set BUILD_ERROR=1

@REM This doesn't need to be built, but the MRM versions do
@REM Add them when they are in sourcesafe
@REM %MSDEV% utils/smdlexp/smdlexp.dsw %CONFIG%"smdlexp - Win32 Debug" %build_target%

%MSDEV% utils/newdat/newdat.dsp %CONFIG%"newdat - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vtex/vtex.dsp %CONFIG%"vtex - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vtex/vtex_dll.dsp %CONFIG%"vtex_dll - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/unittest/unittest.dsp %CONFIG%"unittest - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/bsppack/bsppack.dsp %CONFIG%"bsppack - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/bspzip/bspzip.dsp %CONFIG%"bspzip - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/height2normal/height2normal.dsp %CONFIG%"height2normal - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vtf2tga/vtf2tga.dsp %CONFIG%"vtf2tga - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/bugreporter/bugreporter.dsp %CONFIG%"bugreporter - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done


@rem
@rem All done
@rem
:done
call end_build