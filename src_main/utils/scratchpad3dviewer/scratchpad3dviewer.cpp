//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "d3dapp.h"
#include "d3dx8math.h"
#include "mathlib.h"
#include "ScratchPad3D.h"
#include "vstdlib/strtools.h"


class VertPosDiffuse
{
public:
	inline void		Init(Vector const &vPos, float r, float g, float b, float a)
	{
		m_Pos[0] = vPos.x;
		m_Pos[1] = vPos.y;
		m_Pos[2] = vPos.z;
		SetDiffuse( r, g, b, a );
	}

	inline void		SetDiffuse( float r, float g, float b, float a )
	{
		m_Diffuse[0] = (unsigned char)(b * 255.9f);
		m_Diffuse[1] = (unsigned char)(g * 255.9f);
		m_Diffuse[2] = (unsigned char)(r * 255.9f);
		m_Diffuse[3] = (unsigned char)(a * 255.9f);
	}

	inline void		SetDiffuse( Vector const &vColor )
	{
		SetDiffuse( vColor.x, vColor.y, vColor.z, 1 );
	}

	static inline DWORD	GetFVF()	{return D3DFVF_DIFFUSE | D3DFVF_XYZ;}

	float			m_Pos[3];
	unsigned char	m_Diffuse[4];
};

class PosController
{
public:
	Vector	m_vPos;
	QAngle	m_vAngles;
};

int g_nLines, g_nPolygons;

PosController	g_ViewController;
Vector			g_IdentityBasis[3] = {Vector(1,0,0), Vector(0,1,0), Vector(0,0,1)};
Vector			g_ViewerPos;
Vector			g_ViewerBasis[3];
VMatrix			g_mModelView;
CScratchPad3D	*g_pScratchPad = NULL;
FILETIME		g_LastWriteTime;
char			g_Filename[256];


// ------------------------------------------------------------------------------------------ //
// Helper functions.
// ------------------------------------------------------------------------------------------ //

inline float FClamp(float val, float min, float max)
{
	return (val < min) ? min : (val > max ? max : val);
}

inline float FMin(float val1, float val2)
{
	return (val1 < val2) ? val1 : val2;
}

inline float FMax(float val1, float val2)
{
	return (val1 > val2) ? val1 : val2;
}

inline float CosDegrees(float angle)
{
	return (float)cos(DEG2RAD(angle));
}

inline float SinDegrees(float angle)
{
	return (float)sin(DEG2RAD(angle));
}

inline float FRand(float a, float b)
{
	return a + (b - a) * ((float)rand() / RAND_MAX);
}

void DrawLine2(const Vector &vFrom, const Vector &vTo, float r1, float g1, float b1, float a1, float r2, float g2, float b2, float a2)
{	
	VertPosDiffuse verts[2];
	
	verts[0].Init( vFrom, r1, g1, b1, 1 );
	verts[1].Init( vTo,   r2, g2, b2, 1 );

	HRESULT hr;
	hr = g_pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	hr = g_pDevice->SetTexture( 0, NULL ); 
	hr = g_pDevice->SetVertexShader( VertPosDiffuse::GetFVF() );
	hr = g_pDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, verts, sizeof(verts[0]) );

	++g_nLines;
}


void DrawLine(const Vector &vFrom, const Vector &vTo, float r, float g, float b, float a)
{
	DrawLine2(vFrom, vTo, r,g,b,a, r,g,b,a);
}


