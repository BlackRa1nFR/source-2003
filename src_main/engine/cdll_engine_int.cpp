//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//  cdll_int.c
//
// 4-23-98  
// JOHN:  implementation of interface between client-side DLL and game engine.
//  The cdll shouldn't have to know anything about networking or file formats.
//  This file is Win32-dependant
//=============================================================================

#include "glquake.h"
#include "getintersectingsurfaces_struct.h"
#include "gl_model_private.h"
#include "surfinfo.h"
#include "vstdlib/random.h"
#include "cdll_int.h"
#include "cmodel_engine.h"
#include "vid.h"
#include "hud_handlers.h"
#include "tmessage.h"
#include "console.h"
#include "snd_audio_source.h"
#include <vgui_controls/Controls.h>
#include <vgui/IInput.h>
#include "IEngine.h"
#include "keys.h"
#include "con_nprint.h"
#include "game_interface.h"
#include "tier0/vprof.h"
#include "sound.h"
#include "gl_rmain.h"
#include "measure_section.h"
#include "proto_version.h"
#include "client_class.h"
#include "gl_rsurf.h"
#include "dispbuild.h"
#include "server.h"
#include "r_local.h"
#include "lightcache.h"
#include "gl_matsysiface.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "IStudioRender.h"
#include "l_studio.h"
#include "voice.h"
#include "EngineStats.h"
#include "testscriptmgr.h"
#include "r_areaportal.h"
#include "host.h"
#include "vox.h"
#include "iprediction.h"
#include "icliententitylist.h"
#include "ivguicenterprint.h"
#include "engine/IClientLeafSystem.h"
#include "dt_recv_eng.h"
#include <vgui/IVGui.h>
#include "sys_dll.h"
#include "vphysics_interface.h"
#include "materialsystem/IMesh.h"
#include "IOcclusionSystem.h"
#include "FileSystem_Engine.h"
#include "vstdlib/icommandline.h"
#include "client_textmessage.h"
#include "host_saverestore.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
IMaterial* BrushModel_GetLightingAndMaterial( const Vector &start, 
	const Vector &end, Vector &diffuseLightColor, Vector &baseColor );
char *COM_ParseFile(char *data, char *token);
const char *Key_NameForBinding( const char *pBinding );

extern float scr_fov_value;
extern bool scr_drawloading;	


//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------
static CSysModule		*g_ClientDLLModule = NULL;
static ConVar vcollide_axes( "vcollide_axes", "0" );


void AddIntersectingLeafSurfaces( mleaf_t *pLeaf, GetIntersectingSurfaces_Struct *pStruct )
{
	for ( int iSurf=0; iSurf < pLeaf->nummarksurfaces; iSurf++ )
	{
		int surfID = host_state.worldmodel->brush.marksurfaces[pLeaf->firstmarksurface + iSurf];
		ASSERT_SURF_VALID( surfID );
		
		if ( MSurf_Flags(surfID) & SURFDRAW_SKY )
			continue;

		// Make sure we haven't already processed this one.
		bool foundSurf = false;
		for(int iTest=0; iTest < pStruct->m_nSetInfos; iTest++)
		{
			if(pStruct->m_pInfos[iTest].m_pEngineData == (void *)surfID)
			{
				foundSurf = true;
				break;
			}
		}
		if ( foundSurf )
			continue;

		// Make sure there's output room.
		if(pStruct->m_nSetInfos >= pStruct->m_nMaxInfos)
			return;
		SurfInfo *pOut = &pStruct->m_pInfos[pStruct->m_nSetInfos];
		pOut->m_nVerts = 0;
		pOut->m_pEngineData = (void *)surfID;

		// Build vertex list and bounding box.			
		Vector vMin( 1000000.0f,  1000000.0f,  1000000.0f);
		Vector vMax(-1000000.0f, -1000000.0f, -1000000.0f);
		for(int iVert=0; iVert < MSurf_VertCount( surfID ); iVert++)
		{
			int vertIndex = pStruct->m_pModel->brush.vertindices[MSurf_FirstVertIndex( surfID ) + iVert];

			pOut->m_Verts[pOut->m_nVerts] = pStruct->m_pModel->brush.vertexes[vertIndex].position;
			vMin = vMin.Min(pOut->m_Verts[pOut->m_nVerts]);
			vMax = vMax.Max(pOut->m_Verts[pOut->m_nVerts]);

			++pOut->m_nVerts;
			if(pOut->m_nVerts >= MAX_SURFINFO_VERTS)
				break;
		}

		// See if the sphere intersects the box.
		for(int iDim=0; iDim < 3; iDim++)
		{
			if(((*pStruct->m_pCenter)[iDim]+pStruct->m_Radius) < vMin[iDim] || 
				((*pStruct->m_pCenter)[iDim]-pStruct->m_Radius) > vMax[iDim])
			{
				break;
			}
		}
		
		if(iDim == 3)
		{
			// (Couldn't reject the sphere in the loop above).
			pOut->m_Plane = MSurf_GetForwardFacingPlane( surfID );
			++pStruct->m_nSetInfos;
		}
	}
}

