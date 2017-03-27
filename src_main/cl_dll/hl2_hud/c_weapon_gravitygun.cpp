//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "keydefs.h"
#include "hud.h"
#include "in_buttons.h"
#include "beamdraw.h"
#include "c_weapon__stubs.h"

class C_BeamQuadratic : public CDefaultClientRenderable
{
public:
	C_BeamQuadratic();
	void			Update( C_BaseEntity *pOwner );

	// IClientRenderable
	virtual const Vector&			GetRenderOrigin( void ) { return m_worldPosition; }
	virtual const QAngle&			GetRenderAngles( void ) { return vec3_angle; }
	virtual bool					ShouldDraw( void ) { return true; }
	virtual bool					IsTransparent( void ) { return true; }
	virtual bool					ShouldCacheRenderInfo() { return false;}
	virtual bool					ShouldReceiveShadows() { return false; }
	virtual int						DrawModel( int flags );

	// Returns the bounds relative to the origin (render bounds)
	virtual void	GetRenderBounds( Vector& mins, Vector& maxs )
	{
		// bogus.  But it should draw if you can see the end point
		mins.Init(-32,-32,-32);
		maxs.Init(32,32,32);
	}

	C_BaseEntity			*m_pOwner;
	Vector					m_targetPosition;
	Vector					m_worldPosition;
	ClientRenderHandle_t	m_handle;
	int						m_active;
	int						m_glueTouching;
	int						m_viewModelIndex;
};


class C_WeaponGravityGun : public C_BaseCombatWeapon
{
	DECLARE_CLASS( C_WeaponGravityGun, C_BaseCombatWeapon );
public:
	C_WeaponGravityGun() {}

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	int KeyInput( int down, int keynum, const char *pszCurrentBinding )
	{
		if ( gHUD.m_iKeyBits & IN_ATTACK )
		{
			switch ( keynum )
			{
			case K_MWHEELUP:
				gHUD.m_iKeyBits |= IN_WEAPON1;
				return 0;

			case K_MWHEELDOWN:
				gHUD.m_iKeyBits |= IN_WEAPON2;
				return 0;
			}
		}

		// Allow engine to process
		return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
	}

	void OnDataChanged( DataUpdateType_t updateType )
	{
		BaseClass::OnDataChanged( updateType );
		m_beam.Update( this );
	}

private:
	C_WeaponGravityGun( const C_WeaponGravityGun & );

	C_BeamQuadratic	m_beam;
};

STUB_WEAPON_CLASS_IMPLEMENT( weapon_physgun, C_WeaponGravityGun );

IMPLEMENT_CLIENTCLASS_DT( C_WeaponGravityGun, DT_WeaponGravityGun, CWeaponGravityGun )
	RecvPropVector( RECVINFO_NAME(m_beam.m_targetPosition,m_targetPosition) ),
	RecvPropVector( RECVINFO_NAME(m_beam.m_worldPosition, m_worldPosition) ),
	RecvPropInt( RECVINFO_NAME(m_beam.m_active, m_active) ),
	RecvPropInt( RECVINFO_NAME(m_beam.m_glueTouching, m_glueTouching) ),
	RecvPropInt( RECVINFO_NAME(m_beam.m_viewModelIndex, m_viewModelIndex) ),
END_RECV_TABLE()


C_BeamQuadratic::C_BeamQuadratic()
{
	m_pOwner = NULL;
	m_handle = INVALID_CLIENT_RENDER_HANDLE;
}

void C_BeamQuadratic::Update( C_BaseEntity *pOwner )
{
	m_pOwner = pOwner;
	if ( m_active )
	{
		if ( m_handle == INVALID_CLIENT_RENDER_HANDLE )
		{
			m_handle = ClientLeafSystem()->AddRenderable( this, RENDER_GROUP_TRANSLUCENT_ENTITY );
		}
		else
		{
			ClientLeafSystem()->RenderableMoved( m_handle );
		}
	}
	else if ( !m_active && m_handle != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->RemoveRenderable( m_handle );
		m_handle = INVALID_CLIENT_RENDER_HANDLE;
	}
}


int	C_BeamQuadratic::DrawModel( int )
{
	Vector points[3];
	QAngle tmpAngle;

	if ( !m_active )
		return 0;

	C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_viewModelIndex );
	if ( !pEnt )
		return 0;
	pEnt->GetAttachment( 1, points[0], tmpAngle );

	points[1] = 0.5 * (m_targetPosition + points[0]);
	
	// a little noise 11t & 13t should be somewhat non-periodic looking
	//points[1].z += 4*sin( gpGlobals->curtime*11 ) + 5*cos( gpGlobals->curtime*13 );
	points[2] = m_worldPosition;

	int subdivisions = 16;
	IMaterial *pMat = materials->FindMaterial( "sprites/physbeam", 0, 0 );

	CBeamSegDraw beamDraw;
	beamDraw.Start( subdivisions, pMat );

	CBeamSeg seg;
	seg.m_flAlpha = 1.0;
	seg.m_flWidth = 13;
	if ( m_glueTouching )
	{
		seg.m_vColor = Vector(1,0,0);
	}
	else
	{
		seg.m_vColor = Vector(1,1,1);
	}

	
	float t = 0;
	float u = gpGlobals->curtime - (int)gpGlobals->curtime;
	float dt = 1.0 / (float)subdivisions;
	for( int i = 0; i <= subdivisions; i++, t += dt )
	{
		float omt = (1-t);
		float p0 = omt*omt;
		float p1 = 2*t*omt;
		float p2 = t*t;

		seg.m_vPos = p0 * points[0] + p1 * points[1] + p2 * points[2];
		seg.m_flTexCoord = u - t;
		if ( i == subdivisions )
		{
			// HACK: fade out the end a bit
			seg.m_vColor = vec3_origin;
		}
		beamDraw.NextSeg( &seg );
	}

	beamDraw.End();
	return 1;
}

/*
P0 = start
P1 = control
P2 = end
P(t) = (1-t)^2 * P0 + 2t(1-t)*P1 + t^2 * P2
*/
