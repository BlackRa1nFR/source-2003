//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
 
#include <stdarg.h>
#include "dt_send.h"
#include "dt.h"
#include "dt_recv.h"
#include "dt_encode.h"
#include "convar.h"
#include "commonmacros.h"
#include "vstdlib/strtools.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define MAX_TOTAL_SENDTABLE_PROPS	1024
#define PROPINDEX_NUMBITS 10


ConVar g_CV_DTWatchEnt( "dtwatchent", "-1", 0 );
ConVar g_CV_DTWatchVar( "dtwatchvar", "", 0 );
ConVar g_CV_DTWarning( "dtwarning", "0", 0 );



// ----------------------------------------------------------------------------- //
//
// CBuildHierarchyStruct
//
// Used while building a CSendNode hierarchy.
//
// ----------------------------------------------------------------------------- //
class CBuildHierarchyStruct
{
public:
	const ExcludeProp	*m_pExcludeProps;
	int					m_nExcludeProps;

	const SendProp		*m_pDatatableProps[MAX_TOTAL_SENDTABLE_PROPS];
	int					m_nDatatableProps;
	
	const SendProp		*m_pProps[MAX_TOTAL_SENDTABLE_PROPS];
	unsigned char		m_PropProxyIndices[MAX_TOTAL_SENDTABLE_PROPS];
	int					m_nProps;

	unsigned char m_nPropProxies;
};


// ------------------------------------------------------------------------------------ //
// CDeltaBitsWriter.
// ------------------------------------------------------------------------------------ //

// OLD.. THIS CAN BE REMOVED ANYTIME.
// #define TRACK_PROP_DELTA_BITS
#if defined( TRACK_PROP_DELTA_BITS )
	bool g_bTrackProps = false;

	int g_nProps = 0;
	int g_TotalPropDiff = 0;
	int g_MaxPropDiff = 0;
	int g_MinPropDiff = 10000;

	int g_nTotalDeltaBits = 0;

	class CChecker
	{
	public:
		~CChecker()
		{
			float avgDiff = (float)g_TotalPropDiff / g_nProps;
			float avgBits = (float)g_nTotalDeltaBits / g_nProps;
		}
	} asdf;
#endif


CDeltaBitsWriter::CDeltaBitsWriter( bf_write *pBuf )
{
	m_pBuf = pBuf;
	m_iLastProp = -1;
}


CDeltaBitsWriter::~CDeltaBitsWriter()
{
	// Write a final zero bit, signifying that there are no more properties in the buffer.
	m_pBuf->WriteOneBit( 0 );
}


int CDeltaBitsWriter::WritePropIndex( int iProp )
{
	int diff = iProp - m_iLastProp;
	m_iLastProp = iProp;

	m_pBuf->WriteOneBit( 1 );
	int nBitsToEncode = 1;

	--diff; // It's always at least 1 so subtract 1.
	int iIteration=1;
	bool bForceContinue;
	do
	{
		int maxVal = (1 << iIteration)-1;
		int val;
		if ( diff >= maxVal )
		{
			val = maxVal;
			bForceContinue = true; // must go through once more, even if the next value is a zero.
		}
		else
		{
			bForceContinue = false;
			val = diff;
		}

		m_pBuf->WriteUBitLong( val, iIteration );
		nBitsToEncode += iIteration;

		diff -= val;
		++iIteration;
	} while ( diff || bForceContinue );

#if defined( TRACK_PROP_DELTA_BITS )
	if ( g_bTrackProps )
	{
		g_TotalPropDiff += diff;
		++g_nProps;
	
		if ( diff > g_MaxPropDiff )
			g_MaxPropDiff = diff;
		
		if ( diff < g_MinPropDiff )
			g_MinPropDiff = diff;
	}
	g_nTotalDeltaBits += nBitsToEncode;
#endif

	return nBitsToEncode;
}


// ------------------------------------------------------------------------------------ //
// CDeltaBitsReader.
// ------------------------------------------------------------------------------------ //

