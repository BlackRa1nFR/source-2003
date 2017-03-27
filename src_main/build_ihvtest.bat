
@rem
@rem If necessary, do setlocal.
@rem Then start the build.
@rem

@if "%build_level%"=="" setlocal
@call start_build %1 %2 %3

@echo off

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

%MSDEV% materialsystem/shaderempty/shaderempty.dsp %CONFIG%"shaderempty - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderdx8/shaderdx8_ihvtest.dsp %CONFIG%"shaderdx8_ihvtest - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/materialsystemihvtest.dsp %CONFIG%"MaterialSystemIHVTest - Win32 IHVTest Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% filesystem/filesystem_stdio/filesystem_stdio.dsp %CONFIG%"FileSystem_Stdio - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% StudioRender/StudioRender.dsp %CONFIG%"StudioRender - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/ihvtest1/ihvtest1.dsp %CONFIG%"ihvtest1 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done


:build_debug

%MSDEV% tier0/tier0.dsp %CONFIG%"tier0 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vstdlib/vstdlib.dsp %CONFIG%"vstdlib - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderempty/shaderempty.dsp %CONFIG%"shaderempty - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderdx8/shaderdx8_ihvtest.dsp %CONFIG%"shaderdx8_ihvtest - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/materialsystemihvtest.dsp %CONFIG%"MaterialSystemIHVTest - Win32 IHVTest Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% filesystem/filesystem_stdio/filesystem_stdio.dsp %CONFIG%"FileSystem_Stdio - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% StudioRender/StudioRender.dsp %CONFIG%"StudioRender - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/ihvtest1/ihvtest1.dsp %CONFIG%"ihvtest1 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done


@rem
@rem All done
@rem
:done
call end_build