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

%MSDEV% launcher/launcher.dsp %CONFIG%"launcher - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% launcher_main/launcher_main.dsp %CONFIG%"launcher_main - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done


:build_debug

%MSDEV% launcher/launcher.dsp %CONFIG%"launcher - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% launcher_main/launcher_main.dsp %CONFIG%"launcher_main - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done


@rem
@rem All done
@rem
:done
call end_build