void GetIntersectingSurfaces_R(
	GetIntersectingSurfaces_Struct *pStruct,
	mnode_t *pNode
	)
{
	if(pStruct->m_nSetInfos >= pStruct->m_nMaxInfos)
		return;

	// Ok, this is a leaf. Check its surfaces.
	if(pNode->contents >= 0)
	{
		mleaf_t *pLeaf = (mleaf_t*)pNode;

		if(pStruct->m_bOnlyVisible && pStruct->m_pCenterPVS)
		{
			if(pLeaf->cluster < 0)
				return;

			if(!(pStruct->m_pCenterPVS[pLeaf->cluster>>3] & (1 << (pLeaf->cluster&7))))
				return;
		}

		// First, add tris from displacements.
		for( CDispIterator it=pLeaf->GetDispIterator(); it.IsValid(); )
		{
			CDispLeafLink *pCur = it.Inc();
			IDispInfo *pDispInfo = (IDispInfo*)pCur->m_pDispInfo;
			pDispInfo->GetIntersectingSurfaces( pStruct );
		}


		// Next, add brush tris.
		AddIntersectingLeafSurfaces( pLeaf, pStruct );
		return;
	}
	
	// Recurse.
	float dot;
	cplane_t *plane = pNode->plane;
	if ( plane->type < 3 )
	{
		dot = (*pStruct->m_pCenter)[plane->type] - plane->dist;
	}
	else
	{
		dot = pStruct->m_pCenter->Dot(plane->normal) - plane->dist;
	}

	// Recurse into child nodes.
	if(dot > -pStruct->m_Radius)
		GetIntersectingSurfaces_R(pStruct, pNode->children[SIDE_FRONT]);
	
	if(dot < pStruct->m_Radius)
		GetIntersectingSurfaces_R(pStruct, pNode->children[SIDE_BACK]);
}


//-----------------------------------------------------------------------------
// slow routine to draw a physics model
// NOTE: very slow code!!! just for debugging!
//-----------------------------------------------------------------------------
void DebugDrawPhysCollide( const CPhysCollide *pCollide, IMaterial *pMaterial, matrix3x4_t& transform, const color32 &color )
{
	if ( !pMaterial )
	{
		pMaterial = materialSystemInterface->FindMaterial("shadertest/wireframevertexcolor", NULL);
	}

	Vector *outVerts;
	int vertCount = physcollision->CreateDebugMesh( pCollide, &outVerts );
	if ( vertCount )
	{
		IMesh *pMesh = materialSystemInterface->GetDynamicMesh( true, NULL, NULL, pMaterial );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, vertCount/3 );

		for ( int j = 0; j < vertCount; j++ )
		{
			Vector out;
			VectorTransform( outVerts[j].Base(), transform, out.Base() );
			meshBuilder.Position3fv( out.Base() );
			meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
			meshBuilder.TexCoord2f( 0, 0, 0 );
			meshBuilder.AdvanceVertex();
		}
		meshBuilder.End();
		pMesh->Draw();
	}
	physcollision->DestroyDebugMesh( vertCount, outVerts );

	// draw the axes
	if ( vcollide_axes.GetBool() )
	{
		Vector xaxis(10,0,0), yaxis(0,10,0), zaxis(0,0,10);
		Vector center;
		Vector out;

		MatrixGetColumn( transform, 3, center );
		IMesh *pMesh = materialSystemInterface->GetDynamicMesh( true, NULL, NULL, pMaterial );
		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_LINES, 3 );

		// X
		meshBuilder.Position3fv( center.Base() );
		meshBuilder.Color4ub( 255, 0, 0, 255 );
		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.AdvanceVertex();
		VectorTransform( xaxis.Base(), transform, out.Base() );
		meshBuilder.Position3fv( out.Base() );
		meshBuilder.Color4ub( 255, 0, 0, 255 );
		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.AdvanceVertex();

		// Y
		meshBuilder.Position3fv( center.Base() );
		meshBuilder.Color4ub( 0, 255, 0, 255 );
		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.AdvanceVertex();
		VectorTransform( yaxis.Base(), transform, out.Base() );
		meshBuilder.Position3fv( out.Base() );
		meshBuilder.Color4ub( 0, 255, 0, 255 );
		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.AdvanceVertex();

		// Z
		meshBuilder.Position3fv( center.Base() );
		meshBuilder.Color4ub( 0, 0, 255, 255 );
		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.AdvanceVertex();
		VectorTransform( zaxis.Base(), transform, out.Base() );
		meshBuilder.Position3fv( out.Base() );
		meshBuilder.Color4ub( 0, 0, 255, 255 );
		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.AdvanceVertex();
		meshBuilder.End();
		pMesh->Draw();
	}
}

