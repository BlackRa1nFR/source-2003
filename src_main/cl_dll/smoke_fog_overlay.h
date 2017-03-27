
#ifndef SMOKE_FOG_OVERLAY_H
#define SMOKE_FOG_OVERLAY_H


#include "basetypes.h"
#include "vector.h"


void InitSmokeFogOverlay();
void TermSmokeFogOverlay();
void DrawSmokeFogOverlay();


// Set these before calling DrawSmokeFogOverlay.
extern float	g_SmokeFogOverlayAlpha;
extern Vector	g_SmokeFogOverlayColor;


#endif