void DrawTri( Vector const &A, Vector const &B, Vector const &C, Vector const &vColor )
{
	VertPosDiffuse verts[3];

	verts[0].Init( A, vColor.x, vColor.y, vColor.z, 1 );
	verts[1].Init( B, vColor.x, vColor.y, vColor.z, 1 );
	verts[2].Init( C, vColor.x, vColor.y, vColor.z, 1 );

	HRESULT hr;
	hr = g_pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	hr = g_pDevice->SetTexture( 0, NULL ); 
	hr = g_pDevice->SetVertexShader( D3DFVF_DIFFUSE | D3DFVF_XYZ );
	hr = g_pDevice->DrawPrimitiveUP( D3DPT_TRIANGLELIST, 1, verts, sizeof(verts[0]) );

	++g_nPolygons;
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
void SetupLookAt( const Vector &vEye, const Vector &vDest, Vector basis[3] )
{
	basis[0] = (vDest - vEye);	// Forward.
	VectorNormalize( basis[0] );
	basis[2].Init(0.0f, 0.0f, 1.0f);		// Up.
	
	basis[1] = basis[2].Cross(basis[0]);	// Left.
	VectorNormalize( basis[1] );
	
	basis[2] = basis[0].Cross(basis[1]);	// Regenerate up.
	VectorNormalize( basis[2] );
}


D3DMATRIX* VEngineToTempD3DMatrix( VMatrix const &mat )
{
	static VMatrix ret;
	ret = mat.Transpose();
	return (D3DMATRIX*)&ret;
}


void UpdateView(float mouseDeltaX, float mouseDeltaY)
{
	VMatrix mRot;
	PosController *pController;


	pController = &g_ViewController;

	// WorldCraft-like interface..
	if( Sys_HasFocus() )
	{
		Vector vForward, vUp, vRight;
		AngleVectors( pController->m_vAngles, &vForward, &vRight, &vUp );

		static float fAngleScale = 0.4f;
		static float fDistScale = 0.5f;

		if( Sys_GetKeyState( APPKEY_LBUTTON ) )
		{
			if( Sys_GetKeyState( APPKEY_RBUTTON ) )
			{
				// Ok, move forward and backwards.
				pController->m_vPos += vForward * -mouseDeltaY * fDistScale;
				pController->m_vPos += vRight * mouseDeltaX * fDistScale;
			}
			else
			{
				pController->m_vAngles.y += -mouseDeltaX * fAngleScale;
				pController->m_vAngles.x += mouseDeltaY * fAngleScale;
			}
		}
		else if( Sys_GetKeyState( APPKEY_RBUTTON ) )
		{
			pController->m_vPos += vUp * -mouseDeltaY * fDistScale;
			pController->m_vPos += vRight * mouseDeltaX * fDistScale;
		}
	}

    // Set the projection matrix to 90 degrees.
	D3DXMATRIX matProj;
     D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/2, Sys_ScreenWidth() / (float)Sys_ScreenHeight(), 1.0f, 10000.0f );
    g_pDevice->SetTransform( D3DTS_PROJECTION, &matProj );


	// This matrix converts from D3D coordinates (X=right, Y=up, Z=forward)
	// to VEngine coordinates (X=forward, Y=left, Z=up).
	VMatrix mD3DToVEngine(
		0.0f,  0.0f,  1.0f, 0.0f,
		-1.0f, 0.0f,  0.0f,  0.0f,
		0.0f,  1.0f,  0.0f,  0.0f,
		0.0f,  0.0f,  0.0f,  1.0f);
	
	g_ViewerPos = pController->m_vPos;
	mRot = SetupMatrixAngles( pController->m_vAngles );

	g_mModelView = ~mD3DToVEngine * mRot.Transpose3x3() * SetupMatrixTranslation(-g_ViewerPos);

	HRESULT hr;
	hr = g_pDevice->SetTransform( D3DTS_VIEW, VEngineToTempD3DMatrix(g_mModelView) );

	// World matrix is identity..
	VMatrix mIdentity = SetupMatrixIdentity();
	hr = g_pDevice->SetTransform( D3DTS_WORLD, (D3DMATRIX*)&mIdentity );
}


// ------------------------------------------------------------------------------------------ //
// ScratchPad3D command implementation.
// ------------------------------------------------------------------------------------------ //

void CommandRender_Point( CScratchPad3D::CBaseCommand *pInCmd, IDirect3DDevice8 *pDevice )
{
	CScratchPad3D::CCommand_Point *pCmd = (CScratchPad3D::CCommand_Point*)pInCmd;
	
	g_pDevice->SetRenderState( D3DRS_POINTSIZE, *((DWORD*)&pCmd->m_flPointSize) );

	VertPosDiffuse vert;
	vert.Init( pCmd->m_Vert.m_vPos, VectorExpand(pCmd->m_Vert.m_vColor), 1 );

	g_pDevice->DrawPrimitiveUP( D3DPT_POINTLIST, 1, &vert, sizeof(vert) );
}

