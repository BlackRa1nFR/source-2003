// Get rid of:
//	warning C4049: compiler limit : terminating line number emission
#ifdef _WIN32
#pragma warning (disable:4049)
#endif
#define DEFINE_SHADERS
#include "tier0/dbg.h"
#include "materialsystem/ishader.h"
#include "shaderlib/shaderDLL.h"
#ifdef PS20A
#include "fxctmp9_nv3x\Refract_ps20.inc"
#else
#include "fxctmp9_nv3x_ps20\Refract_ps20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\Refract_vs20.inc"
#else
#include "fxctmp9_nv3x_ps20\Refract_vs20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\ShatteredGlass_ps20.inc"
#else
#include "fxctmp9_nv3x_ps20\ShatteredGlass_ps20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\ShatteredGlass_vs20.inc"
#else
#include "fxctmp9_nv3x_ps20\ShatteredGlass_vs20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\WaterCheap_ps20.inc"
#else
#include "fxctmp9_nv3x_ps20\WaterCheap_ps20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\WaterCheap_vs20.inc"
#else
#include "fxctmp9_nv3x_ps20\WaterCheap_vs20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\Water_ps20.inc"
#else
#include "fxctmp9_nv3x_ps20\Water_ps20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\Water_vs20.inc"
#else
#include "fxctmp9_nv3x_ps20\Water_vs20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\hsv_ps20.inc"
#else
#include "fxctmp9_nv3x_ps20\hsv_ps20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\lightmappedgeneric_ps20.inc"
#else
#include "fxctmp9_nv3x_ps20\lightmappedgeneric_ps20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\lightmappedgeneric_vs20.inc"
#else
#include "fxctmp9_nv3x_ps20\lightmappedgeneric_vs20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\screenspaceeffect_vs20.inc"
#else
#include "fxctmp9_nv3x_ps20\screenspaceeffect_vs20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\shadow_ps20.inc"
#else
#include "fxctmp9_nv3x_ps20\shadow_ps20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\shadow_vs20.inc"
#else
#include "fxctmp9_nv3x_ps20\shadow_vs20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\teeth_vs20.inc"
#else
#include "fxctmp9_nv3x_ps20\teeth_vs20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\vertexlit_and_unlit_generic_ps20.inc"
#else
#include "fxctmp9_nv3x_ps20\vertexlit_and_unlit_generic_ps20.inc"
#endif

#ifdef PS20A
#include "fxctmp9_nv3x\vertexlit_and_unlit_generic_vs20.inc"
#else
#include "fxctmp9_nv3x_ps20\vertexlit_and_unlit_generic_vs20.inc"
#endif

