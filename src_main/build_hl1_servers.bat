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

@REM Master server (not for end-users)
REM %MSDEV% master/hlmaster.dsp %CONFIG%"HLMaster - Win32 Release" %1
@REM Mod Master server (not for end-users)
REM %MSDEV% master/hlmodmaster/hlmodmaster.dsp %CONFIG%"HLMaster - Win32 Release" %1
@REM Master server status watch (Valve only diagnostic tool)
REM %MSDEV% status/status.dsp %CONFIG%"Status - Win32 Release" %1

goto done


:build_debug

goto done


@rem
@rem All done
@rem
:done
call end_build


