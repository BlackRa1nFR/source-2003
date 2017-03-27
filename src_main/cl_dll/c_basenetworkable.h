//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef C_BASENETWORKABLE_H
#define C_BASENETWORKABLE_H
#ifdef _WIN32
#pragma once
#endif


#include "iclientnetworkable.h"
#include "client_class.h"
#include "cliententitylist.h"
#include "iclientthinkable.h"


class C_BaseNetworkable : public IClientUnknown, public IClientNetworkable, public IClientThinkable
{
public:
	DECLARE_CLASS_NOBASE( C_BaseNetworkable );
	DECLARE_CLIENTCLASS();

									C_BaseNetworkable();
	virtual							~C_BaseNetworkable();

	// Called by the CLIENTCLASS macros.
	void							Init( int iEntity, int iSerialNum );
	void							Term();

	// This just picks one of the routes to IClientUnknown.
	IClientUnknown*					GetIClientUnknown();

	ClientEntityHandle_t			GetClientEntityHandle();


// IHandleEntity.
public:

	virtual void SetRefEHandle( const CBaseHandle &handle );
	virtual const CBaseHandle& GetRefEHandle() const;


// IClientUnknown implementation.
public:
		
	virtual ICollideable*		GetClientCollideable()	{ return 0; }
	virtual IClientNetworkable*	GetClientNetworkable()	{ return this; }
	virtual IClientRenderable*	GetClientRenderable()	{ return 0; }
	virtual IClientEntity*		GetIClientEntity()		{ return 0; }
	virtual C_BaseEntity*		GetBaseEntity()			{ return 0; }
	virtual IClientThinkable*	GetClientThinkable()	{ return this; }


// IClientThinkable.
public:

	virtual void				ClientThink();
	virtual ClientThinkHandle_t	GetThinkHandle();
	virtual void				SetThinkHandle( ClientThinkHandle_t hThink );


// IClientNetworkable implementation.
public:

	virtual void					Release();
	virtual void					NotifyShouldTransmit( ShouldTransmitState_t state );
	virtual void					PreDataUpdate( DataUpdateType_t updateType );
	virtual void					PostDataUpdate( DataUpdateType_t updateType );
	virtual void					OnPreDataChanged( DataUpdateType_t updateType );
	virtual void					OnDataChanged( DataUpdateType_t updateType );
	virtual bool					IsDormant( void );
	virtual int						entindex( void );
	virtual void					ReceiveMessage( const char *msgname, int length, void *data );
	virtual void*					GetDataTableBasePtr();


private:

	void							SetDormant( bool bDormant );

	// The list that holds OnDataChanged events uses this to make sure we don't get multiple
	// OnDataChanged calls in the same frame if the client receives multiple packets.
	int								m_DataChangeEventRef;

	bool							m_bDormant;
	
	// For the client entity list.
	unsigned short					index; 
	ClientEntityHandle_t			m_ClientHandle;

	ClientThinkHandle_t				m_hThink;
};


// --------------------------------------------------------------------------- //
// Inlines.
// --------------------------------------------------------------------------- //

inline IClientUnknown* C_BaseNetworkable::GetIClientUnknown()
{ 
	return this; 
}


inline ClientEntityHandle_t C_BaseNetworkable::GetClientEntityHandle()
{
	return m_ClientHandle;
}


#endif // C_BASENETWORKABLE_H