CDeltaBitsReader::CDeltaBitsReader( bf_read *pBuf )
{
	m_pBuf = pBuf;
	m_bFinished = false;
	m_iLastProp = -1;
}


CDeltaBitsReader::~CDeltaBitsReader()
{
	// Make sure they read to the end unless they specifically said they don't care.
	if ( m_pBuf )
	{
		Assert( m_bFinished );
	}
}


bool CDeltaBitsReader::ReadNextPropIndex( int *iProp )
{
	Assert( !m_bFinished );

	if ( m_pBuf->ReadOneBit() )
	{
		*iProp = m_iLastProp + 1;
		int iIteration = 1;
		while ( 1 )
		{
			int val = m_pBuf->ReadUBitLong( iIteration );
			*iProp += val;

			if ( val == (1 << iIteration)-1 )
			{
				// Ok, there's more.
				++iIteration;
			}
			else
			{
				// No more.. break out.
				break;
			}
		}

		m_iLastProp = *iProp;
		return true;
	}
	else
	{
		m_bFinished = true;
		return false;
	}
}


void CDeltaBitsReader::ForceFinished()
{
	m_bFinished = true;
	m_pBuf = NULL;
}


// ----------------------------------------------------------------------------- //
// CSendNode.
// ----------------------------------------------------------------------------- //

CSendNode::CSendNode()
{
	m_iDatatableProp = -1;
	m_pTable = NULL;
	
	m_iFirstRecursiveProp = m_nRecursiveProps = 0;
	m_iFirstProp = m_nProps = 0;
}

CSendNode::~CSendNode()
{
	int c = GetNumChildren();
	for ( int i = c - 1 ; i >= 0 ; i-- )
	{
		delete GetChild( i );
	}
	m_Children.Purge();
}

// ----------------------------------------------------------------------------- //
// CSendTablePrecalc
// ----------------------------------------------------------------------------- //

CSendTablePrecalc::CSendTablePrecalc()
{
	m_pDTITable = NULL;
	m_pSendTable = 0;
}


CSendTablePrecalc::~CSendTablePrecalc()
{
	if ( m_pSendTable )
		m_pSendTable->m_pPrecalc = 0;
}


const ExcludeProp* FindExcludeProp(
	char const *pTableName,
	char const *pPropName,
	const ExcludeProp *pExcludeProps, 
	int nExcludeProps)
{
	for ( int i=0; i < nExcludeProps; i++ )
	{
		if ( stricmp(pExcludeProps[i].m_pTableName, pTableName) == 0 && stricmp(pExcludeProps[i].m_pPropName, pPropName ) == 0 )
			return &pExcludeProps[i];
	}

	return NULL;
}


// Fill in a list of all the excluded props.
static bool SendTable_GetPropsExcluded( const SendTable *pTable, ExcludeProp *pExcludeProps, int &nExcludeProps, int nMaxExcludeProps )
{
	for(int i=0; i < pTable->m_nProps; i++)
	{
		SendProp *pProp = &pTable->m_pProps[i];

		if ( pProp->IsExcludeProp() )
		{
			char const *pName = pProp->GetExcludeDTName();

			ErrorIfNot( pName,
				("Found an exclude prop missing a name.")
			);
			
			ErrorIfNot( nExcludeProps < nMaxExcludeProps,
				("SendTable_GetPropsExcluded: Overflowed max exclude props with %s.", pName)
			);

			pExcludeProps[nExcludeProps].m_pTableName = pName;
			pExcludeProps[nExcludeProps].m_pPropName = pProp->GetName();
			nExcludeProps++;
		}
		else if ( pProp->GetDataTable() )
		{
			if( !SendTable_GetPropsExcluded( pProp->GetDataTable(), pExcludeProps, nExcludeProps, nMaxExcludeProps ) )
				return false;
		}
	}

	return true;
}


