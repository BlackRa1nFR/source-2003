// particle.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <assert.h>
#include <conio.h>
#include <stdarg.h>
#include "matsysapp.h"
#include "vmatrix.h"
#include "counter.h"
#include "particlemgr.h"
#include "particle_prototype.h"
#include "helpers.h"
#include "tweakpropdlg.h"
#include "lowpassstream.h"
#include "imesh.h"


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


extern "C" char g_szAppName[];
extern "C" void AppInit( void );
extern "C" void AppRender( float frametime, float mouseDeltaX, float mouseDeltaY );
extern "C" void AppExit( void );
extern "C" void AppKey( int key, int down );
extern "C" void AppChar( int key );
bool g_bCaptureOnFocus=false;


#define MAX_PARTICLE_LIGHTS		32


CLowPassStream<double, 8>	g_FrameTimeFilter;
CLowPassStream<double, 8>	g_MicrosecPerParticleFilter;
IMaterial		*g_pAxesMaterial = NULL;
IMaterial		*g_pBackgroundMaterial = NULL;
IMaterialSystem	*g_pMaterialSystem = NULL;
IMaterialSystem	*materials = NULL; // all the effects reference this.
float			g_ParticleAppFrameTime = 0;

ITweakPropDlg	*g_pTweakPropDlg = NULL;

bool			g_bDirectionalLight = true;
int				g_nDrawn = 0;
char			g_szAppName[] = "Particle Test";

IPrototypeAppEffect *g_pCurEffect = NULL;

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
	Vector	m_vPos;
	Vector	m_vAngles;
};

PosController	g_ViewController;
PosController	g_DirLightController;
Vector			g_DirLightPos;


void InitParticleMgr()
{
	g_ParticleMgr.Init(8192*6, g_pMaterialSystem);
}


void TermParticleMgr()
{
	g_ParticleMgr.Term();
}


bool RestartKeyframer()
{
	delete g_pCurEffect;
	g_pCurEffect = NULL;
	
	if(__argc >= 2)
	{
		char *pEffectName = __argv[1];
		for(PrototypeEffectLink *pCur=g_pPrototypeEffects; pCur; pCur=pCur->m_pNext)
		{
			if(stricmp(pEffectName, pCur->m_pEffectName) == 0)
			{
				if(g_pCurEffect = pCur->m_CreateFn())
				{
					RecvTable *pTable;
					void *pObj;
					if(g_pCurEffect->GetPropEditInfo(&pTable, &pObj))
					{
						if(g_pTweakPropDlg)
						{
							g_pTweakPropDlg->SetObj(pObj);
						}
						else
						{
							g_pTweakPropDlg = CreateTweakPropDlg(
								pTable,
								pObj,
								"Properties",
								g_MaterialSystemApp.m_hWnd,
								g_MaterialSystemApp.m_hInstance);
						}
					}
										
					PrototypeArgAccessImpl prototypeArgAccessImpl;
					g_pCurEffect->Start(&g_ParticleMgr, &prototypeArgAccessImpl);
					return true;
				}
			}
		}
	}

	return false;
}


void AppInit( void )
{
	// Bind the materials we'll be using.
	g_pMaterialSystem = materials = g_MaterialSystemApp.m_pMaterialSystem;
	g_pAxesMaterial = g_pMaterialSystem->FindMaterial("particle/particleapp_axes", NULL, false);	
	g_pBackgroundMaterial = g_pMaterialSystem->FindMaterial("particle/particleapp_background", NULL, false);

	// Initialize the particle manager.
	InitParticleMgr();

	// Viewer info.
	g_ViewController.m_vPos.Init( -100, 0, 0 );
	g_ViewController.m_vAngles.Init( 0, 0, 0 );

	g_DirLightController.m_vPos.Init( -50, 0, 0 );

	g_mIdentity.Identity();
	g_bDistance = false;

	g_SleepMS = (unsigned long)g_MaterialSystemApp.FindNumParameter("-SleepMS", 0);

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

	//g_MaterialSystemApp.MakeWindowTopmost();
}


