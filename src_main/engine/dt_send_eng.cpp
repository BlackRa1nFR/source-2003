//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "dt_send_eng.h"
#include "dt_encode.h"
#include "dt_instrumentation_server.h"
#include "commonmacros.h"
#include "dt_stack.h"
#include "tier0/dbg.h"
#include "tier0/vprof.h"
#include "checksum_crc.h"
#include "sv_packedentities.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern int host_framecount;
void Con_DPrintf (const char *fmt, ...);

class CSendTablePrecalc;
class CSendNode;



// This stack doesn't actually call any proxies. It uses the CSendProxyRecipients to tell
// what can be sent to the specified client.
class CPropCullStack : public CDatatableStack
{
public:
						CPropCullStack( 
							CSendTablePrecalc *pPrecalc, 
							int iClient, 
							const CSendProxyRecipients *pOldStateProxies,
							const int nOldStateProxies, 
							const CSendProxyRecipients *pNewStateProxies,
							const int nNewStateProxies
							) :
							
							CDatatableStack( pPrecalc, (unsigned char*)1, -1 ),
							m_pOldStateProxies( pOldStateProxies ),
							m_nOldStateProxies( nOldStateProxies ),
							m_pNewStateProxies( pNewStateProxies ),
							m_nNewStateProxies( nNewStateProxies )
						{
							m_pPrecalc = pPrecalc;
							m_iClient = iClient;
						}

	virtual unsigned char*	CallPropProxy( CSendNode *pNode, int iProp, unsigned char *pStructBase )
	{
		const SendProp *pProp = m_pPrecalc->GetDatatableProp( iProp );

		if ( pProp->GetDataTableProxyIndex() == DATATABLE_PROXY_INDEX_NOPROXY )
		{
			return (unsigned char*)1;
		}
		else
		{
			Assert( pProp->GetDataTableProxyIndex() < m_nNewStateProxies );
			bool bCur = m_pNewStateProxies[ pProp->GetDataTableProxyIndex() ].m_Bits.Get( m_iClient ) != 0;

			if ( m_pOldStateProxies )
			{
				Assert( pProp->GetDataTableProxyIndex() < m_nOldStateProxies );
				bool bPrev = m_pOldStateProxies[ pProp->GetDataTableProxyIndex() ].m_Bits.Get( m_iClient ) != 0;
				if ( bPrev != bCur )
				{
					if ( bPrev )
					{
						// Old state had the data and the new state doesn't.
						return 0;
					}
					else
					{
						// Add ALL properties under this proxy because the previous state didn't have any of them.
						for ( int i=0; i < pNode->m_nRecursiveProps; i++ )
						{
							if ( m_nNewProxyProps < ARRAYSIZE( m_NewProxyProps ) )
							{
								m_NewProxyProps[m_nNewProxyProps] = pNode->m_iFirstRecursiveProp + i;
							}
							else
							{
								Error( "CPropCullStack::CallPropProxy - overflowed m_nNewProxyProps" );
							}

							++m_nNewProxyProps;
						}

						// Tell the outer loop that writes to m_pOutProps not to write anything from this
						// proxy since we just did.
						return 0;
					}
				}
			}

			return (unsigned char*)bCur;
		}
	}

	
	inline void AddProp( int iProp )
	{
		if ( m_nOutProps < m_nMaxOutProps )
		{
			m_pOutProps[m_nOutProps] = iProp;
		}
		else
		{
			Error( "CPropCullStack::AddProp - m_pOutProps overflowed" );
		}

		++m_nOutProps;
	}


	void CullPropsFromProxies( const int *pStartProps, int nStartProps, int *pOutProps, int nMaxOutProps )
	{
		m_nOutProps = 0;
		m_pOutProps = pOutProps;
		m_nMaxOutProps = nMaxOutProps;
		m_nNewProxyProps = 0;

		Init();

		// This list will have any newly available props written into it. Write a sentenel at the end.
		m_NewProxyProps[m_nNewProxyProps] = 99999;
		int *pCurNewProxyProp = m_NewProxyProps;

		for ( int i=0; i < nStartProps; i++ )
		{
			// Fill in the gaps with any properties that are newly enabled by the proxies.
			while ( *pCurNewProxyProp < pStartProps[i] )
			{
				AddProp( *pCurNewProxyProp );
				++pCurNewProxyProp;
			}

			// Now write this property's index if the proxies are allowing this property to be written.
			SeekToProp( pStartProps[i] );
			if ( IsCurProxyValid() )
			{
				AddProp( pStartProps[i] );
			}
		}
	}

