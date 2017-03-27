//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DATATABLE_H
#define DATATABLE_H
#ifdef _WIN32
#pragma once
#endif


#include "dt_common.h"
#include "dt_recv_eng.h"
#include "dt_send_eng.h"
#include "utlvector.h"
#include "dt_encode.h"


class bf_read;
class bf_write;
class SendTable;
class RecvTable;
class CDTISendTable;


// (Temporary.. switch to something more efficient). Number of bits to
// encode property indices in the delta bits.
#define PROP_SENTINEL	0x7FFFFFFF


#define MAX_EXCLUDE_PROPS		512


// Bit counts used to encode the information about a property.
#define PROPINFOBITS_NUMPROPS			9
#define PROPINFOBITS_TYPE				5
#define PROPINFOBITS_FLAGS				SPROP_NUMFLAGBITS
#define PROPINFOBITS_STRINGBUFFERLEN	9
#define PROPINFOBITS_NUMBITS			6
#define PROPINFOBITS_RIGHTSHIFT			6
#define PROPINFOBITS_NUMELEMENTS		10	// For arrays.



class ExcludeProp
{
public:
	char const	*m_pTableName;
	char const	*m_pPropName;
};


// ------------------------------------------------------------------------------------ //
// CDeltaBitsReader.
// ------------------------------------------------------------------------------------ //

class CDeltaBitsReader
{
public:
				CDeltaBitsReader( bf_read *pBuf );
				~CDeltaBitsReader();

	// Write the next property index. Returns the number of bits used.
	bool		ReadNextPropIndex( int *iProp );

	// If you know you're done but you're not at the end (you haven't called until
	// ReadNextPropIndex returns false), call this so it won't assert in its destructor.
	void		ForceFinished();


private:

	bf_read		*m_pBuf;
	bool		m_bFinished;
	int			m_iLastProp;
};


// ------------------------------------------------------------------------------------ //
// CDeltaBitsWriter.
// ------------------------------------------------------------------------------------ //

class CDeltaBitsWriter
{
public:
				CDeltaBitsWriter( bf_write *pBuf );
				~CDeltaBitsWriter();

	// Write the next property index. Returns the number of bits used.
	int			WritePropIndex( int iProp );

	// Access the buffer it's outputting to.
	bf_write*	GetBitBuf();

private:
	
	bf_write	*m_pBuf;
	int			m_iLastProp;
};

inline bf_write* CDeltaBitsWriter::GetBitBuf()
{
	return m_pBuf;
}


// ----------------------------------------------------------------------------- //
// 
// CSendNode
//
// Each datatable gets a tree of CSendNodes. There is one CSendNode
// for each datatable property that was in the original SendTable.
//
// ----------------------------------------------------------------------------- //

class CSendNode
{
public:

					CSendNode();
					~CSendNode();

	int				GetNumChildren() const;
	CSendNode*		GetChild( int i ) const;
	
	int				GetFirstPropIndex() const;
	int				GetNumProps() const;

	// Note: this can return a negative index if it's the root and there are no props.
	int				GetLastPropIndex() const;
	
	// Returns true if the specified prop is in this node.
	bool			IsPropInNodeProps( int i ) const;	
	
	// Returns true if the specified prop is in this node or any of its children.
	bool			IsPropInRecursiveProps( int i ) const;


public:

	// Child datatables.
	CUtlVector<CSendNode*>	m_Children;

	// The datatable property that leads us to this CSendNode.
	// This indexes the CSendTablePrecalc or CRecvDecoder's m_DatatableProps list.
	// The root CSendNode sets this to -1.
	int				m_iDatatableProp;

	// The SendTable that this node represents.
	// ALL CSendNodes have this.
	const SendTable	*m_pTable;

	//
	// Properties in this table.
	//

	// m_iFirstRecursiveProp to m_nRecursiveProps defines the list of propertise
	// of this node and all its children.
	unsigned short	m_iFirstRecursiveProp;
	unsigned short	m_nRecursiveProps;

	// m_iFirstProp to m_nProps define the list of properties in this node
	// (but not in its children).
	unsigned short	m_iFirstProp;
	unsigned short	m_nProps;
};


inline int CSendNode::GetNumChildren() const
{
	return m_Children.Count(); 
}

inline CSendNode* CSendNode::GetChild( int i ) const
{
	return m_Children[i];
}

inline int CSendNode::GetFirstPropIndex() const
{
	return m_iFirstProp; 
}

inline int CSendNode::GetNumProps() const
{
	return m_nProps; 
}

inline int CSendNode::GetLastPropIndex() const
{
	return (int)(m_iFirstProp + m_nProps) - 1; 
}

inline bool CSendNode::IsPropInNodeProps( int i ) const
{
	int index = i - (int)m_iFirstProp;
	return index >= 0 && index < m_nProps;
}

inline bool CSendNode::IsPropInRecursiveProps( int i ) const
{
	int index = i - (int)m_iFirstRecursiveProp;
	return index >= 0 && index < m_nRecursiveProps;
}



// ----------------------------------------------------------------------------- //
// CSendTablePrecalc
// ----------------------------------------------------------------------------- //
class CSendTablePrecalc
{
public:
						CSendTablePrecalc();
	virtual				~CSendTablePrecalc();

