@rem
@rem If we're the top-level process and there were errors,
@rem issue a warning
@rem
if "%build_level%"=="2" goto done
if "%build_level%"=="3" goto done

if "%BUILD_ERROR%"=="" goto build_ok

echo *********************
echo *********************
echo *** Build Errors! ***
echo *********************
echo *********************
goto done


@rem
@rem Successful build
@rem
:build_ok

echo .
echo Build succeeded!
echo .


@rem
@rem All done.  Decrement build_level
@rem
:done

if "%build_level%"=="1" set build_level=
if "%build_level%"=="2" set build_level=1
if "%build_level%"=="3" set build_level=2