	int GetNumOutProps()
	{
		return m_nOutProps;
	}


private:
	CSendTablePrecalc		*m_pPrecalc;
	int						m_iClient;	// Which client it's encoding out for.
	const CSendProxyRecipients	*m_pOldStateProxies;
	const int					m_nOldStateProxies;
	
	const CSendProxyRecipients	*m_pNewStateProxies;
	const int					m_nNewStateProxies;

	// The output property list.
	int						*m_pOutProps;
	int						m_nMaxOutProps;
	int						m_nOutProps;

	int m_NewProxyProps[MAX_DATATABLE_PROPS+1];
	int m_nNewProxyProps;
};



// ----------------------------------------------------------------------------- //
// CDeltaCalculator encapsulates part of the functionality for calculating deltas between entity states.
//
// To use it, initialize it with the from and to states, then walk through the 'to' properties using
// SeekToNextToProp(). 
//
// At each 'to' prop, either call SkipToProp or CalcDelta
// ----------------------------------------------------------------------------- //
class CDeltaCalculator
{
public:

						CDeltaCalculator(
							CSendTablePrecalc *pPrecalc,
							const void *pFromState,
							const int nFromBits,
							const void *pToState,
							const int nToBits,
							int *pDeltaProps,
							int nMaxDeltaProps,
							const int objectID );
						
						~CDeltaCalculator();

	// Returns the next prop index in the 'to' buffer. Returns PROP_SENTINEL if there are no more.
	// If a valid property index is returned, you MUST call PropSkip() or PropCalcDelta() after calling this.
	int					SeekToNextProp();	

	// Ignore the current 'to' prop.
	void				PropSkip();

	// Seek the 'from' buffer up to the current property, determine if there is a delta, and
	// write the delta (and the property data) into the buffer if there is a delta.
	void				PropCalcDelta();

	// Returns the number of deltas detected throughout the 
	int					GetNumDeltasDetected();
		

private:
	CSendTablePrecalc	*m_pPrecalc;

	bf_read				m_bfFromState;
	bf_read				m_bfToState;
	
	CDeltaBitsReader	m_FromBitsReader;
	CDeltaBitsReader	m_ToBitsReader;

	DecodeInfo			m_FromSkipper;
	DecodeInfo			m_ToSkipper;

	int					m_ObjectID;
	
	// Current property indices.
	int					m_iFromProp;
	int					m_iToProp;

	// Bit position into m_bfToState where the current prop (m_iToProp) starts.
	int					m_iToStateStart;

	// Output..
	int					*m_pDeltaProps;
	int					m_nMaxDeltaProps;
	int					m_nDeltaProps;
};


CDeltaCalculator::CDeltaCalculator(
	CSendTablePrecalc *pPrecalc,
	const void *pFromState,
	const int nFromBits,
	const void *pToState,
	const int nToBits,
	int *pDeltaProps,
	int nMaxDeltaProps,
	const int objectID ) 
	
	: m_bfFromState( "CDeltaCalculator->m_bfFromState", pFromState, PAD_NUMBER( nFromBits, 8 ) / 8, nFromBits ),
	m_FromBitsReader( &m_bfFromState ),
	m_bfToState( "CDeltaCalculator->m_bfToState", pToState, PAD_NUMBER( nToBits, 8 ) / 8, nToBits ),
	m_ToBitsReader( &m_bfToState )
{
	m_pPrecalc = pPrecalc;
	m_ObjectID = objectID;

	m_pDeltaProps = pDeltaProps;
	m_nMaxDeltaProps = nMaxDeltaProps;
	m_nDeltaProps = 0;
	
	// This is used to skip properties.
	InitDecodeInfoForSkippingProps( &m_FromSkipper, &m_bfFromState, objectID );
	InitDecodeInfoForSkippingProps( &m_ToSkipper, &m_bfToState, objectID );
	
	// Walk through each property in the to state, looking for ones in the from 
	// state that are missing or that are different.
	m_iFromProp = NextProp( &m_FromBitsReader );
	m_iToProp = -1;
}


