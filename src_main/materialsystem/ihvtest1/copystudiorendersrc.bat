..\..\devtools\bin\perl ..\..\..\bin\makedirhier.pl \ihvtest1\src\studiorender
for /r %%i in (..\..\studiorender\*.cpp) do ..\..\devtools\bin\perl ihvtestcopy.pl %%i \ihvtest1\src\studiorender
for /r %%i in (..\..\studiorender\*.h) do ..\..\devtools\bin\perl ihvtestcopy.pl %%i \ihvtest1\src\studiorender
copy ..\..\studiorender\*.dsp \ihvtest1\src\studiorender
