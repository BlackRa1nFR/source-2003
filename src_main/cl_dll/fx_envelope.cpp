//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "fx_envelope.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_EnvelopeFX::C_EnvelopeFX( void )
{
	m_active = false;
	m_handle = INVALID_CLIENT_RENDER_HANDLE;
	m_active = false;
}

C_EnvelopeFX::~C_EnvelopeFX()
{
	RemoveRenderable();
}


void C_EnvelopeFX::RemoveRenderable()
{
	if ( m_handle != INVALID_CLIENT_RENDER_HANDLE )
	{
		ClientLeafSystem()->RemoveRenderable( m_handle );
		m_handle = INVALID_CLIENT_RENDER_HANDLE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the envelope being in the client's known entity list
//-----------------------------------------------------------------------------
void C_EnvelopeFX::Update( void )
{
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
	else
	{
		RemoveRenderable();
	}
};

//-----------------------------------------------------------------------------
// Purpose: Starts up the effect
// Input  : entityIndex - entity to be attached to
//			attachment - attachment point (if any) to be attached to
//-----------------------------------------------------------------------------
void C_EnvelopeFX::EffectInit( int entityIndex, int attachment )
{
	m_entityIndex = entityIndex;
	m_attachment = attachment;

	m_active = 1;
	m_t = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down the effect
//-----------------------------------------------------------------------------
void C_EnvelopeFX::EffectShutdown( void ) 
{
	m_active = 0;
	m_t = 0;
}