//-----------------------------------------------------------------------------
//
// implementation of IVEngineHud
//
//-----------------------------------------------------------------------------

// UNDONE: Move this to hud export code, subsume previous functions
class CEngineClient : public IVEngineClient
{
	cmodel_t *GetCModel(int modelindex)
	{
		if (modelindex >= MAX_MODELS || modelindex < 0)
		{
			return NULL;
		}

		if (cmodel_t *pCModel = CM_InlineModelNumber(modelindex-1))
		{
			return pCModel;
		}

		return NULL;
	}

	int		GetIntersectingSurfaces(
		const model_t *model,
		const Vector &vCenter, 
		const float radius,
		const bool bOnlyVisible,
		SurfInfo *pInfos, 
		const int nMaxInfos)
	{
		if ( !model )
			return 0;

		GetIntersectingSurfaces_Struct theStruct;
		theStruct.m_pModel = ( model_t * )model;
		theStruct.m_pCenter = &vCenter;
		theStruct.m_pCenterPVS = CM_ClusterPVS( CM_LeafCluster( CM_PointLeafnum( vCenter ) ) );
		theStruct.m_Radius = radius;
		theStruct.m_bOnlyVisible = bOnlyVisible;
		theStruct.m_pInfos = pInfos;
		theStruct.m_nMaxInfos = nMaxInfos;
		theStruct.m_nSetInfos = 0;		

		// Go down the BSP.
		GetIntersectingSurfaces_R(
			&theStruct,
			&model->brush.nodes[ model->brush.firstnode ] );

		return theStruct.m_nSetInfos;
	}

	Vector	GetLightForPoint(const Vector &pos, bool bClamp)
	{
		extern Vector R_TestLightForPoint(const Vector &vPoint, bool bClamp);
		return R_TestLightForPoint(pos, bClamp);
	}

	char *COM_ParseFile( char *data, char *token )
	{
		return ::COM_ParseFile( data, token );
	}

	void GetScreenSize( int& w, int &h )
	{
		w = vid.width;
		h = vid.height;
	}

	int ServerCmd( const char *szCmdString, bool bReliable )
	{
		return hudServerCmd( (char *)szCmdString, bReliable );
	}

	int ClientCmd( const char *szCmdString )
	{
		return hudClientCmd( (char *)szCmdString );
	}

	void GetPlayerInfo( int ent_num, hud_player_info_t *pinfo )
	{
		hudGetPlayerInfo( ent_num, pinfo );
	}

	bool GetPlayerUniqueID(int iPlayer, char playerID[16])
	{
		--iPlayer;	// make into a client index
		if(iPlayer < 0 || iPlayer >= cl.maxclients)
			return false;

		// Make sure there is a player here..
		if(!cl.players[iPlayer].userinfo[0] || !cl.players[iPlayer].name[0])
			return false;

		memcpy(playerID, cl.players[iPlayer].hashedcdkey, 16);
		return true;
	}

	client_textmessage_t *TextMessageGet( const char *pName )
	{
		return ::TextMessageGet( pName );
	}

	bool Con_IsVisible( void )
	{
		return ::Con_IsVisible();
	}

	int GetPlayer( void )
	{
		return cl.playernum  + 1;
	}

	double	GetLastTimeStamp( void )
	{
		return cl.mtime[0];
	}

	const model_t *LoadModel( const char *pName, bool bProp )
	{
		return modelloader->GetModelForName( pName, bProp ? IModelLoader::FMODELLOADER_DETAILPROP : IModelLoader::FMODELLOADER_CLIENTDLL );
	}

	void UnloadModel( const model_t *model, bool bProp )
	{
		modelloader->ReleaseModel( (model_t *)model, bProp ? IModelLoader::FMODELLOADER_DETAILPROP : IModelLoader::FMODELLOADER_CLIENTDLL );
	}

	CSentence *GetSentence( CAudioSource *pAudioSource )
	{
		if (pAudioSource)
		{
			return pAudioSource->GetSentence();
		}
		return NULL;
	}

	float GetSentenceLength( CAudioSource *pAudioSource )
	{
		if (pAudioSource && pAudioSource->SampleRate() > 0 )
		{
			float length = (float)pAudioSource->SampleCount() / (float)pAudioSource->SampleRate();
			return length;
		}
		return 0.0f;
	}

