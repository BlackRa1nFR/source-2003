
@echo off
rem Augments srcbackup.bat and backs up the extra files necessary to
rem fully build the source.

pkzip25 -add -max -dir=full -rec %1 ivp/ivp_library/ivp_physics.lib ivp/ivp_library/ivp_compactbuilder.lib ivp/ivp_library/hk_physics.lib ivp/havana/hk_library/win32/release/hk_base.lib ivp/havana/hk_library/win32/release/hk_math.lib engine/gas2masm/debug/gas2masm.exe "devtools/devstudio macros/comments.dsm" dx8sdk/lib/d3derr8.lib dx8sdk/lib/ddraw.lib dx8sdk/lib/dxguid.lib dx8sdk/lib/dsound.lib dx8sdk/lib/d3d8.lib dx8sdk/lib/d3dx8.lib public/s3tc.lib *.hxx utils/mxtk/lib/mxtk.lib utils/mxtk/lib/mxtkd.lib worldcraft/*.cur worldcraft/res/*.bmp devtools/bin/ml.exe common/wonauth.lib common/woncrypt.lib engine/a3d/verifya3d.lib engine/a3d/a3dwrapper.lib utils/crypt/lib/win32_vc6/crypt.lib %3 %4 %5 %6

