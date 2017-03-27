//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "quakedef.h"
#include "cl_localnetworkbackdoor.h"
#include "icliententitylist.h"
#include "LocalNetworkBackdoor.h"
#include "iclientnetworkable.h"
#include "cl_ents_parse.h"
#include "basehandle.h"
#include "client_class.h"
#include "dt_localtransfer.h"


class CLocalNetworkBackdoor : public ILocalNetworkBackdoor
{
public:
	virtual void StartEntityStateUpdate()
	{
		memset( m_EntsAlive, 0, sizeof( m_EntsAlive ) );
		memset( m_EntsCreated, 0, sizeof( m_EntsCreated ) );
		memset( m_EntsChanged, 0, sizeof( m_EntsChanged ) );
		memset( m_EntsDormant, 0, sizeof( m_EntsDormant ) );
	}

	virtual void EndEntityStateUpdate()
	{
		// Now make sure we delete all entities the server did not update.
		int highest_index = entitylist->GetHighestEntityIndex();
		for ( int i = 0; i <= highest_index; i++ )
		{
			if ( m_EntsAlive[i>>5] & (1 << (i & 31)) )
			{
				IClientNetworkable *pNet = entitylist->GetClientNetworkable( i );
				Assert( pNet );
				if ( m_EntsCreated[i>>5] & (1 << (i & 31)) )
				{
					pNet->PostDataUpdate( DATA_UPDATE_CREATED );
					pNet->NotifyShouldTransmit( SHOULDTRANSMIT_START );
				}
				else
				{
					if ( m_EntsChanged[i>>5] & (1 << (i & 31)) )
						pNet->PostDataUpdate( DATA_UPDATE_DATATABLE_CHANGED );
					
					if ( m_EntsDormant[i>>5] & (1 << (i & 31)) )
					{
					}
					else
					{
						if ( pNet->IsDormant() )
							pNet->NotifyShouldTransmit( SHOULDTRANSMIT_START );
						else
							pNet->NotifyShouldTransmit( SHOULDTRANSMIT_STAY );
					}
				}
			}
			else
			{
				CL_DeleteDLLEntity( i, "EndEntityStateUpdate" );
			}
		}
	}

	virtual void EntityDormant( int iEnt, int iSerialNum )
	{
		m_EntsDormant[iEnt>>5] |= (1 << (iEnt & 31));

		IClientNetworkable *pNet = entitylist->GetClientNetworkable( iEnt );
		if ( pNet )
		{
			if ( iSerialNum == pNet->GetIClientUnknown()->GetRefEHandle().GetSerialNumber() )
			{
				m_EntsAlive[iEnt>>5] |= (1 << (iEnt & 31));

				// Tell the game code that this guy is now dormant.
				if ( !pNet->IsDormant() )
					pNet->NotifyShouldTransmit( SHOULDTRANSMIT_END );
			}
			else
			{
				pNet->Release();
			}
		}
	}

	virtual void EntState( 
		int iEnt, 
		int iSerialNum, 
		int iClass, 
		const SendTable *pSendTable, 
		const void *pSourceEnt,
		bool bChanged )
	{
		// Remember that this ent is alive.
		m_EntsAlive[iEnt>>5] |= (1 << (iEnt & 31));

		ClientClass *pClientClass = cl.m_pServerClasses[iClass].m_pClientClass;
		if ( !pClientClass )
			Error( "CLocalNetworkBackdoor::EntState - missing client class %d", iClass );

		// Do we have an entity here already?
		bool bExistedAndWasDormant = false;
		IClientNetworkable *pNet = entitylist->GetClientNetworkable( iEnt );
		if ( pNet )
		{
			// If the serial numbers are different, make it recreate the ent.
			if ( iSerialNum == pNet->GetIClientUnknown()->GetRefEHandle().GetSerialNumber() )
			{
				bExistedAndWasDormant = pNet->IsDormant();
			}
			else
			{
				pNet->Release();
				pNet = NULL;
			}
		}

		// Create the entity?
		DataUpdateType_t updateType;
		if ( pNet )
		{
			updateType = DATA_UPDATE_DATATABLE_CHANGED;
		}
		else		
		{
			updateType = DATA_UPDATE_CREATED;
			pNet = pClientClass->m_pCreateFn( iEnt, iSerialNum );
			m_EntsCreated[iEnt>>5] |= (1 << (iEnt & 31));
		}

		// Now decode the entity.
		if ( bExistedAndWasDormant || bChanged || ( updateType == DATA_UPDATE_CREATED ) )
		{
			pNet->PreDataUpdate( updateType );
			LocalTransfer_TransferEntity( pSendTable, pSourceEnt, pClientClass->m_pRecvTable, pNet->GetDataTableBasePtr(), iEnt );
			m_EntsChanged[iEnt>>5] |= (1 << (iEnt & 31));

			if ( bExistedAndWasDormant )
			{
				// Set this so we use DATA_UPDATE_CREATED.
				m_EntsCreated[iEnt>>5] |= (1 << (iEnt & 31));
			}
		}
	}


private:
	unsigned long m_EntsAlive[MAX_EDICTS / 32 + 1];
	unsigned long m_EntsCreated[MAX_EDICTS / 32 + 1];
	unsigned long m_EntsChanged[MAX_EDICTS / 32 + 1];
	unsigned long m_EntsDormant[MAX_EDICTS / 32 + 1];
};

CLocalNetworkBackdoor g_LocalNetworkBackdoor;


void CL_SetupLocalNetworkBackDoor( bool bUseBackDoor )
{
	if ( bUseBackDoor )
	{
		g_pLocalNetworkBackdoor = &g_LocalNetworkBackdoor;
	}
	else
	{
		g_pLocalNetworkBackdoor = NULL;
	}
}