	int GetWindowCenterX( void )
	{
		return vid.width >> 1;
	}

	int GetWindowCenterY( void )
	{
		return vid.height >> 1;
	}

	void SetCursorPos( int x, int y )
	{
		vgui::input()->SetCursorPos( x, y );
	}
	
	// FIXME, move entirely to client .dll
	void GetViewAngles( QAngle& va )
	{
		VectorCopy( cl.viewangles, va );
	}
	
	void SetViewAngles( QAngle& va )
	{
		VectorCopy( va, cl.viewangles );
	}
	
	int GetMaxClients( void )
	{
		return cl.maxclients;
	}

	void Key_Event( int key, int down )
	{
		eng->TrapKey_Event( key, down );
	}

	const char *Key_LookupBinding( const char *pBinding )
	{
		return ::Key_NameForBinding( pBinding );
	}

	int		IsInGame( void )
	{
		return ( ( cls.state == ca_active ) && ( cls.signon == SIGNONS ) ) ? 1 : 0;
	}

	int		IsConnected( void )
	{
		return ( ( cls.state != ca_dedicated ) && ( cls.state != ca_disconnected ) ) ? 1 : 0;
	}
	
	bool	IsDrawingLoadingImage( void )
	{
		return scr_drawloading;
	}

	void OutputDebugStringToDeveloperConsole( const char *str )
	{
		DebugOutStraight( (char *)str );
	}

	void Con_NPrintf( int pos, char *fmt, ... )
	{
		va_list		argptr;
		char		text[4096];
		va_start (argptr, fmt);
		Q_vsnprintf(text, sizeof( text ), fmt, argptr);
		va_end (argptr);

		::Con_NPrintf( pos, text );
	}

	void Con_NXPrintf( struct con_nprint_s *info, char *fmt, ... )
	{
		va_list		argptr;
		char		text[4096];
		va_start (argptr, fmt);
		Q_vsnprintf(text, sizeof( text ), fmt, argptr);
		va_end (argptr);

		::Con_NXPrintf( info, text );
	}

	int Cmd_Argc( void )
	{
		return ::Cmd_Argc();	
	}

	char *Cmd_Argv( int arg )
	{
		return ::Cmd_Argv( arg );
	}

	IMaterial *TraceLineMaterialAndLighting( const Vector &start, const Vector &end, 
		                                     Vector &diffuseLightColor, Vector &baseColor )
	{
		return BrushModel_GetLightingAndMaterial( start, end, diffuseLightColor, baseColor );
	}

	int		IsBoxVisible( const Vector& mins, const Vector& maxs ) 
	{
		return CM_BoxVisible( mins, maxs, Map_VisCurrent() );
	}

	int		IsBoxInViewCluster( const Vector& mins, const Vector& maxs )
	{
		byte *ppvs;
		ppvs = CM_ClusterPVS( Map_VisCurrentCluster() );
		return CM_BoxVisible(mins, maxs, ppvs);
	}

	// Extracts the base name of a file (no path, no extension, assumes '/' or '\\' as path separator)
	void COM_FileBase( const char *in, char *out )
	{
		::COM_FileBase( in, out );
	}

	struct cmd_s *GetCommand( int command_number )
	{
		if ( command_number < 0 || 
			 command_number >= MULTIPLAYER_BACKUP )
		{
			return &cl.commands[ 0 ];
		}

		return &cl.commands[ command_number ];
	}

	float Time()
	{
		return Sys_FloatTime();
	}

	void Sound_ExtraUpdate( void )
	{
		VPROF_BUDGET( "CEngineClient::Sound_ExtraUpdate()", VPROF_BUDGETGROUP_OTHER_SOUND );
		S_ExtraUpdate();
	}

	void SetFieldOfView( float fov )
	{
		scr_fov_value = fov;
	}

	bool CullBox ( const Vector& mins, const Vector& maxs )
	{
		return R_CullBox( mins, maxs, g_Frustum );
	}

	CMeasureSection *GetMeasureSectionList( void )
	{
		return CMeasureSection::GetList();
	}

	const char *GetGameDirectory( void )
	{
		return com_gamedir;
	}
	
	const char *GetVersionString( void )
	{
		static char ver[ 128 ];
		Q_snprintf(ver, sizeof( ver ), "Source Engine %i/%s (build %d)", PROTOCOL_VERSION, gpszVersionString, build_number() );
		return ver;
	}

	const VMatrix& WorldToScreenMatrix()
	{
		return g_EngineRenderer->WorldToScreenMatrix();
	}

	const VMatrix& WorldToViewMatrix()
	{
		return g_EngineRenderer->ViewMatrix();
	}