void AppExit( void )
{	
	TermParticleMgr();

	if(g_pTweakPropDlg)
	{
		g_pTweakPropDlg->Release();
		g_pTweakPropDlg = NULL;
	}
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
	basis[0] = vDest - vEye;
	VectorNormalize( basis[0] );
	basis[2].Init(0.0f, 0.0f, 1.0f);		// Up.
	
	basis[1] = basis[2].Cross(basis[0]);
	VectorNormalize( basis[1] );
	
	basis[2] = basis[0].Cross(basis[1]);
	VectorNormalize( basis[2] );
}


void UpdateView(float mouseDeltaX, float mouseDeltaY)
{
	VMatrix mRot;
	PosController *pController;


	if(g_bDirLightController)
		pController = &g_DirLightController;
	else
		pController = &g_ViewController;

	Vector vForward, vUp, vRight;
	AngleVectors( pController->m_vAngles, &vForward, &vRight, &vUp );

	static float fAngleScale = 0.4f;
	static float fDistScale = 0.5f;

	if( MSA_IsMouseButtonDown( MSA_BUTTON_LEFT ) )
	{
		if( MSA_IsMouseButtonDown( MSA_BUTTON_RIGHT ) )
		{
			// Ok, move forward and backwards.
			pController->m_vPos += vForward * mouseDeltaY * fDistScale;
			pController->m_vPos += vRight * mouseDeltaX * fDistScale;
		}
		else
		{
			pController->m_vAngles.y += -mouseDeltaX * fAngleScale;
			pController->m_vAngles.x += -mouseDeltaY * fAngleScale;
		}
	}
	else if( MSA_IsMouseButtonDown( MSA_BUTTON_RIGHT ) )
	{
		pController->m_vPos += vUp * mouseDeltaY * fDistScale;
		pController->m_vPos += vRight * mouseDeltaX * fDistScale;
	}

	g_DirLightPos = g_DirLightController.m_vPos;


	g_pMaterialSystem->MatrixMode(MATERIAL_VIEW);
	g_ViewerPos = g_ViewController.m_vPos;

	// This matrix converts from GL coordinates (X=right, Y=up, Z=towards viewer)
	// to VEngine coordinates (X=forward, Y=left, Z=up).
	VMatrix mGLToVEngine(
		0.0f,  0.0f,  -1.0f, 0.0f,
		-1.0f, 0.0f,  0.0f,  0.0f,
		0.0f,  1.0f,  0.0f,  0.0f,
		0.0f,  0.0f,  0.0f,  1.0f);

	VMatrix mVEngineToGL = mGLToVEngine.Transpose();

	mRot = SetupMatrixAngles( g_ViewController.m_vAngles );
	mRot.GetBasisVectors( g_ViewerBasis[0], g_ViewerBasis[1], g_ViewerBasis[2] );

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
	Vector linePositions[2] = {Vector(vFrom.x, vFrom.y, vFrom.z), Vector(vTo.x, vTo.y, vTo.z)};
	float color[2][4]   = {{r,g,b,a}, {r,g,b,a}};
	unsigned short indices[2] = {0,1};

	CMeshBuilder builder;
	builder.Begin( 
		g_pMaterialSystem->GetDynamicMesh( false ),
		MATERIAL_LINES,
		2 );

	for( int i=0; i < 2; i++ )
	{
		builder.Position3fv( (float*)&linePositions[i] );
		builder.Color3fv( color[i] );
		builder.AdvanceVertex();
	}

	builder.End( false, true );
}