CDeltaCalculator::~CDeltaCalculator()
{
	// Make sure we didn't overflow.
	ErrorIfNot( 
		m_nDeltaProps <= m_nMaxDeltaProps && !m_bfFromState.IsOverflowed() && !m_bfToState.IsOverflowed(), 
		( "SendTable_CalcDelta: overflowed on datatable '%s'.", m_pPrecalc->GetSendTable()->GetName() ) 
		);

	// We may not have read to the end of our input bits, but we don't care.
	m_FromBitsReader.ForceFinished();
	m_ToBitsReader.ForceFinished();
}


inline int CDeltaCalculator::SeekToNextProp()
{
	m_iToProp = NextProp( &m_ToBitsReader );
	return m_iToProp;
}
		

inline void CDeltaCalculator::PropSkip()
{
	Assert( m_iToProp != -1 );
	SkipPropData( &m_ToSkipper, m_pPrecalc->GetProp( m_iToProp ) );
}


void CDeltaCalculator::PropCalcDelta()
{
	// Skip any properties in the from state that aren't in the to state.
	while ( m_iFromProp < m_iToProp )
	{
		SkipPropData( &m_FromSkipper, m_pPrecalc->GetProp( m_iFromProp ) );
		m_iFromProp = NextProp( &m_FromBitsReader );
	}

	int bChange = true;

	if ( m_iFromProp == m_iToProp )
	{
		const SendProp *pProp = m_pPrecalc->GetProp( m_iToProp );

		// The property is in both states, so compare them and write the index 
		// if the states are different.
		bChange = g_PropTypeFns[pProp->m_Type].CompareDeltas( pProp, &m_bfFromState, &m_bfToState );
		
		// Seek to the next properties.
		m_iFromProp = NextProp( &m_FromBitsReader );
	}
	else
	{
		// Only the 'to' state has this property, so just skip its data and register a change.
		SkipPropData( &m_ToSkipper, m_pPrecalc->GetProp( m_iToProp ) );
	}

	if ( bChange )
	{
		// Write the property index.
		if ( m_nDeltaProps < m_nMaxDeltaProps )
		{
			m_pDeltaProps[m_nDeltaProps] = m_iToProp;
		}
		++m_nDeltaProps;
	}
}


inline int CDeltaCalculator::GetNumDeltasDetected()
{
	return m_nDeltaProps;
}


// ----------------------------------------------------------------------------- //
// CEncodeInfo
// Used by SendTable_Encode.
// ----------------------------------------------------------------------------- //
class CEncodeInfo : public CServerDatatableStack
{
public:
					CEncodeInfo( CSendTablePrecalc *pPrecalc, unsigned char *pStructBase, int objectID ) :
						CServerDatatableStack( pPrecalc, pStructBase, objectID )
					{
					}

public:

	bf_write	*m_pOut;
	bool		m_bEncodeAll;
	int			m_ObjectID;
	
	// Track sizes of what we write..
	int			m_nOverheadBits;
	int			m_nDataBits;

	CDeltaBitsWriter	*m_pDeltaBitsWriter;
};



// ------------------------------------------------------------------------ //
// Globals.
// ------------------------------------------------------------------------ //

CUtlVector< SendTable* > g_SendTables;



// ------------------------------------------------------------------------ //
// SendTable functions.
// ------------------------------------------------------------------------ //

static bool debug_info_shown = false;
static int  debug_bits_start = 0;

static inline void ShowEncodeDeltaWatchInfo( 
	const SendProp *pProp, 
	const DVariant *pVar,
	const int objectID )
{
#if defined(_DEBUG)
	if ( ShouldWatchThisProp(objectID, pProp->GetName()) )
	{
		if( g_PropTypeFns[pProp->m_Type].ShowSendWatchInfo )
		{
			static int lastframe = -1;
			if ( host_framecount != lastframe )
			{
				lastframe = host_framecount;
				Con_DPrintf( "%i\n", host_framecount );
			}

			debug_info_shown = true;

			g_PropTypeFns[pProp->m_Type].ShowSendWatchInfo( pVar, pProp, objectID );
		}
	}
#endif
}


