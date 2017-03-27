@rem
@rem If necessary, do setlocal.
@rem Then start the build.
@rem

@if "%build_level%"=="" setlocal
@call start_build %1 %2 %3


call build_common %1 %2 %3
call build_tools %1 %2 %3
call build_engine %1 %2 %3
call build_worldcraft %1 %2 %3
call build_hl2_game %1 %2 %3
call build_launcher %1 %2 %3
call build_hl2_servers %1 %2 %3


@rem
@rem All done
@rem
:done
call end_build