	// Loads a game lump off disk
	int		GameLumpVersion( int lumpId ) const
	{
		return Mod_GameLumpVersion( lumpId ); 
	}

	int		GameLumpSize( int lumpId ) const 
	{ 
		return Mod_GameLumpSize( lumpId ); 
	}

	bool	LoadGameLump( int lumpId, void* pBuffer, int size ) 
	{ 
		return Mod_LoadGameLump( lumpId, pBuffer, size ); 
	}

	// Returns the number of leaves in the level
	int		LevelLeafCount() const
	{
		return host_state.worldmodel->brush.numleafs;
	}

	virtual ISpatialQuery* GetBSPTreeQuery()
	{
		return g_pToolBSPTree;
	}

	int	ApplyTerrainMod_Internal(
		TerrainModType type,
		CTerrainModParams const &params,
		CSpeculativeTerrainModVert *pSpeculativeVerts,
		int nMaxVerts )
	{
		ITerrainMod *pMod = SetupTerrainMod( type, params );

		Vector modMin, modMax;
		pMod->GetBBox( modMin, modMax );

		int nModifiedVerts = 0;
		
		// Apply the mod to rendering.		
		for( int iDisp=0; iDisp < host_state.worldmodel->brush.numDispInfos; iDisp++ )
		{
			IDispInfo *pDisp = DispInfo_IndexArray( host_state.worldmodel->brush.hDispInfos, iDisp );
			
			// Trivial reject.
			Vector bbMin, bbMax;
			pDisp->GetBoundingBox( bbMin, bbMax );

			if( QuickBoxIntersectTest( modMin, modMax, bbMin, bbMax ) )
			{			
				if( pSpeculativeVerts )
				{
					nModifiedVerts += pDisp->ApplyTerrainMod_Speculative( 
						pMod, 
						&pSpeculativeVerts[nModifiedVerts], 
						nMaxVerts-nModifiedVerts );
				}
				else
				{
					pDisp->ApplyTerrainMod( pMod );
					ReAddDispSurfToLeafs( host_state.worldmodel, pDisp );
				}
			}
		}

		// Apply it to physics unless we've got a local server in which case
		// the mod has already been applied.
		if( !sv.active )
		{
			CM_ApplyTerrainMod( pMod );
		}

		return nModifiedVerts;
	}
		

	virtual void		ApplyTerrainMod( TerrainModType type, CTerrainModParams const &params )
	{
		ApplyTerrainMod_Internal( type, params, NULL, 0 );
	}

	virtual int			ApplyTerrainMod_Speculative( 
		TerrainModType type,
		CTerrainModParams const &params,
		CSpeculativeTerrainModVert *pSpeculativeVerts,
		int nMaxVerts )
	{
		return ApplyTerrainMod_Internal( type, params, pSpeculativeVerts, nMaxVerts );
	}

	// Convert texlight to gamma...
	virtual void		LinearToGamma( float* linear, float* gamma )
	{
		gamma[0] = LinearToTexture( linear[0] ) / 255.0f;
		gamma[1] = LinearToTexture( linear[1] ) / 255.0f;
		gamma[2] = LinearToTexture( linear[2] ) / 255.0f;
	}

	// Get the lightstyle value
	virtual float		LightStyleValue( int style )
	{
		return ::LightStyleValue( style );
	}


	virtual void DrawPortals()
	{
		R_DrawPortals();
	}

	// Computes light due to dynamic lighting at a point
	// If the normal isn't specified, then it'll return the maximum lighting
	virtual void ComputeDynamicLighting( Vector const& pt, Vector const* pNormal, Vector& color )
	{
		::ComputeDynamicLighting( pt, pNormal, color );
	}

	// Computes light due to dynamic lighting at a point
	// If the normal isn't specified, then it'll return the maximum lighting
	virtual void ComputeLighting( const Vector& pt, const Vector* pNormal, bool bClamp, Vector& color )
	{
		::ComputeLighting( pt, pNormal, bClamp, color );
	}

	// Returns the color of the ambient light
	virtual void GetAmbientLightColor( Vector& color )
	{
		dworldlight_t* pWorldLight = FindAmbientLight();
		if (!pWorldLight)
			color.Init( 0, 0, 0 );
		else
			VectorCopy( pWorldLight->intensity, color );
	}

	// Returns the dx support level
	virtual int	GetDXSupportLevel()
	{
		return g_pMaterialSystemHardwareConfig->GetDXSupportLevel();
	}
	
	// GR - HDR support
	virtual bool SupportsHDR()
	{
		return g_pMaterialSystemHardwareConfig->SupportsHDR();
	}