static void SendTable_EncodeProp( CEncodeInfo *pInfo, unsigned long iProp )
{
	// Don't write a property if a proxy above the current datatable returned false.
	if ( !pInfo->IsCurProxyValid() )
		return;

	// Write the index.
	pInfo->m_nOverheadBits += pInfo->m_pDeltaBitsWriter->WritePropIndex( iProp );

	const SendProp *pProp = pInfo->GetCurProp();

	// Call their proxy to get the property's value.
	DVariant var;
	
	pProp->GetProxyFn()( 
		pInfo->GetCurStructBase(), 
		pInfo->GetCurStructBase() + pProp->GetOffset(), 
		&var, 
		0, // iElement
		pInfo->m_ObjectID
		);

	// Encode it.
	int iStartPos = pInfo->m_pOut->GetNumBitsWritten();

	g_PropTypeFns[pProp->m_Type].Encode( 
		pInfo->GetCurStructBase(), 
		&var, 
		pProp, 
		pInfo->m_pOut, 
		pInfo->m_ObjectID
		); 

	pInfo->m_nDataBits += ( pInfo->m_pOut->GetNumBitsWritten() - iStartPos ); // record # bits written.
}


static void SendTable_EncodePropIfNonZero( CEncodeInfo *pInfo, unsigned long iProp )
{
	// Don't write a property if a proxy above the current datatable returned false.
	if ( !pInfo->IsCurProxyValid() )
		return;

	const SendProp *pProp = pInfo->GetCurProp();

	// Call their proxy to get the property's value.
	DVariant var;
	
	pProp->GetProxyFn()( 
		pInfo->GetCurStructBase(), 
		pInfo->GetCurStructBase() + pProp->GetOffset(), 
		&var, 
		0, // iElement
		pInfo->m_ObjectID
		);

	if ( !g_PropTypeFns[pProp->m_Type].IsAllZeros( pInfo->GetCurStructBase(), &var, pProp ) )
	{
		// Write it for real if it's got a nonzero value.
		SendTable_EncodeProp( pInfo, iProp );
	}
}


int SendTable_CullPropsFromProxies( 
	const SendTable *pTable,
	
	const int *pStartProps,
	int nStartProps,

	const int iClient,
	
	const CSendProxyRecipients *pOldStateProxies,
	const int nOldStateProxies, 
	
	const CSendProxyRecipients *pNewStateProxies,
	const int nNewStateProxies,
	
	int *pOutProps,
	int nMaxOutProps
	)
{
	Assert( !( nNewStateProxies && !pNewStateProxies ) );
	CPropCullStack stack( pTable->m_pPrecalc, iClient, pOldStateProxies, nOldStateProxies, pNewStateProxies, nNewStateProxies );
	
	stack.CullPropsFromProxies( pStartProps, nStartProps, pOutProps, nMaxOutProps );

	ErrorIfNot( stack.GetNumOutProps() <= nMaxOutProps, ("CullPropsFromProxies: overflow in '%s'.", pTable->GetName()) );
	return stack.GetNumOutProps();
}


bool SendTable_Encode(
	const SendTable *pTable,
	const void *pStruct, 
	bf_write *pOut, 
	bf_read *pDeltaBits,
	int objectID,
	CUtlMemory<CSendProxyRecipients> *pRecipients,
	bool bNonZeroOnly
	)
{
	CSendTablePrecalc *pPrecalc = pTable->m_pPrecalc;
	ErrorIfNot( pPrecalc, ("SendTable_Encode: Missing m_pPrecalc for SendTable %s.", pTable->m_pNetTableName) );
	if ( pRecipients )
	{
		ErrorIfNot(	pRecipients->NumAllocated() >= pTable->GetNumDataTableProxies(), ("SendTable_Encode: pRecipients array too small.") );
	}

	VPROF( "SendTable_Encode" );

	CServerDTITimer timer( pTable, SERVERDTI_ENCODE );

	// This writes and delta-compresses the delta bits.
	CDeltaBitsWriter deltaBitsWriter( pOut );

	// Setup all the info we'll be walking the tree with.
	CEncodeInfo info( pPrecalc, (unsigned char*)pStruct, objectID );
	info.m_pOut = pOut;
	info.m_ObjectID = objectID;
	info.m_nDataBits = 0;
	info.m_nOverheadBits = 0;
	info.m_pDeltaBitsWriter = &deltaBitsWriter;
	info.m_pRecipients = pRecipients;	// optional buffer to store the bits for which clients get what data.
	
	// Walk through all the properties referenced in the delta bits and encode them.
	CEncodeInfo *pInfo = &info;

	if ( pDeltaBits )
	{
		CDeltaBitsReader deltaBitsReader( pDeltaBits );

		// Ok, only write the properties referenced by delta bits.
		int iProp;
		while( deltaBitsReader.ReadNextPropIndex( &iProp ) )
		{
			info.SeekToProp( iProp );
			SendTable_EncodeProp( &info, iProp );
		}
	}
	else if ( bNonZeroOnly )
	{
		// This happens the first time an entity is sent to a client.. Write ALL the properties..
		int iEndProp = pInfo->m_pPrecalc->m_Root.GetLastPropIndex();
		for ( int iProp=0; iProp <= iEndProp; iProp++ )
		{
			info.SeekToProp( iProp );
			SendTable_EncodePropIfNonZero( &info, iProp );
		}
	}
	else
	{
		// This happens the first time an entity is sent to a client.. Write ALL the properties..
		int iEndProp = pInfo->m_pPrecalc->m_Root.GetLastPropIndex();
		for ( int iProp=0; iProp <= iEndProp; iProp++ )
		{
			info.SeekToProp( iProp );
			SendTable_EncodeProp( &info, iProp );
		}
	}

	return !pOut->IsOverflowed();
}


