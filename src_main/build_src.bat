@if "%overbose%" == "" echo off
set BUILD_DIR=u:\goldsrc
REM ----------------------------------
REM build_update.bat
REM Erik Johnson 04.23.2001
REM ----------------------------------
setlocal
set PATH=%PATH%;devtools\build\components
@call goldsrc_settings.bat
REM ------------------------------------------------------------------
REM figure out what to build
REM ------------------------------------------------------------------
if "%1" == "" goto :Usage


:ProjectLoop

if "%1" == "" goto :End	
set SSDIR=\\jeeves\hl2vss
for %%a in (%1) do				if /i "%1" == "%%a" (set build=%1) & goto SelectBuild


REM ------------------------------------------------------------------
REM iterate on inputs
REM ------------------------------------------------------------------
:SelectBuild

if %build% == sync					set TASK=sync_all_goldsrc.bat				& goto StartBuild
if %build% == clean					set _clean=/REBUILD							& goto ArgLoop

if %build% == all					set TASK=build_all_goldsrc.bat				& goto StartBuild
if %build% == mods					set TASK=build_all_mods.bat					& goto StartBuild

if %build% == vgui					set TASK=build_vgui_dll.bat					& goto StartBuild

if %build% == hl_launcher			set TASK=build_hl_launcher.bat				& goto StartBuild
if %build% == cs_launcher			set TASK=build_cs_launcher.bat				& goto StartBuild

if %build% == engine				set TASK=build_engine.bat					& goto StartBuild

if %build% == dedicated_server		set TASK=build_dedicated_server.bat			& goto StartBuild
if %build% == proxy_server			set TASK=build_proxy_server.bat				& goto StartBuild

if %build% == hl_game				set TASK=build_hl_game_dll.bat				& goto StartBuild
if %build% == tfc_game				set TASK=build_tfc_game_dll.bat				& goto StartBuild
if %build% == dmc_game				set TASK=build_dmc_game_dll.bat				& goto StartBuild
if %build% == 3wave_game			set TASK=build_3wave_game_dll.bat			& goto StartBuild
if %build% == cs_game				set TASK=build_cs_game_dll.bat				& goto StartBuild

if %build% == hl_client				set TASK=build_hl_client_dll.bat			& goto StartBuild
if %build% == tfc_client			set TASK=build_tfc_client_dll.bat			& goto StartBuild
if %build% == dmc_client			set TASK=build_dmc_client_dll.bat			& goto StartBuild
if %build% == 3wave_client			set TASK=build_3wave_client_dll.bat			& goto StartBuild
if %build% == cs_client				set TASK=build_cs_client_dll.bat			& goto StartBuild

echo skipping unknown argument %1
shift
goto :ProjectLoop

:ArgLoop
shift
goto ProjectLoop

:Usage
echo ------------------------------------------------------------------
echo build_goldsrc.bat
echo.
echo Official Valve Goldsrc Build Batchfile
echo.
echo Example: build_goldsrc 
echo.
echo.
echo Usage: build_goldsrc [sync][clean][component/group]
echo.
echo Where:
echo		sync		(syncs up to all of $/goldsrc)
echo		clean		clean (does a rebuild all)
echo		group		all (builds everything)
echo				mods (all game and client .dlls)
echo		component	vgui
echo				hl_launcher
echo				cs_launcher
echo				engine
echo				dedicated_server
echo				proxy_server
echo				hl_game
echo				tfc_game
echo				dmc_game
echo				3wave_game
echo				cs_game
echo				ricochet_game
echo				hl_client
echo				tfc_client
echo				dmc_client
echo				3wave_client
echo				cs_client
echo				ricochet_client


echo ------------------------------------------------------------------
goto :End

:StartBuild
pushd %COMPONENTS%
@call %TASK% %_clean%
popd
if %build% == all goto End
goto NextBuild

REM ------------------------------------------------------------------
REM restart loop
REM ------------------------------------------------------------------
:NextBuild
shift
goto :ProjectLoop

REM ------------------------------------------------------------------
REM end of build
REM ------------------------------------------------------------------
:End
echo Build Process Completed