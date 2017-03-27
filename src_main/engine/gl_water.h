#ifndef GL_WATER_H
#define GL_WATER_H
#pragma once



// sky
extern void			R_AddSkySurface( int surfID );
extern void			R_ClearSkyBox (void);
extern void			R_DrawSkyBox (float zFar, int nDrawFlags = 0x3F );

#endif		// GL_WATER_H