void SendTable_WritePropList(
	const SendTable *pTable,
	const void *pState,
	const int nBits,
	bf_write *pOut,
	const int objectID,
	const int *pCheckProps,
	const int nCheckProps )
{

	debug_info_shown = false;
	debug_bits_start = pOut->GetNumBitsWritten();

	CSendTablePrecalc *pPrecalc = pTable->m_pPrecalc;
	CDeltaBitsWriter deltaBitsWriter( pOut );
	
	bf_read inputBuffer( "SendTable_WritePropList->inputBuffer", pState, BitByte( nBits ), nBits );
	CDeltaBitsReader inputBitsReader( &inputBuffer );

	DecodeInfo propSkipper;
	InitDecodeInfoForSkippingProps( &propSkipper, &inputBuffer, objectID );

	// Ok, they want to specify a small list of properties to check.
	int iToProp = NextProp( &inputBitsReader );
	int i = 0;
	while ( i < nCheckProps )
	{
		// Seek the 'to' state to the current property we want to check.
		while ( iToProp < pCheckProps[i] )
		{
			SkipPropData( &propSkipper, pPrecalc->GetProp( iToProp ) );
			iToProp = NextProp( &inputBitsReader );
		}

		if ( iToProp == PROP_SENTINEL )
		{
			break;
		}
		else if ( iToProp == pCheckProps[i] )
		{
			const SendProp *pProp = pPrecalc->GetProp( iToProp );

			// See how many bits the data for this property takes up.
			int iStartBit = inputBuffer.GetNumBitsRead();
			SkipPropData( &propSkipper, pProp );
			int nToStateBits = inputBuffer.GetNumBitsRead() - iStartBit;

			// Show debug stuff.
			ShowEncodeDeltaWatchInfo( pProp, &propSkipper.m_Value, objectID );
			
			// Write the data into the output.
			deltaBitsWriter.WritePropIndex( iToProp );
			inputBuffer.Seek( iStartBit );
			pOut->WriteBitsFromBuffer( &inputBuffer, nToStateBits );

			// Seek to the next prop.
			iToProp = NextProp( &inputBitsReader );
		}

		++i;
	}
	
	if ( debug_info_shown )
	{
		int  bits = pOut->GetNumBitsWritten() - debug_bits_start;

		Con_DPrintf( "-- %i bits (%i bytes)\n", bits, ( bits + 7 ) >> 3 );
	}

	inputBitsReader.ForceFinished(); // avoid a benign assert
}


int SendTable_CalcDelta(
	const SendTable *pTable,
	
	const void *pFromState,
	const int nFromBits,
	
	const void *pToState,
	const int nToBits,
	
	int *pDeltaProps,
	int nMaxDeltaProps,
	
	const int objectID
	)
{
	CServerDTITimer timer( pTable, SERVERDTI_CALCDELTA );

	VPROF( "SendTable_CalcDelta" );
	
	// Trivial reject.
	if ( CompareBitArrays( pFromState, pToState, nFromBits, nToBits ) )
	{
		return 0;
	}

	// Now just walk through each property in the 'to' buffer and write it in if it's a new property
	// or if it's different from the previous state.
	CDeltaCalculator deltaCalc( 
		pTable->m_pPrecalc, 
		pFromState, nFromBits, 
		pToState, nToBits, 
		pDeltaProps, nMaxDeltaProps, 
		objectID );

	// Just calculate a delta for each prop in the 'to' state.
	while ( deltaCalc.SeekToNextProp() != PROP_SENTINEL )
	{
		deltaCalc.PropCalcDelta();
	}
	
	// Return the # of properties that changed between 'from' and 'to'.
	return deltaCalc.GetNumDeltasDetected();
}