	virtual void Mat_Stub( IMaterialSystem *pMatSys )
	{
		materialSystemInterface = pMatSys;
		
		// Pass the call to the model renderer.
		if ( g_pStudioRender )
			g_pStudioRender->Mat_Stub( pMatSys );
	}

	virtual char const *GetLevelName( void )
	{
		if ( cls.state == ca_dedicated )
		{
			return "Dedicated Server";
		}
		else if ( cls.state == ca_disconnected )
		{
			return "";
		}

		return cl.levelname;
	}

	virtual void PlayerInfo_SetValueForKey( const char *key, const char *value )
	{
		Info_SetValueForStarKey(cls.userinfo, key, value, MAX_INFO_STRING );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Returns a value from the specified players infostring
	// Input  : playerNum - 
	//			*key - 
	// Output : const char
	//-----------------------------------------------------------------------------
	virtual const char *PlayerInfo_ValueForKey(int playerNum, const char *key)
	{
		// find the player
		if ( ( playerNum > cl.maxclients ) ||
			 ( playerNum < 1 ) ||
			 ( cl.players[playerNum-1].name == NULL ) ||
			 ( *(cl.players[playerNum-1].name) == 0 ) )
		{
			return NULL;
		}

		return Info_ValueForKey(cl.players[playerNum-1].userinfo, key);
	}

	//-----------------------------------------------------------------------------
	// Purpose: Takes a player slot and returns their TrackerID
	//			returns 0 if user doesn't have a TrackerID
	//-----------------------------------------------------------------------------
	virtual int GetTrackerIDForPlayer(int playerSlot)
	{
		--playerSlot;	// make into a client index
		if (!cl.players[playerSlot].userinfo[0] || !cl.players[playerSlot].name[0])
			return 0;

		return cl.players[playerSlot].trackerID;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Takes a trackerID and returns which player slot that user is in
	//			returns 0 if no player found with that ID
	//-----------------------------------------------------------------------------
	virtual int	GetPlayerForTrackerID(int trackerID)
	{
		int i;

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (!cl.players[i].userinfo[0] || !cl.players[i].name[0])
				continue;

			if (cl.players[i].trackerID == trackerID)
			{
				// make into a player slot
				return (i+1);
			}
		}

		return 0;
	}

	virtual struct IVoiceTweak_s *GetVoiceTweakAPI( void )
	{
		return &g_VoiceTweakAPI;
	}

	virtual void EngineStats_BeginFrame( void )
	{
		g_EngineStats.BeginFrame();
	}

	virtual void EngineStats_EndFrame( void )
	{
		g_EngineStats.EndFrame();
	}

	virtual void FireEvents()
	{
		// Run any events queued up for this frame
		CL_FireEvents();
	}

	virtual void CheckPoint( const char *pName )
	{
		GetTestScriptMgr()->CheckPoint( pName );
	}

	virtual int GetLeavesArea( int *pLeaves, int nLeaves )
	{
		if ( nLeaves == 0 )
			return -1;

		int iArea = host_state.worldmodel->brush.leafs[pLeaves[0]].area;
		for ( int i=1; i < nLeaves; i++ )
		{
			int iTestArea = host_state.worldmodel->brush.leafs[pLeaves[i]].area;
			if ( iTestArea != iArea )
				return -1;
		}

		return iArea;
	}

	virtual bool DoesBoxTouchAreaFrustum( const Vector &mins, const Vector &maxs, int iArea )
	{
		const Frustum_t *pFrustum = GetAreaFrustum( iArea );
		return !R_CullBox( mins, maxs, *pFrustum );
	}

	// Sets the hearing origin
	virtual void SetHearingOrigin( const Vector &vecOrigin, const QAngle &angles )
	{
		h_origin = vecOrigin;
		AngleVectors( angles, &h_forward, &h_right, &h_up );
	}

	//-----------------------------------------------------------------------------
	//
	// Sentence API
	//
	//-----------------------------------------------------------------------------

	virtual int SentenceGroupPick( int groupIndex, char *name, int nameLen )
	{
		return VOX_GroupPick( groupIndex, name, nameLen );
	}


	virtual int SentenceGroupPickSequential( int groupIndex, char *name, int nameLen, int sentenceIndex, int reset )
	{
		return VOX_GroupPickSequential( groupIndex, name, nameLen, sentenceIndex, reset );
	}

	virtual int SentenceIndexFromName( const char *pSentenceName )
	{
		int sentenceIndex = -1;
		
		VOX_LookupString( pSentenceName, &sentenceIndex );
		
		return sentenceIndex;
	}

	virtual const char *SentenceNameFromIndex( int sentenceIndex )
	{
		return VOX_SentenceNameFromIndex( sentenceIndex );
	}


	virtual int SentenceGroupIndexFromName( const char *pGroupName )
	{
		return VOX_GroupIndexFromName( pGroupName );
	}

	virtual const char *SentenceGroupNameFromIndex( int groupIndex )
	{
		return VOX_GroupNameFromIndex( groupIndex );
	}


	virtual float SentenceLength( int sentenceIndex )
	{
		return VOX_SentenceLength( sentenceIndex );
	}

	virtual void DebugDrawPhysCollide( const CPhysCollide *pCollide, IMaterial *pMaterial, matrix3x4_t& transform, const color32 &color )
	{
		::DebugDrawPhysCollide( pCollide, pMaterial, transform, color );
	}

	// Activates/deactivates an occluder...
	virtual void ActivateOccluder( int nOccluderIndex, bool bActive )
	{
		OcclusionSystem()->ActivateOccluder( nOccluderIndex, bActive );
	}

	virtual void		*SaveAllocMemory( size_t num, size_t size )
	{
		return ::SaveAllocMemory( num, size );
	}

	virtual void		SaveFreeMemory( void *pSaveMem )
	{
		::SaveFreeMemory( pSaveMem );
	}
};

static CEngineClient s_VEngineClient;
IVEngineClient *engineClient = &s_VEngineClient;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CEngineClient, IVEngineClient, VENGINE_CLIENT_INTERFACE_VERSION, s_VEngineClient );


