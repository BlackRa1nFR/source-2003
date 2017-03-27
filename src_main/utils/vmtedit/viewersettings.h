//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           ViewerSettings.h
// last modified:  May 29 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
// version:        1.2
//
// email:          mete@swissquake.ch
// web:            http://www.swissquake.ch/chumbalum-soft/
//
#ifndef INCLUDED_VIEWERSETTINGS
#define INCLUDED_VIEWERSETTINGS


typedef struct
{
	// model 
	float rot[3];
	float trans[3];

	// fullscreen
	int width, height;
	bool cds;
} ViewerSettings;



extern ViewerSettings g_viewerSettings;



void InitViewerSettings (void);
bool LoadViewerSettings (const char *filename);
bool SaveViewerSettings (const char *filename);


#endif // INCLUDED_VIEWERSETTINGS