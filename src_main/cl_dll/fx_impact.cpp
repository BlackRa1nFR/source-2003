//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "decals.h"
#include "materialsystem/IMaterialVar.h"
#include "c_splash.h"
#include "ieffects.h"
#include "fx.h"
#include "fx_impact.h"
#include "view.h"
#include "engine/IStaticPropMgr.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool Impact( Vector &vecOrigin, Vector &vecStart, int iMaterial, int iDamageType, int iHitbox, int iEntIndex, trace_t &tr, bool bDecal, int maxLODToDecal )
{
	// Clear out the trace
	memset( &tr, 0, sizeof(trace_t));
	tr.fraction = 1.f;

	// Setup our shot information
	Vector shotDir = vecOrigin - vecStart;
	float flLength = VectorNormalize( shotDir );
	Vector traceExt;
	VectorMA( vecStart, flLength + 8.0f, shotDir, traceExt );

	// Attempt to hit ragdolls
	bool bHitRagdoll = AffectRagdolls( vecOrigin, vecStart, iDamageType );

	// Get the entity we hit, according to the server
	C_BaseEntity *pEntity = ClientEntityList().GetEnt( iEntIndex );

	if ( bDecal )
	{
		int decalNumber = decalsystem->GetDecalIndexForName( GetImpactDecal( pEntity, iMaterial, iDamageType ) );
		if ( decalNumber == -1 )
			return false;

		if ( (iEntIndex == 0) && (iHitbox != 0) )
		{
			// Special case for world entity with hitbox (that's a static prop):
			// In this case, we've hit a static prop. Decal it!
			staticpropmgr->AddDecalToStaticProp( vecStart, traceExt, iHitbox - 1, decalNumber, true, tr );
		}
		else if ( pEntity )
		{
			// Here we deal with decals on entities.
			pEntity->AddDecal( vecStart, traceExt, vecOrigin, iHitbox, decalNumber, true, tr, maxLODToDecal );
		}
	}
	else
	{
		// Perform the trace ourselves
		Ray_t ray;
		ray.Init( vecStart, traceExt );
		enginetrace->ClipRayToEntity( ray, MASK_SHOT, pEntity, &tr );
	}

	// If we found the surface, emit debris flecks
	if ( ( tr.fraction == 1.0f ) || ( bHitRagdoll ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *GetImpactDecal( C_BaseEntity *pEntity, int iMaterial, int iDamageType )
{
	char const *decalName;
	if ( !pEntity )
	{
		decalName = "Impact.Concrete";
	}
	else
	{
		decalName = pEntity->DamageDecal( iDamageType, iMaterial );
	}

	// See if we need to offset the decal for material type
	return decalsystem->TranslateDecalForGameMaterial( decalName, iMaterial );
}

//==========================================================================================================================
// RAGDOLL ENUMERATOR
//==========================================================================================================================
CRagdollEnumerator::CRagdollEnumerator( Ray_t& shot, float force, int iDamageType )
{
	m_rayShot = shot;
	m_flForce = force;
	m_iDamageType = iDamageType;
	m_bHit = false;
}

IterationRetval_t CRagdollEnumerator::EnumElement( IHandleEntity *pHandleEntity )
{
	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
	if ( pEnt == NULL )
		return ITERATION_CONTINUE;

	C_BaseAnimating *pModel = static_cast< C_BaseAnimating * >( pEnt );

	if ( pModel == NULL )
		return ITERATION_CONTINUE;

	trace_t tr;
	enginetrace->ClipRayToEntity( m_rayShot, MASK_SHOT, pModel, &tr );

	IPhysicsObject	*pPhysicsObject = pModel->VPhysicsGetObject();
	
	if ( pPhysicsObject == NULL )
		return ITERATION_CONTINUE;

	if ( tr.fraction < 1.0 )
	{
		//Send the ragdoll the explosion force
		Vector	dir = m_rayShot.m_Delta;
		VectorNormalize( dir );

		pPhysicsObject->ApplyForceOffset( dir * m_flForce, tr.endpos );	

		/*
		//FIXME: JDW - Can't do this until the decal references are client-side as well
		int decalNumber = decalsystem->GetDecalIndexForName( GetImpactDecal( pModel, &tr, m_iDamageType ) );
		
		if ( pModel != NULL )
		{
			pModel->AddDecal( m_rayShot.m_Start, tr.endpos, tr.endpos, tr.hitbox, decalNumber, true, tr );
		}
		*/
		
		m_bHit = true;

		//FIXME: Yes?  No?
		return ITERATION_STOP;
	}

	return ITERATION_CONTINUE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool AffectRagdolls( Vector vecOrigin, Vector vecStart, int iDamageType )
{
	Ray_t shotRay;
	shotRay.Init( vecStart, vecOrigin );

	CRagdollEnumerator ragdollEnum( shotRay, CLIENT_SHOT_FORCE, iDamageType );
	partition->EnumerateElementsAlongRay( PARTITION_CLIENT_RESPONSIVE_EDICTS, shotRay, false, &ragdollEnum );

	return ragdollEnum.Hit();
}

//------------------------------------------------------------------------------
// Purpose : Create leak effect if material requests it
// Input   :
// Output  :
//------------------------------------------------------------------------------
void LeakEffect( trace_t &tr )
{
	Vector			diffuseColor, baseColor;
	Vector			vTraceDir	= (tr.endpos - tr.startpos);
	VectorNormalize(vTraceDir);
	Vector			vTraceStart = tr.endpos - 0.1*vTraceDir;
	Vector			vTraceEnd	= tr.endpos + 0.1*vTraceDir;
	IMaterial*		pTraceMaterial = engine->TraceLineMaterialAndLighting( vTraceStart, vTraceEnd, diffuseColor, baseColor );

	if (!pTraceMaterial)
		return;

	bool			found;
	IMaterialVar	*pLeakVar = pTraceMaterial->FindVar( "$leakamount", &found, false );
	if( !found )
		return;

	C_Splash* pLeak = new C_Splash();
	if (!pLeak)
		return;

	ClientEntityList().AddNonNetworkableEntity( pLeak->GetIClientUnknown() );

	IMaterialVar*	pLeakColorVar = pTraceMaterial->FindVar( "$leakcolor", &found );
	if (found)
	{
		Vector color;
		pLeakColorVar->GetVecValue(color.Base(),3);
		pLeak->m_vStartColor = pLeak->m_vEndColor = color;
	}

	IMaterialVar*	pLeakNoiseVar = pTraceMaterial->FindVar( "$leaknoise", &found );
	if (found)
	{
		pLeak->m_flNoise = pLeakNoiseVar->GetFloatValue();
	}

	IMaterialVar*	pLeakForceVar = pTraceMaterial->FindVar( "$leakforce", &found );
	if (found)
	{
		float flForce = pLeakForceVar->GetFloatValue();
		pLeak->m_flSpeed		 = flForce;
		pLeak->m_flSpeedRange	 = pLeak->m_flNoise * flForce;
	}

	pLeak->m_flSpawnRate		= pLeakVar->GetFloatValue();;
	pLeak->m_flParticleLifetime = 10;
	pLeak->m_flWidthMin			= 1;
	pLeak->m_flWidthMax			= 5;
	pLeak->SetLocalOrigin( tr.endpos );
	
	QAngle angles;
	VectorAngles( tr.plane.normal, angles );
	pLeak->SetLocalAngles( angles );

	pLeak->Start(&g_ParticleMgr, NULL);
	pLeak->m_flStopEmitTime	= gpGlobals->curtime+5.0;
	pLeak->SetNextClientThink(gpGlobals->curtime+20.0);
}

//-----------------------------------------------------------------------------
// Purpose: Perform custom effects based on the Decal index
//-----------------------------------------------------------------------------
void PerformCustomEffects( Vector &vecOrigin, trace_t &tr, Vector &shotDir, int iMaterial, int iScale )
{
	// Throw out the effect if any of these are true
	if ( tr.surface.flags & (SURF_SKY|SURF_NODRAW|SURF_HINT|SURF_SKIP) )
		return;

	// Cement and wood have dust and flecks
	if ( iMaterial == CHAR_TEX_CONCRETE )
	{
		FX_DebrisFlecks( vecOrigin, &tr, iMaterial, iScale );
	}
	else if ( iMaterial == CHAR_TEX_WOOD )
	{
		FX_DebrisFlecks( vecOrigin, &tr, iMaterial, iScale );
	}
	else if ( iMaterial == CHAR_TEX_DIRT )
	{
		FX_DustImpact( vecOrigin, &tr, iScale );
	}
	else if ( iMaterial == CHAR_TEX_ANTLION )
	{
		FX_AntlionImpact( vecOrigin, &tr );
	}
	else if ( iMaterial == CHAR_TEX_METAL )
	{
		Vector	reflect;
		float	dot = shotDir.Dot( tr.plane.normal );
		reflect = shotDir + ( tr.plane.normal * ( dot*-2.0f ) );

		reflect[0] += random->RandomFloat( -0.2f, 0.2f );
		reflect[1] += random->RandomFloat( -0.2f, 0.2f );
		reflect[2] += random->RandomFloat( -0.2f, 0.2f );

		FX_MetalSpark( vecOrigin, reflect, iScale );
	}
	else if ( iMaterial == CHAR_TEX_COMPUTER )
	{
		Vector	offset = vecOrigin + ( tr.plane.normal * 1.0f );

		g_pEffects->Sparks( offset );
	}

	//---------------------------
	// Do leak effect
	//---------------------------
	LeakEffect(tr);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PlayImpactSound( trace_t	&tr )
{
	surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );

	const char *pbulletImpactSoundName = physprops->GetString( pdata->bulletImpact, rand()%pdata->bulletImpactCount );

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, NULL, pbulletImpactSoundName, &tr.endpos );
}

//-----------------------------------------------------------------------------
// Purpose: Pull the impact data out
// Input  : &data - 
//			*vecOrigin - 
//			*vecAngles - 
//			*iMaterial - 
//			*iDamageType - 
//			*iHitbox - 
//			*iEntIndex - 
//-----------------------------------------------------------------------------
void ParseImpactData( const CEffectData &data, Vector *vecOrigin, Vector *vecStart, Vector *vecShotDir, int *iMaterial, int *iDamageType, int *iHitbox, int *iEntIndex )
{
	*vecOrigin = data.m_vOrigin;
	*vecStart = data.m_vStart;
	*iMaterial = data.m_nMaterial;
	*iDamageType = data.m_nDamageType;
	*iHitbox = data.m_nHitBox;
	*iEntIndex = data.m_nEntIndex;
	*vecShotDir = (*vecOrigin - *vecStart);
	VectorNormalize( *vecShotDir );
}