// Set the datatable proxy indices in all datatable SendProps.
static void SetDataTableProxyIndices_R( SendTable *pMainTable, SendTable *pCurTable )
{
	for ( int i=0; i < pCurTable->GetNumProps(); i++ )
	{
		SendProp *pProp = pCurTable->GetProp( i );
	
		if ( pProp->GetType() != DPT_DataTable )
			continue;

		if ( pProp->GetFlags() & SPROP_PROXY_ALWAYS_YES )
		{
			pProp->SetDataTableProxyIndex( DATATABLE_PROXY_INDEX_NOPROXY );
		}
		else
		{
			pProp->SetDataTableProxyIndex( pMainTable->GetNumDataTableProxies() );
			pMainTable->SetNumDataTableProxies( pMainTable->GetNumDataTableProxies() + 1 );
		}

		SetDataTableProxyIndices_R( pMainTable, pProp->GetDataTable() );
	}
}


void SendTable_BuildHierarchy( 
	CSendNode *pNode,
	const SendTable *pTable, 
	CBuildHierarchyStruct *bhs
	 )
{
	pNode->m_pTable = pTable;
	pNode->m_iFirstRecursiveProp = bhs->m_nProps;
	
	Assert( bhs->m_nPropProxies < 255 );
	unsigned char curPropProxy = bhs->m_nPropProxies;
	++bhs->m_nPropProxies;

	const SendProp *pNonDatatableProps[MAX_TOTAL_SENDTABLE_PROPS];
	int nNonDatatableProps = 0;
	
	// First add all the child datatables.
	int i;
	for ( i=0; i < pTable->m_nProps; i++ )
	{
		const SendProp *pProp = &pTable->m_pProps[i];

		if ( pProp->IsExcludeProp() || 
			pProp->IsInsideArray() || 
			FindExcludeProp( pTable->GetName(), pProp->GetName(), bhs->m_pExcludeProps, bhs->m_nExcludeProps ) )
		{
			continue;
		}

		if ( pProp->GetType() == DPT_DataTable )
		{
			// Setup a child datatable reference.
			CSendNode *pChild = new CSendNode;

			// Setup a datatable prop for this node to reference (so the recursion
			// routines can get at the proxy).
			ErrorIfNot( bhs->m_nDatatableProps < ARRAYSIZE( bhs->m_pDatatableProps ), 
				("Overflowed datatable prop list in SendTable '%s'.", pTable->GetName()) 
			);
			
			bhs->m_pDatatableProps[bhs->m_nDatatableProps] = pProp;
			pChild->m_iDatatableProp = bhs->m_nDatatableProps;
			++bhs->m_nDatatableProps;

			pNode->m_Children.AddToTail( pChild );

			// Recurse into the new child datatable.
			SendTable_BuildHierarchy( pChild, pProp->GetDataTable(), bhs );
		}
		else
		{
			ErrorIfNot( nNonDatatableProps < ARRAYSIZE( pNonDatatableProps ),
				("SendTable_BuildHierarchy: overflowed non-datatable props with '%s'.", pProp->GetName())
			);
			
			pNonDatatableProps[nNonDatatableProps] = pProp;
			++nNonDatatableProps;
		}
	}

	
	// Now add the properties.
	pNode->m_iFirstProp = bhs->m_nProps;

	// Make sure there's room, then just copy the pointers from the loop above.
	ErrorIfNot( bhs->m_nProps + nNonDatatableProps < ARRAYSIZE( bhs->m_pProps ),
		("SendTable_BuildHierarchy: overflowed prop buffer.")
	);
	
	for ( i=0; i < nNonDatatableProps; i++ )
	{
		bhs->m_pProps[bhs->m_nProps] = pNonDatatableProps[i];
		bhs->m_PropProxyIndices[bhs->m_nProps] = curPropProxy;
		++bhs->m_nProps;
	}

	pNode->m_nProps = nNonDatatableProps;
	pNode->m_nRecursiveProps = bhs->m_nProps - pNode->m_iFirstRecursiveProp;
}

