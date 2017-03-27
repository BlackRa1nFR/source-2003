@rem
@rem If necessary, do setlocal.
@rem Then start the build.
@rem

@if "%build_level%"=="" setlocal
@call start_build %1 %2 %3


call build_hl2.bat		%1 %2 /rebuild
call build_tf2_game.bat %1 %2 /rebuild
call build_hl1_game.bat %1 %2 /rebuild


@rem
@rem All done
@rem
:done
call end_build