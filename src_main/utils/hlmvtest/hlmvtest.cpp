// particle.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <assert.h>
#include <ctype.h>
#include <conio.h>
#include <stdarg.h>
#include "matsysapp.h"
#include "vmatrix.h"
//#include "counter.h"
//#include "particlemgr.h"
//#include "particle_prototype.h"
//#include "helpers.h"
//#include "tweakpropdlg.h"

#include "ViewerSettings.h"
ViewerSettings g_viewerSettings;

/*
class PrototypeArgAccessImpl : public IPrototypeArgAccess
{
public:
	virtual const char*	FindArg(const char *pName, const char *pDefault)
	{
		const char *ret = g_MaterialSystemApp.FindParameterArg(pName);
		if(ret)
			return ret;
		else
			return pDefault;
	}		
};
*/

extern "C" char g_szAppName[];
extern "C" void AppInit( void );
extern "C" void AppRender( float frametime, float mouseDeltaX, float mouseDeltaY );
extern "C" void AppExit( void );
extern "C" void AppKey( int key, int down );
extern "C" void AppChar( int key );
bool g_bCaptureOnFocus=false;


#define MAX_PARTICLE_LIGHTS		32


IMaterial		*g_pAxesMaterial = NULL;
IMaterial		*g_pBackgroundMaterial = NULL;
IMaterialSystem	*g_pMaterialSystem = NULL;

// ITweakPropDlg	*g_pTweakPropDlg = NULL;

bool			g_bDirectionalLight = true;
int				g_nDrawn = 0;
char			g_szAppName[] = "Particle Test";

// IPrototypeAppEffect *g_pCurEffect = NULL;

bool			g_bInfiniteLight = false;
bool			g_bDrawCoordinateSystem = true;
Vector			g_IdentityBasis[3] = {Vector(1.0f, 0.0f, 0.0f), Vector(0.0f, 1.0f, 0.0f), Vector(0.0f, 0.0f, 1.0f)};
float			g_TimeScale = 1.0f;
bool			g_bDistance; // Adjusting distance?
Vector			g_ViewerPos;
Vector			g_ViewerBasis[3];
VMatrix			g_mModelView;
VMatrix			g_mIdentity;
bool			g_bDirLightController = false;
unsigned long	g_SleepMS = 0;


unsigned long totalms=0;
unsigned long totalframes=0;



class PosController
{
public:
	float	m_Distance;
	float	m_XYAngle;
	float	m_ZAngle;
};

PosController	g_ViewController;
PosController	g_DirLightController;

#include "StudioModel.h"

static StudioModel tempmodel;

void InitStudioModel()
{
	tempmodel.LoadModel( "u:/hl2/hl2/models/assassin.mdl" );
	tempmodel.SetSequence( 0 );

	tempmodel.SetController( 0, 0.0 );
	tempmodel.SetController( 1, 0.0 );
	tempmodel.SetController( 2, 0.0 );
	tempmodel.SetController( 3, 0.0 );
	tempmodel.SetMouth( 0 );
}


void TermStudioModel()
{
	// g_StudioModel.Term();

	// delete g_pCurEffect;
	// g_pCurEffect = NULL;
}




void AppInit( void )
{
	// Bind the materials we'll be using.
	g_pMaterialSystem = g_MaterialSystemApp.m_pMaterialSystem;
	g_pAxesMaterial = g_pMaterialSystem->FindMaterial("particle/particleapp_axes", NULL, false);	
	g_pBackgroundMaterial = g_pMaterialSystem->FindMaterial("particle/particleapp_background", NULL, false);

	// Initialize the particle manager.
	InitStudioModel();

	// Viewer info.
	g_ViewController.m_Distance = (float)g_MaterialSystemApp.FindNumParameter("-StartDist", 150);
	g_ViewController.m_XYAngle = 180.0f;
	g_ViewController.m_ZAngle = 13.0f;

	g_DirLightController.m_Distance = 50.0f;
	g_DirLightController.m_XYAngle = 0.0f;
	g_DirLightController.m_ZAngle = 0.0f;

	g_mIdentity.Identity();
	g_bDistance = false;

	g_SleepMS = (unsigned long)g_MaterialSystemApp.FindNumParameter("-SleepMS", 0);

#if 0
	if(!RestartKeyframer())
	{
		char msg[4096];
		sprintf(msg, "Error: first parameter must be a particle system name:\n");

		for(PrototypeEffectLink *pCur=g_pPrototypeEffects; pCur; pCur=pCur->m_pNext)
		{
			if(pCur != g_pPrototypeEffects)
				strcat(msg, ", ");

			strcat(msg, pCur->m_pEffectName);
		}

		Sys_Error("%s", msg);
	}
#endif

	// g_MaterialSystemApp.MakeWindowTopmost();
}


