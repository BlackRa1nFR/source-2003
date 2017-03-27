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

#include "vector.h"

enum // render modes
{
	RM_WIREFRAME,
	RM_FLATSHADED,
	RM_SMOOTHSHADED,
	RM_TEXTURED
};



typedef struct
{
	char	registrysubkey[ 64 ];
	int		application_mode;	// 0 expression, 1 choreo

	bool showHitBoxes;
	bool showBones;
	bool showAttachments;
	bool showPhysicsModel;
	bool showPhysicsPreview;
	bool showSequenceBoxes;
	bool enableIK;
	bool showNormals;

	// Current attachment we're editing. -1 if none.
	int m_iEditAttachment;

	int solveHeadTurn; // 0 == no, 1 == based on dt, 2 == completely.
	float flHeadTurn;
	Vector vecHeadTarget;
	Vector vecEyeTarget;

	bool showLightingCenter;
	int  highlightPhysicsBone;
	int  highlightHitbox;
	int  highlightBone;
	QAngle lightrot;	// light rotation
	float lColor[4];	// directional color
	float aColor[4];	// ambient color

	// external

	// model 
	QAngle rot;
	Vector trans;
	float  fov;		// horizontal field of view

	// render
	int renderMode;
	bool showBackground;
	bool showGround;
	bool showTexture;
	bool showMovement;
	int texture;
	int skin;

	// animation
	int sequence;
	int overlaySequence[4];
	float overlayWeight[4];
	float speedScale;
	bool blendSequenceChanges;

	// bodyparts and bonecontrollers
	int submodels[32];
	float controllers[8];

	// fullscreen
	int width, height;
	bool cds;

	// colors
	float bgColor[4];	// background color
	float gColor[4];

	// misc
	bool pause;
	bool rotating;
	bool mousedown;

	// only used for fullscreen mode
	char modelFile[256];
	//char backgroundTexFile[256];
	//char groundTexFile[256];

	int lod;
	bool autoLOD;
	bool softwareSkin;
	bool overbright;

	int	thumbnailsize;

	int	speechapiindex;
} ViewerSettings;

extern ViewerSettings g_viewerSettings;


void InitViewerSettings ( const char *subkey );
bool LoadViewerSettings (const char *filename);
bool SaveViewerSettings (const char *filename);

// For saving/loading "global" settings
bool LoadViewerSettingsInt( char const *keyname, int *value );
bool SaveViewerSettingsInt ( const char *keyname, int value );


#endif // INCLUDED_VIEWERSETTINGS