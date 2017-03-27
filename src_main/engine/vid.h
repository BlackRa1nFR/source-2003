// vid.h -- video driver defs
#ifndef VID_H
#define VID_H

#include "vmodes.h"

extern	viddef_t	vid;				// global video state

void VID_Init( void );
void VID_Shutdown( void );

// Screen shot functionality
void VID_WriteMovieFrame( void );
void VID_TakeSnapshot( const char *pFilename );

#endif //VIDH