void AppExit( void )
{	
#if 0
	TermStudioModel();

	if(g_pTweakPropDlg)
	{
		g_pTweakPropDlg->Release();
		g_pTweakPropDlg = NULL;
	}
#endif
}


inline float FClamp(float val, float min, float max)
{
	return (val < min) ? min : (val > max ? max : val);
}

inline float SinDegrees( float angle )
{
	return sin( angle * 3.151892653 / 180.0 );
}

inline float CosDegrees( float angle )
{
	return cos( angle * 3.151892653 / 180.0 );
}

// zAngle's range is [-90,90].
// When zAngle is 0, the position is in the middle of the sphere (vertically).
// When zAngle is  90, the position is at the top of the sphere.
// When zAngle is -90, the position is at the bottom of the sphere.
Vector CalcSphereVecAngles(float xyAngle, float zAngle, float fRadius)
{
	Vector vec;

	vec.x = CosDegrees(xyAngle) * CosDegrees(zAngle);
	vec.y = SinDegrees(xyAngle) * CosDegrees(zAngle);
	vec.z = SinDegrees(zAngle);

	return vec * fRadius;
}


// Figure out the rotation to look from vEye to vDest.
void SetupLookAt(
	const Vector &vEye,
	const Vector &vDest,
	Vector basis[3])
{
	basis[0] = (vDest - vEye).Normalize();	// Forward.
	basis[2].Init(0.0f, 0.0f, 1.0f);		// Up.
	basis[1] = basis[2].Cross(basis[0]).Normalize();	// Left.
	basis[2] = basis[0].Cross(basis[1]).Normalize();	// Regenerate up.
}


void UpdateView(float mouseDeltaX, float mouseDeltaY)
{
	VMatrix mRot;
	PosController *pController;


	if(g_bDirLightController)
		pController = &g_DirLightController;
	else
		pController = &g_ViewController;


	if(g_bDistance)
	{
		static float fDistScale = 0.5f;
		pController->m_Distance += mouseDeltaY * fDistScale;
		if(pController->m_Distance < 10.0f)
			pController->m_Distance = 10.0f;
	}
	else
	{
		static float fAngleScale = 0.8f;
		pController->m_XYAngle += mouseDeltaX * fAngleScale;
		if(pController->m_XYAngle < 0.0f)
			pController->m_XYAngle = 360.0f - (float)fmod(pController->m_XYAngle, 360.0f);
		else
			pController->m_XYAngle = (float)fmod(pController->m_XYAngle, 360.0f);
		
		pController->m_ZAngle += mouseDeltaY * fAngleScale;
		pController->m_ZAngle = FClamp(pController->m_ZAngle, -89, 89);
	}


	g_pMaterialSystem->MatrixMode(MATERIAL_MODELVIEW);

	// This matrix converts from GL coordinates (X=right, Y=up, Z=towards viewer)
	// to VEngine coordinates (X=forward, Y=left, Z=up).
	VMatrix mGLToVEngine(
		0.0f,  0.0f,  -1.0f, 0.0f,
		-1.0f, 0.0f,  0.0f,  0.0f,
		0.0f,  1.0f,  0.0f,  0.0f,
		0.0f,  0.0f,  0.0f,  1.0f);

	VMatrix mVEngineToGL = mGLToVEngine.Transpose();

	g_ViewerPos = CalcSphereVecAngles(g_ViewController.m_XYAngle, g_ViewController.m_ZAngle, g_ViewController.m_Distance);

	SetupLookAt(
		g_ViewerPos,
		Vector(0,0,0),
		g_ViewerBasis);

	mRot = SetupMatrixIdentity();
	mRot.SetBasisVectors(g_ViewerBasis[0], g_ViewerBasis[1], g_ViewerBasis[2]);
	g_mModelView = mVEngineToGL * mRot.Transpose3x3() * SetupMatrixTranslation(-g_ViewerPos);

	g_pMaterialSystem->LoadMatrix((float*)g_mModelView.Transpose().m);
}


void gln_DrawLine(
	const Vector &vFrom,
	const Vector &vTo,
	float r, 
	float g, 
	float b, 
	float a)
{
	float linePositions[2][4] = {{vFrom.x, vFrom.y, vFrom.z, 1}, {vTo.x, vTo.y, vTo.z, 1}};
	float color[2][4]   = {{r,g,b,a}, {r,g,b,a}};
	unsigned short indices[2] = {0,1};
	
	g_pMaterialSystem->PositionPointer((float*)linePositions);
	g_pMaterialSystem->ColorPointer((float*)color);
	g_pMaterialSystem->IndexPointer(indices);
	g_pMaterialSystem->DrawElements(MATERIAL_LINES, 2, 2, false);
}