bool SendTable_SendInfo( SendTable *pTable, bf_write *pBuf )
{
	pBuf->WriteString(pTable->m_pNetTableName);
	pBuf->WriteUBitLong( (unsigned int)pTable->m_nProps, PROPINFOBITS_NUMPROPS );

	// Send each property.
	for ( int iProp=0; iProp < pTable->m_nProps; iProp++ )
	{
		const SendProp *pProp = &pTable->m_pProps[iProp];

		pBuf->WriteUBitLong( (unsigned int)pProp->m_Type, PROPINFOBITS_TYPE );
		pBuf->WriteString( pProp->GetName() );
		pBuf->WriteUBitLong( pProp->GetFlags(), PROPINFOBITS_FLAGS );

		if( pProp->m_Type == DPT_DataTable )
		{
			// Just write the name and it will be able to reuse the table with a matching name.
			pBuf->WriteString( pProp->GetDataTable()->m_pNetTableName );
		}
		else
		{
			if ( pProp->IsExcludeProp() )
			{
				pBuf->WriteString( pProp->GetExcludeDTName() );
			}
			else if( pProp->GetType() == DPT_String )
			{
				pBuf->WriteUBitLong( (unsigned int)pProp->m_StringBufferLen, PROPINFOBITS_STRINGBUFFERLEN );
			}
			else if ( pProp->GetType() == DPT_Array )
			{
				pBuf->WriteUBitLong( pProp->GetNumElements(), PROPINFOBITS_NUMELEMENTS );
			}
			else
			{			
				pBuf->WriteBitFloat( pProp->m_fLowValue );
				pBuf->WriteBitFloat( pProp->m_fHighValue );
				pBuf->WriteUBitLong( pProp->m_nBits, PROPINFOBITS_NUMBITS );
			}
		}
	}

	return !pBuf->IsOverflowed();
}


// Spits out warnings for invalid properties and forces property values to
// be in valid ranges for the encoders and decoders.
static void SendTable_Validate( CSendTablePrecalc *pPrecalc )
{
	SendTable *pTable = pPrecalc->m_pSendTable;
	for( int i=0; i < pTable->m_nProps; i++ )
	{
		SendProp *pProp = &pTable->m_pProps[i];
		
		if ( pProp->GetArrayProp() )
		{
			ErrorIfNot( 
				pProp->GetArrayProp()->GetType() != DPT_DataTable, 
				("Invalid property: %s/%s (array of datatables).", pTable->m_pNetTableName, pProp->GetName()) 
			);
		}
		else
		{
			ErrorIfNot( pProp->GetNumElements() == 1, ("Prop %s/%s has an invalid element count for a non-array.", pTable->m_pNetTableName, pProp->GetName()) );
		}
			
		// Check for 1-bit signed properties (their value doesn't get down to the client).
		if ( pProp->m_nBits == 1 && !(pProp->GetFlags() & SPROP_UNSIGNED) )
		{
			DataTable_Warning("SendTable prop %s::%s is a 1-bit signed property. Use SPROP_UNSIGNED or the client will never receive a value.\n", pTable->m_pNetTableName, pProp->GetName());
		}
	}
}


static bool SendTable_InitTable( SendTable *pTable )
{
	if( pTable->m_pPrecalc )
		return true;
	
	// Create the CSendTablePrecalc.	
	CSendTablePrecalc *pPrecalc = new CSendTablePrecalc;
	pTable->m_pPrecalc = pPrecalc;

	pPrecalc->m_pSendTable = pTable;
	pTable->m_pPrecalc = pPrecalc;

	// Bind the instrumentation if -dti was specified.
	pPrecalc->m_pDTITable = ServerDTI_HookTable( pTable );

	// Setup its flat property array.
	if ( !pPrecalc->SetupFlatPropertyArray() )
		return false;

	SendTable_Validate( pPrecalc );
	return true;
}


