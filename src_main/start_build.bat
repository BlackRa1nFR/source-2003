@echo off

if "%USE_INCREDIBUILD%"=="" goto no_incredibuild

set MSDEV=BuildConsole
set CONFIG=/ShowTime /ShowAgent /nologo /cfg=
goto after_incredibuild

:no_incredibuild
set MSDEV=msdev
set CONFIG=/make 

:after_incredibuild

@rem
@rem Set default variable values
@rem
:set_default_variables
set build_target=
set build_type=release


@rem
@rem Now check the command line parameters
@rem
:check_command_line

if "%1"=="" goto finished_command_line

if "%1"=="DEBUG" goto set_debug
if "%1"=="debug" goto set_debug
if "%1"=="Debug" goto set_debug
if "%1"=="D" goto set_debug
if "%1"=="d" goto set_debug

@rem Didn't find any matches, so must be build target
goto set_target

:set_debug
set build_type=debug
shift
goto check_command_line

:set_target
set build_target=%1
shift
goto check_command_line

@rem
@rem Done with the command line.  If build_level=="" (ie,
@rem we're the top-level script), clear build_error
@rem and the build log and set up the VC environment variables.
@rem
:finished_command_line

if "%build_level%"=="1" goto increment_build_level
if "%build_level%"=="2" goto increment_build_level

echo on
set BUILD_ERROR=
del build.log
call vcvars32


@rem
@rem All done.
@rem
:increment_build_level
if "%build_level%"=="2" set build_level=3
if "%build_level%"=="1" set build_level=2
if "%build_level%"=="" set build_level=1