bool CSendTablePrecalc::SetupFlatPropertyArray()
{
	SendTable *pTable = GetSendTable();

	// First go through and set SPROP_INSIDEARRAY when appropriate, and set array prop pointers.
	SetupArrayProps_R<SendTable, SendTable::PropType>( pTable );

	// Make a list of which properties are excluded.
	ExcludeProp excludeProps[MAX_EXCLUDE_PROPS];
	int nExcludeProps = 0;
	if( !SendTable_GetPropsExcluded( pTable, excludeProps, nExcludeProps, MAX_EXCLUDE_PROPS ) )
		return false;

	// Now build the hierarchy.
	CBuildHierarchyStruct bhs;
	bhs.m_pExcludeProps = excludeProps;
	bhs.m_nExcludeProps = nExcludeProps;
	bhs.m_nProps = bhs.m_nDatatableProps = 0;
	bhs.m_nPropProxies = 0;
	SendTable_BuildHierarchy( GetRootNode(), pTable, &bhs );
	
	// Copy the SendProp pointers into the precalc.	
	m_Props.CopyArray( bhs.m_pProps, bhs.m_nProps );
	m_DatatableProps.CopyArray( bhs.m_pDatatableProps, bhs.m_nDatatableProps );
	m_PropProxyIndices.CopyArray( bhs.m_PropProxyIndices, bhs.m_nProps );

	// Assign the datatable proxy indices.
	pTable->SetNumDataTableProxies( 0 );
	SetDataTableProxyIndices_R( pTable, pTable );

	return true;
}


// ---------------------------------------------------------------------------------------- //
// Helpers.
// ---------------------------------------------------------------------------------------- //

// Compares two arrays of bits.
// Returns true if they are equal.
bool AreBitArraysEqual(
	void const *pvBits1,
	void const *pvBits2,
	int nBits ) 
{
	int i, nBytes, bit1, bit2;

	unsigned char const *pBits1 = (unsigned char*)pvBits1;
	unsigned char const *pBits2 = (unsigned char*)pvBits2;

	// Compare bytes.
	nBytes = nBits >> 3;
	if( memcmp( pBits1, pBits2, nBytes ) != 0 )
		return false;

	// Compare remaining bits.
	for(i=nBytes << 3; i < nBits; i++)
	{
		bit1 = pBits1[i >> 3] & (1 << (i & 7));
		bit2 = pBits2[i >> 3] & (1 << (i & 7));
		if(bit1 != bit2)
			return false;
	}

	return true;
}


// Does a fast memcmp-based test to determine if the two bit arrays are different.
// Returns true if they are equal.
bool CompareBitArrays(
	void const *pPacked1,
	void const *pPacked2,
	int nBits1,
	int nBits2
	)
{
	if( nBits1 >= 0 && nBits1 == nBits2 )
		return AreBitArraysEqual( pPacked1, pPacked2, nBits1 );
	else
		return false;
}

// Looks at the DTWatchEnt and DTWatchProp console variables and returns true
// if the user wants to watch this property.
bool ShouldWatchThisProp(int objectID, const char *pPropName)
{
	if(g_CV_DTWatchEnt.GetInt() != -1 &&
		g_CV_DTWatchEnt.GetInt() == objectID)
	{
		const char *pStr = g_CV_DTWatchVar.GetString();
		if ( pStr && pStr[0] != 0 )
		{
			return stricmp( pStr, pPropName ) == 0;
		}
		else
		{
			return true;
		}
	}

	return false;
}


// Prints a datatable warning into the console.
void DataTable_Warning( const char *pInMessage, ... )
{
	char msg[4096];
	va_list marker;
	
#if 0
	#if !defined(_DEBUG)
		if(!g_CV_DTWarning.GetInt())
			return;
	#endif
#endif

	va_start(marker, pInMessage);
	Q_vsnprintf( msg, sizeof( msg ), pInMessage, marker);
	va_end(marker);

	Warning( "DataTable warning: %s", msg );
}