void DrawCoordinateSystem(
	const Vector &vOrigin,
	const Vector basis[3],
	const float dist)
{
	g_pMaterialSystem->Bind(g_pAxesMaterial);

	gln_DrawLine(vOrigin, vOrigin + basis[0]*dist, 1.0f, 0.0f, 0.0f, 1.0f);
	gln_DrawLine(vOrigin, vOrigin + basis[1]*dist, 0.0f, 1.0f, 0.0f, 1.0f);
	gln_DrawLine(vOrigin, vOrigin + basis[2]*dist, 0.0f, 0.0f, 1.0f, 1.0f);
}


void DrawBackground()
{
	VMatrix mTemp;
	g_pMaterialSystem->Bind(g_pBackgroundMaterial);
	g_pMaterialSystem->MatrixMode(MATERIAL_MODELVIEW);
	
	g_pMaterialSystem->GetMatrix(MATERIAL_MODELVIEW, (float*)mTemp.m);
		g_pMaterialSystem->LoadIdentity();

		float dist=-15000.0f;
		float tMin=0, tMax=1;
		float positions[4][4] = {{-dist,dist,dist,1}, {dist,dist,dist,1}, {dist,-dist,dist,1}, {-dist,-dist,dist,1}};
		float texCoords[4][2] = {{tMin,tMax}, {tMax,tMax}, {tMax,tMin}, {tMin,tMin}};
		float colors[4][4]    = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
		unsigned short indices[4] = {0,1,2,3};

		g_pMaterialSystem->PositionPointer((float*)positions);
		g_pMaterialSystem->ColorPointer((float*)colors);
		g_pMaterialSystem->TexCoordPointer((float*)texCoords);
		g_pMaterialSystem->IndexPointer(indices);
		g_pMaterialSystem->DrawElements(MATERIAL_QUADS, 4, 4, false);
	g_pMaterialSystem->LoadMatrix((float*)mTemp.m);
}


static unsigned long minTime, maxTime, totalTime;
static unsigned long nFrames;

void ResetTimers()
{
	minTime = 100000;
	maxTime = 0;
	totalTime = 0;
	nFrames = 0;
}


void AppRender( float frametime, float mouseDeltaX, float mouseDeltaY )
{
	// Sleep a certain amount (used to test how euler step-based physics act at low framerates).
	MSA_Sleep(g_SleepMS);

	UpdateView(mouseDeltaX, mouseDeltaY);

	DrawBackground();

	if(g_bDrawCoordinateSystem)
	{
		DrawCoordinateSystem(
			Vector(0.0f, 0.0f, 0.0f),
			g_IdentityBasis,
			100.0f);
	}


	// Counter cnt(CSTART_MICRO);

	// SimulateInfo info;
	// static SimulateInfo lastInfo = {0,0};
	// g_StudioModel.SimulateAndRender(info, frametime * g_TimeScale);

	tempmodel.DrawModel( );


	// Wait a few frames before starting the timing.
	++nFrames;
	if(nFrames == 5)
	{
		ResetTimers();
		nFrames = 6;
	}
	else
	{
		++nFrames;
	}

	// unsigned long curTime = cnt.EndMicro();
	// minTime = min(curTime, minTime);
	// maxTime = max(curTime, maxTime);
	// totalTime += curTime;

	/*
	g_MaterialSystemApp.SetTitleText("nMaterials: %d, nParticles: %d, time: (min=%d, max=%d, avg=%d), microsec/particle: %.2f", 
		info.m_nMaterials, 
		info.m_nParticles, 
		minTime,
		maxTime,
		totalTime / nFrames,
		(double)(totalTime / nFrames) / info.m_nParticles
		);
	*/
}


void AppKey( int key, int down )
{
	if(toupper(key) == 'D')
		g_bDistance = !!down;


	if(toupper(key) == 'M')
		g_bDirLightController = !!down;

	if(toupper(key) == 'T')
	{
		ResetTimers();
	}

	if(key == 107)
	{
		g_TimeScale += 0.05f;
		if(g_TimeScale > 20.0f)
			g_TimeScale = 20.0f;
	}
	else if(key == 109)
	{
		g_TimeScale -= 0.05f;
		if(g_TimeScale < 0.0f)
			g_TimeScale = 0.0f;
	}

	if(down && toupper(key) == 'C')
	{
		g_bDrawCoordinateSystem = !g_bDrawCoordinateSystem;
	}

	if(down && toupper(key) == 'R')
	{
		// RestartKeyframer();
		ResetTimers();
	}

	if(down && toupper(key) == 'L')
	{
		g_bDirectionalLight = !g_bDirectionalLight;
	}

	if(down && toupper(key) == 'I')
	{
		g_bInfiniteLight = !g_bInfiniteLight;
	}

	if(down && toupper(key) == 'E')
	{
		g_MaterialSystemApp.MouseRelease();
		g_MaterialSystemApp.m_bPaused = true;
	}
}


void AppChar( int key )
{
}