// --------------------------------------------------------------------------------- //
// IVEngineCache
// --------------------------------------------------------------------------------- //

class CEngineCache : public IVEngineCache
{
	void Flush( void )
	{
		Cache_Flush();
	}
	
	// returns the cached data, and moves to the head of the LRU list
	// if present, otherwise returns NULL
	void *Check( cache_user_t *c )
	{
		return Cache_Check( c );
	}

	void Free( cache_user_t *c )
	{
		Cache_Free( c );
	}
	
	// Returns NULL if all purgable data was tossed and there still
	// wasn't enough room.
	void *Alloc( cache_user_t *c, int size, const char *name )
	{
		return Cache_Alloc( c, size, name );
	}
	
	void Report( void )
	{
		Cache_Report();
	}
	
	// all cache entries that subsequently allocated or successfully checked 
	// are considered "locked" and will not be freed when additional memory is needed
	void EnterCriticalSection()
	{
		Cache_EnterCriticalSection();
	}

	// reset all protected blocks to normal
	void ExitCriticalSection()
	{
		Cache_ExitCriticalSection();
	}
};

EXPOSE_SINGLE_INTERFACE( CEngineCache, IVEngineCache, VENGINE_CACHE_INTERFACE_VERSION );

// The client DLL serves out this interface
IBaseClientDLL *g_ClientDLL = NULL;

// TODO: get this info from the initial update packet
static int iDataSize = 4;	// the size of the hud data update struct

// TODO: allocate this data after iDataSize has been received
static byte *pData = NULL;	// pointer to the local copy of the hud data update struct

IPrediction		*g_pClientSidePrediction = NULL;
IClientEntityList *entitylist = NULL;
ICenterPrint *centerprint = NULL;
static IClientStats* s_pClientStats = 0;
IClientLeafSystemEngine* clientleafsystem = NULL;


static void ClientDLL_InitRecvTableMgr()
{
	// Register all the receive tables.
	RecvTable *pRecvTables[MAX_DATATABLES];
	int nRecvTables = 0;
	for ( ClientClass *pCur = g_ClientDLL->GetAllClasses(); pCur; pCur=pCur->m_pNext )
	{
		ErrorIfNot( 
			nRecvTables < ARRAYSIZE( pRecvTables ), 
			("ClientDLL_InitRecvTableMgr: overflowed MAX_DATATABLES")
			);
		
		pRecvTables[nRecvTables] = pCur->m_pRecvTable;
		++nRecvTables;
	}

	RecvTable_Init( pRecvTables, nRecvTables );
}


static void ClientDLL_ShutdownRecvTableMgr()
{
	RecvTable_Term();
}


