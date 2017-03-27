// view.h

#ifndef VIEW_H
#define VIEW_H
#pragma once

extern	ConVar		v_gamma;		// monitor gamma
extern	ConVar		v_texgamma;		// source gamma of textures
extern	ConVar		v_brightness;	// low level light adjustment
extern  ConVar		v_linearFrameBuffer; // gamma convert in hardware or in textures?

void V_Init (void);
void V_Shutdown( void );

#endif // VIEW_H