void DrawDirLight()
{
	Vector vColor(1,1,1);
	Vector vPos = g_DirLightPos;
	float size = 1;

	Vector pos[4] = 
	{
		vPos + g_ViewerBasis[1]*size - g_ViewerBasis[2]*size,
		vPos + g_ViewerBasis[1]*size + g_ViewerBasis[2]*size,
		vPos - g_ViewerBasis[1]*size + g_ViewerBasis[2]*size,
		vPos - g_ViewerBasis[1]*size - g_ViewerBasis[2]*size
	};

	g_pMaterialSystem->Bind(g_pAxesMaterial);

	CMeshBuilder builder;
	builder.Begin( 
		g_pMaterialSystem->GetDynamicMesh( false ),
		MATERIAL_QUADS,
		1 );

	for( int i=0; i < 4; i++ )
	{
		builder.Position3f( VectorExpand(pos[i]) );
		builder.Color4f( VectorExpand(vColor), 1 );
		builder.AdvanceVertex();
	}

	builder.End( false, true );
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
	VMatrix mTemp, mTempModel;
	
	g_pMaterialSystem->GetMatrix(MATERIAL_MODEL, (float*)mTempModel.m);
	g_pMaterialSystem->GetMatrix(MATERIAL_VIEW, (float*)mTemp.m);

	g_pMaterialSystem->MatrixMode(MATERIAL_VIEW);
	g_pMaterialSystem->LoadIdentity();

	g_pMaterialSystem->MatrixMode(MATERIAL_MODEL);
	g_pMaterialSystem->LoadIdentity();

		float dist=-15000.0f;
		float tMin=0, tMax=1;
		Vector positions[4] = {Vector(-dist,dist,dist), Vector(dist,dist,dist), Vector(dist,-dist,dist), Vector(-dist,-dist,dist)};
		float texCoords[4][2] = {{tMin,tMax}, {tMax,tMax}, {tMax,tMin}, {tMin,tMin}};
		float colors[4][4]    = {{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1}};
		unsigned short indices[4] = {0,1,2,3};

		g_pMaterialSystem->Bind(g_pBackgroundMaterial);

		IMesh *pMesh = g_pMaterialSystem->GetDynamicMesh( false );
		CMeshBuilder builder;
		builder.Begin( pMesh, MATERIAL_QUADS, 4 );
		for( int i=0; i < 4; i++ )
		{
			builder.Position3fv( (float*)&positions[i] );
			builder.Color3fv( colors[i] );
			builder.TexCoord2fv( 0, texCoords[i] );
			builder.AdvanceVertex();
		}
		builder.End( false, true );

	g_pMaterialSystem->MatrixMode(MATERIAL_VIEW);
	g_pMaterialSystem->LoadMatrix((float*)mTemp.m);

	g_pMaterialSystem->MatrixMode(MATERIAL_MODEL);
	g_pMaterialSystem->LoadMatrix((float*)mTempModel.m);
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
	g_bDistance = MSA_IsMouseButtonDown( MSA_BUTTON_RIGHT );

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


	g_ParticleAppFrameTime = frametime * g_TimeScale;

	SimulateInfo info;
	static SimulateInfo lastInfo = {0,0};

	g_ParticleMgr.Update(info, frametime * g_TimeScale);
	g_ParticleMgr.SimulateUndrawnEffects();

	DrawDirLight();

	g_MicrosecPerParticleFilter.AddSample( (double)frametime * 1000000.0 / info.m_nParticles );
	g_FrameTimeFilter.AddSample( frametime * 1000.0f );

	g_MaterialSystemApp.SetTitleText(
		"nParticles: %d, "
		"frametime: %.2fms, "
		"microsec/particle: %.2f", 
		info.m_nParticles, 
		g_FrameTimeFilter.GetCurrentAverage(),
		g_MicrosecPerParticleFilter.GetCurrentAverage()
		);
}


void AppKey( int key, int down )
{
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
		RestartKeyframer();
		ResetTimers();
	}

	if(down && toupper(key) == 'L')
	{
		g_bDirectionalLight = !g_bDirectionalLight;
	}
}


void AppChar( int key )
{
}