//-----------------------------------------------------------------------------
// Purpose: Loads the client .dll
//-----------------------------------------------------------------------------
void ClientDLL_Init( void )
{
	char szDllName[512];

	Assert ( !g_ClientDLLModule );

	Q_snprintf( szDllName, sizeof( szDllName ), "bin\\client.dll" );

	COM_ExpandFilename( szDllName );
	COM_FixSlashes(szDllName);  // Otherwise Sys_GetFactory() fails sometimes(on net drives for instance)

	// clear the pic pointers
	g_ClientDLLModule = FileSystem_LoadModule(szDllName);
	if ( g_ClientDLLModule )
	{
		CreateInterfaceFn clientFactory = Sys_GetFactory( g_ClientDLLModule );
		if ( clientFactory )
		{
			g_ClientDLL = (IBaseClientDLL *)clientFactory( CLIENT_DLL_INTERFACE_VERSION, NULL );
			if ( !g_ClientDLL )
			{
				Sys_Error( "Could not get client.dll interface from library %s", szDllName );
			}

			if ( !g_ClientDLL->Init(g_AppSystemFactory, physicsFactory, &g_ClientGlobalVariables ) )
			{
				Sys_Error("Client.dll Init() in library %s failed.", szDllName);
			}

			// Load the prediction interface from the client .dll
			g_pClientSidePrediction = (IPrediction *)clientFactory( VCLIENT_PREDICTION_INTERFACE_VERSION, NULL );
			if ( !g_pClientSidePrediction )
			{
				Sys_Error( "Could not get IPrediction interface from library %s", szDllName );
			}
			g_pClientSidePrediction->Init();

			s_pClientStats = (IClientStats *)clientFactory( INTERFACEVERSION_CLIENTSTATS, NULL );
			if ( !s_pClientStats )
			{
				Sys_Error( "Could not get client stats interface from library %s", szDllName );
			}
			g_EngineStats.InstallClientStats( s_pClientStats ); 			

			entitylist = ( IClientEntityList  *)clientFactory( VCLIENTENTITYLIST_INTERFACE_VERSION, NULL );
			if ( !entitylist )
			{
				Sys_Error( "Could not get client entity list interface from library %s", szDllName );
			}

			centerprint = ( ICenterPrint * )clientFactory( VCENTERPRINT_INTERFACE_VERSION, NULL );
			if ( !centerprint )
			{
				Sys_Error( "Could not get centerprint interface from library %s", szDllName );
			}

			clientleafsystem = ( IClientLeafSystemEngine *)clientFactory( CLIENTLEAFSYSTEM_INTERFACE_VERSION, NULL );
			if ( !clientleafsystem )
			{
				Sys_Error( "Could not get client leaf system interface from library %s", szDllName );
			}
		}
		else
		{
			Sys_Error( "Could not find factory interface in library %s", szDllName );
		}
	}
	else
	{	
		// library failed to load
		Sys_Error( "Could not load library %s", szDllName );
	}

	if ( CommandLine()->FindParm( "-showlogo" ) )
	{
		// Enable Logo
		int i = 1;
		DispatchDirectUserMsg("Logo", 1, (void *)&i);
	}

	ClientDLL_InitRecvTableMgr();
}

//-----------------------------------------------------------------------------
// Purpose: Unloads the client .dll
//-----------------------------------------------------------------------------
void ClientDLL_Shutdown( void )
{
	ClientDLL_ShutdownRecvTableMgr();

	vgui::ivgui()->RunFrame();
	
	materialSystemInterface->UncacheAllMaterials();

	vgui::ivgui()->RunFrame();

	g_pClientSidePrediction->Shutdown();

	g_ClientDLL->Shutdown();

	cv->RemoveHudCvars();
	// Unhook the clientstats interface
	g_EngineStats.RemoveClientStats(s_pClientStats);

	FileSystem_UnloadModule( g_ClientDLLModule );
	g_ClientDLLModule = NULL;

	g_ClientDLL = NULL;
	entitylist = NULL;
	g_pClientSidePrediction = NULL;
	centerprint = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Called when the game initializes and whenever the vid_mode is changed
//   so the HUD can reinitialize itself.
//-----------------------------------------------------------------------------
void ClientDLL_HudVidInit( void )
{
	g_ClientDLL->HudVidInit();
}

//-----------------------------------------------------------------------------
// Purpose: Allow client .dll to modify input data
//-----------------------------------------------------------------------------

void ClientDLL_ProcessInput( void )
{
	if ( cls.state == ca_dedicated )
		return;

	if ( !g_ClientDLL )
		return;

	g_ClientDLL->HudProcessInput( true );
}


void ClientDLL_FrameStageNotify( ClientFrameStage_t frameStage )
{
	if ( cls.state == ca_dedicated || !g_ClientDLL )
		return;

	g_ClientDLL->FrameStageNotify( frameStage );
}


//-----------------------------------------------------------------------------
// Purpose: Allow client .dll to think
//-----------------------------------------------------------------------------
void ClientDLL_Update( void )
{
	if ( cls.state == ca_dedicated )
		return;

	if ( !g_ClientDLL )
		return;

	g_ClientDLL->HudUpdate( true );
}



void ClientDLL_VoiceStatus( int entindex, bool bTalking )
{
	if( g_ClientDLL )
		g_ClientDLL->VoiceStatus( entindex, bTalking );
}