static void SendTable_TermTable( SendTable *pTable )
{
	if( !pTable->m_pPrecalc )
		return;

	delete pTable->m_pPrecalc;
	Assert( !pTable->m_pPrecalc ); // Make sure it unbound itself.
}


int SendTable_GetNumFlatProps( SendTable *pSendTable )
{
	CSendTablePrecalc *pPrecalc = pSendTable->m_pPrecalc;
	ErrorIfNot( pPrecalc,
		("SendTable_GetNumFlatProps: missing pPrecalc.")
	);
	return pPrecalc->GetNumProps();
}

CRC32_t SendTable_CRCTable( CRC32_t &crc, SendTable *pTable )
{
	CRC32_ProcessBuffer( &crc, (void *)pTable->m_pNetTableName, Q_strlen( pTable->m_pNetTableName) );
	CRC32_ProcessBuffer( &crc, (void *)&pTable->m_nProps, sizeof( pTable->m_nProps ) );

	// Send each property.
	for ( int iProp=0; iProp < pTable->m_nProps; iProp++ )
	{
		const SendProp *pProp = &pTable->m_pProps[iProp];

		CRC32_ProcessBuffer( &crc, (void *)&pProp->m_Type, sizeof( pProp->m_Type ) );
		CRC32_ProcessBuffer( &crc, (void *)pProp->GetName() , Q_strlen( pProp->GetName() ) );

		int flags = pProp->GetFlags();
		CRC32_ProcessBuffer( &crc, (void *)&flags, sizeof( flags ) );

		if( pProp->m_Type == DPT_DataTable )
		{
			CRC32_ProcessBuffer( &crc, (void *)pProp->GetDataTable()->m_pNetTableName, Q_strlen( pProp->GetDataTable()->m_pNetTableName ) );
		}
		else
		{
			if ( pProp->IsExcludeProp() )
			{
				CRC32_ProcessBuffer( &crc, (void *)pProp->GetExcludeDTName(), Q_strlen( pProp->GetExcludeDTName() ) );
			}
			else if( pProp->GetType() == DPT_String )
			{
				CRC32_ProcessBuffer( &crc, (void *)&pProp->m_StringBufferLen, sizeof( pProp->m_StringBufferLen) );
			}
			else if ( pProp->GetType() == DPT_Array )
			{
				int numelements = pProp->GetNumElements();
				CRC32_ProcessBuffer( &crc, (void *)&numelements, sizeof( numelements ) );
			}
			else
			{	
				CRC32_ProcessBuffer( &crc, (void *)&pProp->m_fLowValue, sizeof( pProp->m_fLowValue ) );
				CRC32_ProcessBuffer( &crc, (void *)&pProp->m_fHighValue, sizeof( pProp->m_fHighValue ) );
				CRC32_ProcessBuffer( &crc, (void *)&pProp->m_nBits, sizeof( pProp->m_nBits ) );
			}
		}
	}

	return crc;
}

bool SendTable_Init( SendTable **pTables, int nTables )
{
	ErrorIfNot( g_SendTables.Count() == 0,
		("SendTable_Init: called twice.")
	);

	// Initialize them all.
	for ( int i=0; i < nTables; i++ )
		if ( !SendTable_InitTable( pTables[i] ) )
			return false;

	// Store off the SendTable list.
	g_SendTables.CopyArray( pTables, nTables );
	return true;	
}


void SendTable_Term()
{
	// Term all the SendTables.
	for ( int i=0; i < g_SendTables.Count(); i++ )
		SendTable_TermTable( g_SendTables[i] );

	// Clear the list of SendTables.
	g_SendTables.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Computes the crc for all sendtables for the data sent in the class/table definitions
// Output : CRC32_t
//-----------------------------------------------------------------------------
CRC32_t SendTable_ComputeCRC()
{
	CRC32_t result;
	CRC32_Init( &result );

	// walk the tables and checksum them
	int c = g_SendTables.Count();
	for ( int i = 0 ; i < c; i++ )
	{
		SendTable *st = g_SendTables[ i ];
		result = SendTable_CRCTable( result, st );
	}

	SV_ComputeClassInfosCRC( &result );

	CRC32_Final( &result );

	return result;
}