	// This function builds the flat property array given a SendTable.
	bool				SetupFlatPropertyArray();

	int					GetNumProps() const;
	const SendProp*		GetProp( int i ) const;

	int					GetNumDatatableProps() const;
	const SendProp*		GetDatatableProp( int i ) const;

	SendTable*			GetSendTable() const;
	CSendNode*			GetRootNode();


public:
	
	// These are what CSendNodes reference.
	// These are actual data properties (ints, floats, etc).
	CUtlVector<const SendProp*>	m_Props;

	// Each datatable in a SendTable's tree gets a proxy index, and its properties reference that.
	CUtlVector<unsigned char> m_PropProxyIndices;

	// CSendNode::m_iDatatableProp indexes this.
	// These are the datatable properties (SendPropDataTable).
	CUtlVector<const SendProp*>	m_DatatableProps;

	// This is the property hierarchy, with the nodes indexing m_Props.
	CSendNode				m_Root;

	// From whence we came.
	SendTable				*m_pSendTable;

	// For instrumentation.
	CDTISendTable			*m_pDTITable;
};


inline int CSendTablePrecalc::GetNumProps() const
{
	return m_Props.Count(); 
}

inline const SendProp* CSendTablePrecalc::GetProp( int i ) const
{
	return m_Props[i]; 
}

inline int CSendTablePrecalc::GetNumDatatableProps() const
{
	return m_DatatableProps.Count();
}

inline const SendProp* CSendTablePrecalc::GetDatatableProp( int i ) const
{
	return m_DatatableProps[i];
}

inline SendTable* CSendTablePrecalc::GetSendTable() const
{
	return m_pSendTable; 
}

inline CSendNode* CSendTablePrecalc::GetRootNode()
{
	return &m_Root; 
}


// ------------------------------------------------------------------------ //
// Helpers.
// ------------------------------------------------------------------------ //

// Used internally by various datatable modules.
void DataTable_Warning( const char *pInMessage, ... );
bool ShouldWatchThisProp( int objectID, const char *pPropName );

// Same as AreBitArraysEqual but does a trivial test to make sure the 
// two arrays are equally sized.
bool CompareBitArrays(
	void const *pPacked1,
	void const *pPacked2,
	int nBits1,
	int nBits2
	);


// Helper routines for seeking through encoded buffers.
inline int NextProp( CDeltaBitsReader *pDeltaBitsReader )
{
	int iProp;
	if ( pDeltaBitsReader->ReadNextPropIndex( &iProp ) )
		return iProp;
	else
		return PROP_SENTINEL;
}


// Initialize pDecodeInfo's state so when you call Decode with it, it will skip the properties
// in the buffer. You must set DecodeInfo::m_pProp before calling Decode.
inline void InitDecodeInfoForSkippingProps( DecodeInfo *pDecodeInfo, bf_read *pBuf, int objectID )
{
	pDecodeInfo->m_pStruct = pDecodeInfo->m_pData = 0;
	pDecodeInfo->m_pRecvProp = 0;
	pDecodeInfo->m_ObjectID = objectID;
	pDecodeInfo->m_pIn = pBuf;
}


// Skips over the data in pDecodeInfo's buffer, assuming it's of the type specified by pProp.
// pDecodeInfo MUST have been initialized with InitDecodeInfoForSkippingProps.
inline void SkipPropData( DecodeInfo *pDecodeInfo, const SendProp *pProp )
{
	// Make sure it was initialized with InitDecodeInfoForSkippingProps.
	Assert( pDecodeInfo->m_pData == 0 && pDecodeInfo->m_pStruct == 0 );
	
	pDecodeInfo->m_pProp = pProp;
	g_PropTypeFns[ pProp->GetType() ].Decode( pDecodeInfo );
}


// This is to be called on SendTables and RecvTables to setup array properties
// to point at their property templates and to set the SPROP_INSIDEARRAY flag
// on the properties inside arrays.
// We make the proptype an explicit template parameter because
// gcc templating cannot deduce typedefs from classes in templates properly

template< class TableType, class PropType >
void SetupArrayProps_R( TableType *pTable )
{
	// If this table has already been initialized in here, then jump out.
	if ( pTable->IsInitialized() )
		return;

	pTable->SetInitialized( true );

	for ( int i=0; i < pTable->GetNumProps(); i++ )
	{
		PropType *pProp = pTable->GetProp( i );

		if ( pProp->GetType() == DPT_Array )
		{
			ErrorIfNot( i >= 1,
				("SetupArrayProps_R: array prop '%s' is at index zero.", pProp->GetName())
			);

			// Get the property defining the elements in the array.
			PropType *pArrayProp = pTable->GetProp( i-1 );
			pArrayProp->SetInsideArray();
			pProp->SetArrayProp( pArrayProp );
		}
		else if ( pProp->GetType() == DPT_DataTable )
		{
			// Recurse into children datatables.
			SetupArrayProps_R<TableType,PropType>( pProp->GetDataTable() );
		}
	}
}


#endif // DATATABLE_H
