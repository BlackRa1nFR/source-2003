//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basenetworkable.h"



class CTest_ProxyToggle_Networkable;
static CTest_ProxyToggle_Networkable *g_pTestObj = 0;
static bool g_bEnableProxy = true;


// ---------------------------------------------------------------------------------------- //
// CTest_ProxyToggle_Networkable
// ---------------------------------------------------------------------------------------- //

class CTest_ProxyToggle_Networkable : public CBaseNetworkable
{
public:
	DECLARE_CLASS( CTest_ProxyToggle_Networkable, CBaseNetworkable );
	DECLARE_SERVERCLASS();

			CTest_ProxyToggle_Networkable()
			{
				m_WithProxy = 1241;
				g_pTestObj = this;
			}

			~CTest_ProxyToggle_Networkable()
			{
				g_pTestObj = NULL;
			}

	bool	ShouldTransmit( 
		const edict_t *recipient, 
		const void *pvs, 
		int clientArea
		)
	{
		return true;
	}

	CNetworkVar( int, m_WithProxy );
};

void* SendProxy_TestProxyToggle( const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	if ( g_bEnableProxy )
	{
		return (void*)pData;
	}
	else
	{
		pRecipients->ClearAllRecipients();
		return NULL;
	}
}


// ---------------------------------------------------------------------------------------- //
// Datatables.
// ---------------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( test_proxytoggle, CTest_ProxyToggle_Networkable );

BEGIN_SEND_TABLE_NOBASE( CTest_ProxyToggle_Networkable, DT_ProxyToggle_ProxiedData )
	SendPropInt( SENDINFO( m_WithProxy ) )
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST( CTest_ProxyToggle_Networkable, DT_ProxyToggle )
	SendPropDataTable( "blah", 0, &REFERENCE_SEND_TABLE( DT_ProxyToggle_ProxiedData ), SendProxy_TestProxyToggle )
END_SEND_TABLE()



// ---------------------------------------------------------------------------------------- //
// Console commands for this test.
// ---------------------------------------------------------------------------------------- //

void Test_ProxyToggle_EnableProxy()
{
	if ( engine->Cmd_Argc() < 2 )
	{
		Error( "Test_ProxyToggle_EnableProxy: requires parameter (0 or 1)." );
	}

	g_bEnableProxy = !!atoi( engine->Cmd_Argv( 1 ) );
}

void Test_ProxyToggle_SetValue()
{
	if ( engine->Cmd_Argc() < 2 )
	{
		Error( "Test_ProxyToggle_SetValue: requires value parameter." );
	}
	else if ( !g_pTestObj )
	{
		Error( "Test_ProxyToggle_SetValue: no entity present." );
	}

	g_pTestObj->m_WithProxy = atoi( engine->Cmd_Argv( 1 ) );
}

ConCommand cc_Test_ProxyToggle_EnableProxy( "Test_ProxyToggle_EnableProxy", Test_ProxyToggle_EnableProxy, 0, FCVAR_CHEAT );
ConCommand cc_Test_ProxyToggle_SetValue( "Test_ProxyToggle_SetValue", Test_ProxyToggle_SetValue, 0, FCVAR_CHEAT );


