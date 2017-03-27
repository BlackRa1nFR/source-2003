// Get rid of:
//	warning C4049: compiler limit : terminating line number emission
#ifdef _WIN32
#pragma warning (disable:4049)
#endif
#define DEFINE_SHADERS
#include "tier0/dbg.h"
#include "materialsystem/ishader.h"
#include "shaderlib/shaderDLL.h"
#include "vshtmp9\BumpmappedLightmap.inc"
#include "pshtmp9\BumpmappedLightmap_LightingOnly_OverBright1.inc"
#include "pshtmp9\BumpmappedLightmap_LightingOnly_OverBright2.inc"
#include "vshtmp9\DebugTangentSpace.inc"
#include "vshtmp9\LightingOnly.inc"
#include "vshtmp9\LightmappedGeneric_LightingOnly.inc"
#include "pshtmp9\LightmappedGeneric_LightingOnly_Overbright1.inc"
#include "pshtmp9\LightmappedGeneric_LightingOnly_Overbright2.inc"
#include "vshtmp9\UnlitGeneric_LightingOnly.inc"
#include "pshtmp9\UnlitGeneric_NoTexture.inc"
#include "pshtmp9\VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright1.inc"
#include "pshtmp9\VertexLitGeneric_DiffBumpLightingOnly_TwentyPercent_Overbright2.inc"
#include "vshtmp9\VertexLitGeneric_DiffBumpTimesBase.inc"
#include "fxctmp9\VertexLit_and_unlit_Generic_vs20.inc"
#include "pshtmp9\fillrate.inc"
#include "vshtmp9\fillrate.inc"
#include "pshtmp9\unlitgeneric.inc"
#include "vshtmp9\unlitgeneric.inc"
#include "fxctmp9\vertexlit_lighting_only_ps20.inc"
#include "pshtmp9\vertexlitgeneric_lightingonly_overbright1.inc"
#include "pshtmp9\vertexlitgeneric_lightingonly_overbright2.inc"