void CommandRender_Line( CScratchPad3D::CBaseCommand *pInCmd, IDirect3DDevice8 *pDevice )
{
	CScratchPad3D::CCommand_Line *pCmd = (CScratchPad3D::CCommand_Line*)pInCmd;
	DrawLine2( 
		pCmd->m_Verts[0].m_vPos, 
		pCmd->m_Verts[1].m_vPos,
		pCmd->m_Verts[0].m_vColor.x, pCmd->m_Verts[0].m_vColor.y, pCmd->m_Verts[0].m_vColor.z, 1,
		pCmd->m_Verts[1].m_vColor.x, pCmd->m_Verts[1].m_vColor.y, pCmd->m_Verts[1].m_vColor.z, 1 );
}

void CommandRender_Polygon( CScratchPad3D::CBaseCommand *pInCmd, IDirect3DDevice8 *pDevice )
{
	VertPosDiffuse verts[65];
	CScratchPad3D::CCommand_Polygon *pCmd = (CScratchPad3D::CCommand_Polygon*)pInCmd;

	int nVerts = min( 64, pCmd->m_Verts.Size() );
	for( int i=0; i < nVerts; i++ )
	{
		verts[i].m_Pos[0] = pCmd->m_Verts[i].m_vPos.x;
		verts[i].m_Pos[1] = pCmd->m_Verts[i].m_vPos.y;
		verts[i].m_Pos[2] = pCmd->m_Verts[i].m_vPos.z;
		verts[i].SetDiffuse( pCmd->m_Verts[i].m_vColor );
	}

	HRESULT hr;
	hr = g_pDevice->SetVertexShader( VertPosDiffuse::GetFVF() );

	// Draw wireframe manually since D3D draws internal edges of the triangle fan.
	DWORD dwFillMode;
	g_pDevice->GetRenderState( D3DRS_FILLMODE, &dwFillMode );
	if( dwFillMode == D3DFILL_WIREFRAME )
	{
		if( nVerts >= 2 )
		{
			g_pDevice->DrawPrimitiveUP( D3DPT_LINESTRIP, nVerts-1, verts, sizeof(verts[0]) );
			
			verts[nVerts] = verts[0];
			g_pDevice->DrawPrimitiveUP( D3DPT_LINESTRIP, 1, &verts[nVerts-1], sizeof(verts[0]) );
		}
	}
	else
	{
		hr = g_pDevice->DrawPrimitiveUP( D3DPT_TRIANGLEFAN, nVerts - 2, verts, sizeof(verts[0]) );
	}

	++g_nPolygons;
}

void CommandRender_Matrix( CScratchPad3D::CBaseCommand *pInCmd, IDirect3DDevice8 *pDevice )
{
	CScratchPad3D::CCommand_Matrix *pCmd = (CScratchPad3D::CCommand_Matrix*)pInCmd;

	VMatrix mTransposed = pCmd->m_mMatrix.Transpose();
	HRESULT hr = g_pDevice->SetTransform( D3DTS_WORLD, (D3DMATRIX*)mTransposed.m );
}

void CommandRender_RenderState( CScratchPad3D::CBaseCommand *pInCmd, IDirect3DDevice8 *pDevice )
{
	CScratchPad3D::CCommand_RenderState *pCmd = (CScratchPad3D::CCommand_RenderState*)pInCmd;

	switch( pCmd->m_State )
	{
		case IScratchPad3D::RS_FillMode:
		{
			if( pCmd->m_Val == IScratchPad3D::FillMode_Wireframe )
				g_pDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
			else
				g_pDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
		}
		break;

		case IScratchPad3D::RS_ZRead:
		{
			g_pDevice->SetRenderState( D3DRS_ZENABLE, pCmd->m_Val );
		}
		break;

		case IScratchPad3D::RS_ZBias:
		{
			g_pDevice->SetRenderState( D3DRS_ZBIAS, pCmd->m_Val );
		}
		break;
	}
}


typedef void (*CommandRenderFunction)( CScratchPad3D::CBaseCommand *pInCmd, IDirect3DDevice8 *pDevice );
CommandRenderFunction g_CommandRenderFunctions[CScratchPad3D::COMMAND_NUMCOMMANDS] =
{
	CommandRender_Point,
	CommandRender_Line,
	CommandRender_Polygon,
	CommandRender_Matrix,
	CommandRender_RenderState
};

void RunCommands( )
{
	// Set all the initial states.
	g_pDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	g_pDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );

	VMatrix mIdentity = SetupMatrixIdentity();
	g_pDevice->SetTransform( D3DTS_WORLD, (D3DMATRIX*)&mIdentity );

	for( int i=0; i < g_pScratchPad->m_Commands.Size(); i++ )
	{
		CScratchPad3D::CBaseCommand *pCmd = g_pScratchPad->m_Commands[i];

		if( pCmd->m_iCommand >= 0 && pCmd->m_iCommand < CScratchPad3D::COMMAND_NUMCOMMANDS )
		{
			g_CommandRenderFunctions[pCmd->m_iCommand]( pCmd, g_pDevice );
		}
	}
}


