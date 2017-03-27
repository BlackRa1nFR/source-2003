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

%MSDEV% vtf/vtf.dsp %CONFIG%"vtf - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% shaderlib/shaderlib.dsp %CONFIG%"shaderlib - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_dbg.dsp %CONFIG%"stdshader_dbg - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_dx6.dsp %CONFIG%"stdshader_dx6 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_dx7.dsp %CONFIG%"stdshader_dx7 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_dx8.dsp %CONFIG%"stdshader_dx8 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_dx9.dsp %CONFIG%"stdshader_dx9 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_hdr_dx9.dsp %CONFIG%"stdshader_hdr_dx9 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\shader_nvfx.dsp %CONFIG%"shader_nvfx - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\shader_nvfx.dsp %CONFIG%"shader_nvfx - Win32 Release ps20" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderempty/shaderempty.dsp %CONFIG%"shaderempty - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderdx8/shaderdx8.dsp %CONFIG%"shaderdx8 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderdx8/playback/playback.dsp %CONFIG%"playback - Win32 Release DX9" %build_target%
rem %MSDEV% materialsystem/shaderdx8/playback/playback.dsp %CONFIG%"playback - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/materialsystem.dsp %CONFIG%"MaterialSystem - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/mxtk/msvc/mxtoolkitwin32.dsp %CONFIG%"mxtoolkitwin32 - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/vgui_surfacelib/vgui_surfacelib.dsp %CONFIG%"vgui_surfacelib - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/src/vgui_dll.dsp %CONFIG%"vgui_dll - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/controls/vgui_controls.dsp %CONFIG%"vgui_controls - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vphysics/vphysics.dsp %CONFIG%"vphysics - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% StudioRender/StudioRender.dsp %CONFIG%"StudioRender - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

rem %MSDEV% utils/ScratchPad3DViewer/ScratchPad3DViewer.dsp %CONFIG%"ScratchPad3DViewer - Win32 Release" %build_target%
rem if errorlevel 1 set BUILD_ERROR=1

%MSDEV% filesystem/filesystem_stdio/filesystem_stdio.dsp %CONFIG%"FileSystem_Stdio - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% filesystem/filesystem_stdio/filesystem_stdio.dsp %CONFIG%"FileSystem_Stdio - Win32 Release STEAM" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% appframework\appframework.dsp %CONFIG%"appframework - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vguimatsurface\vguimatsurface.dsp %CONFIG%"vguimatsurface - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% gameui\gameui.dsp %CONFIG%"gameui - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% engine\voice_codecs\miles\miles.dsp %CONFIG%"miles - Win32 Release" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% tracker\serverbrowser\serverbrowser.dsp %CONFIG%"ServerBrowser - Win32 Release" %build_target%
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

%MSDEV% vtf/vtf.dsp %CONFIG%"vtf - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% shaderlib/shaderlib.dsp %CONFIG%"shaderlib - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_dbg.dsp %CONFIG%"stdshader_dbg - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_dx6.dsp %CONFIG%"stdshader_dx6 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_dx7.dsp %CONFIG%"stdshader_dx7 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_dx8.dsp %CONFIG%"stdshader_dx8 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_dx9.dsp %CONFIG%"stdshader_dx9 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\stdshader_hdr_dx9.dsp %CONFIG%"stdshader_hdr_dx9 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\shader_nvfx.dsp %CONFIG%"shader_nvfx - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem\stdshaders\shader_nvfx.dsp %CONFIG%"shader_nvfx - Win32 Debug ps20" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% unitlib/unitlib.dsp %CONFIG%"unitlib - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderempty/shaderempty.dsp %CONFIG%"shaderempty - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderdx8/shaderdx8.dsp %CONFIG%"shaderdx8 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/shaderdx8/playback/playback.dsp %CONFIG%"playback - Win32 Debug DX9" %build_target%
rem %MSDEV% materialsystem/shaderdx8/playback/playback.dsp %CONFIG%"playback - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% materialsystem/materialsystem.dsp %CONFIG%"MaterialSystem - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% utils/mxtk/msvc/mxtoolkitwin32.dsp %CONFIG%"mxtoolkitwin32 - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/vgui_surfacelib/vgui_surfacelib.dsp %CONFIG%"vgui_surfacelib - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/src/vgui_dll.dsp %CONFIG%"vgui_dll - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vgui2/controls/vgui_controls.dsp %CONFIG%"vgui_controls - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vphysics/vphysics.dsp %CONFIG%"vphysics - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% StudioRender/StudioRender.dsp %CONFIG%"StudioRender - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

rem %MSDEV% utils/ScratchPad3DViewer/ScratchPad3DViewer.dsp %CONFIG%"ScratchPad3DViewer - Win32 Debug" %build_target%
rem if errorlevel 1 set BUILD_ERROR=1

%MSDEV% filesystem/filesystem_stdio/filesystem_stdio.dsp %CONFIG%"FileSystem_Stdio - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% filesystem/filesystem_stdio/filesystem_stdio.dsp %CONFIG%"FileSystem_Stdio - Win32 Debug STEAM" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% appframework\appframework.dsp %CONFIG%"appframework - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% vguimatsurface\vguimatsurface.dsp %CONFIG%"vguimatsurface - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% gameui\gameui.dsp %CONFIG%"gameui - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% engine\voice_codecs\miles\miles.dsp %CONFIG%"miles - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

@rem GameUI
%MSDEV% GameUI\GameUI.dsp %CONFIG%"GameUI - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

%MSDEV% tracker\serverbrowser\serverbrowser.dsp %CONFIG%"ServerBrowser - Win32 Debug" %build_target%
if errorlevel 1 set BUILD_ERROR=1

goto done

@rem
@rem All done
@rem
:done
call end_build