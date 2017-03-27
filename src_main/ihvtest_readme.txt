-Make sure you have the processor pack/msdev sp4 installed.
-type "buildall" to build everything
-type "run" to run.

You will get higher performance if you disable some of the timing code.  
To do this, edit src/common/MaterialSystem/ShaderDX8/CMaterialSystemStats.h and change:

#ifdef IHVTEST
#define MEASURE_STATS 1
#endif

to:

#ifdef IHVTEST
//#define MEASURE_STATS 1
#endif

You can select all of our possible lighting configurations by using the "-light" command line option
as in run.bat.  Here are what the N in "-light N" means:

N	light 1		light 2
-------------------------------
0	disable		disable
1 	spot		disable
2	point		disable
3 	directional	disable
4	spot		spot
5	spot		point
6	spot		directional
7	point		point
8	point		directional
9	directional	directional

If you are on a dx8 card and want to force the dx7 path, do:

-dx 7

on the command line.

Please send any feedback to gary@valvesoftware.com .