bool CheckForNewFile( bool bForce )
{
	// See if the file has changed..
	HANDLE hFile = CreateFile( 
		g_pScratchPad->m_pFilename, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL );

	if( !hFile )
		return false;

	FILETIME createTime, accessTime, writeTime;
	if( !GetFileTime( hFile, &createTime, &accessTime, &writeTime ) )
	{
		CloseHandle( hFile );
		return false;
	}

	bool bChange = false;
	if( memcmp(&writeTime, &g_LastWriteTime, sizeof(writeTime)) != 0 || bForce )
	{
		bChange = g_pScratchPad->LoadCommandsFromFile();
		if( bChange )
		{
			memcpy( &g_LastWriteTime, &writeTime, sizeof(writeTime) );
		}
	}

	CloseHandle( hFile );
	return bChange;
}


// ------------------------------------------------------------------------------------------ //
// App callbacks.
// ------------------------------------------------------------------------------------------ //

void UpdateWindowText()
{
	char str[512];
	sprintf( str, "ScratchPad3DViewer: <%s>  lines: %d, polygons: %d", g_Filename, g_nLines, g_nPolygons );
	Sys_SetWindowText( str );
}


void AppInit()
{
	// Viewer info.
	g_ViewController.m_vPos.Init( -200, 0, 0 );
	g_ViewController.m_vAngles.Init( 0, 0, 0 );

	char const *pFilename = Sys_FindArg( "-file", "scratch.pad" );
	Q_strncpy( g_Filename, pFilename, sizeof( g_Filename ) );

	IFileSystem *pFileSystem = ScratchPad3D_SetupFileSystem();
	if( !pFileSystem || pFileSystem->Init() != INIT_OK )
	{
		Sys_Quit();
	}

	// FIXME: I took this out of scratchpad 3d, not sure if this is even necessary any more
	pFileSystem->AddSearchPath( ".", "PLATFORM" );	

	g_pScratchPad = new CScratchPad3D( pFilename, pFileSystem, false );

	g_nLines = g_nPolygons = 0;
	UpdateWindowText();

	g_pDevice->SetRenderState( D3DRS_EDGEANTIALIAS, FALSE );
	g_pDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	g_pDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	g_pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	g_pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	g_pDevice->SetTexture( 0, NULL ); 

	// Setup point scaling parameters.
	float flOne=1;
	float flZero=0;
	g_pDevice->SetRenderState( D3DRS_POINTSCALEENABLE, TRUE );
	g_pDevice->SetRenderState( D3DRS_POINTSCALE_A, *((DWORD*)&flZero) );
	g_pDevice->SetRenderState( D3DRS_POINTSCALE_B, *((DWORD*)&flZero) );
	g_pDevice->SetRenderState( D3DRS_POINTSCALE_C, *((DWORD*)&flOne) );

	memset( &g_LastWriteTime, 0, sizeof(g_LastWriteTime) );
}


void AppRender( float frametime, float mouseDeltaX, float mouseDeltaY, bool bInvalidRect )
{
	g_nLines = 0;
	g_nPolygons = 0;
	
	if( !bInvalidRect && 
		!Sys_GetKeyState( APPKEY_LBUTTON ) && 
		!Sys_GetKeyState( APPKEY_RBUTTON ) && 
		!CheckForNewFile(false) )
	{
		Sys_Sleep( 100 );
		return;
	}

	g_pDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1, 0 );

	g_pDevice->BeginScene();

	UpdateView( mouseDeltaX, mouseDeltaY );
	
	RunCommands();

	g_pDevice->EndScene();

	g_pDevice->Present( NULL, NULL, NULL, NULL );

	UpdateWindowText();
}


void AppPreResize()
{
}

void AppPostResize()
{
}


void AppExit( )
{
}


void AppKey( int key, int down )
{
	if( key == 27 )
	{
		Sys_Quit();
	}
	else if( toupper(key) == 'U' )
	{
		CheckForNewFile( true );
		AppRender( 0.1f, 0, 0, true );
	}
}


void AppChar( int key )
{
}




