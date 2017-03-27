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

%MSDEV% utils/vrad/vrad.dsp %CONFIG%"vrad - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vvis/vvis.dsp %CONFIG%"vvis - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vmpi/vmpi_service/vmpi_service.dsp %CONFIG%"vmpi_service - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vmpi/vmpi_service_ui/vmpi_service_ui.dsp %CONFIG%"vmpi_service_ui - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1


goto done



:build_debug

%MSDEV% utils/vmpi/vmpi.dsp %CONFIG%"vmpi - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vrad/vrad.dsp %CONFIG%"vrad - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vvis/vvis.dsp %CONFIG%"vvis - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vmpi/vmpi_service/vmpi_service.dsp %CONFIG%"vmpi_service - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/vmpi/vmpi_service_ui/vmpi_service_ui.dsp %CONFIG%"vmpi_service_ui - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done


@rem
@rem All done
@rem
:done
call end_build