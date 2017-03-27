//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "dt_send.h"
#include "mathlib.h"
#include "vector.h"
#include "tier0/dbg.h"



// ---------------------------------------------------------------------- //
// Proxies.
// ---------------------------------------------------------------------- //
void SendProxy_AngleToFloat( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	float angle;

	angle = *((float*)pData);
	pOut->m_Float = anglemod( angle );

	Assert( IsFinite( pOut->m_Float ) );
}

void SendProxy_FloatToFloat( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Float = *((float*)pData);
	Assert( IsFinite( pOut->m_Float ) );
}

void SendProxy_QAngles( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	QAngle *v = (QAngle*)pData;
	pOut->m_Vector[0] = anglemod( v->x );
	pOut->m_Vector[1] = anglemod( v->y );
	pOut->m_Vector[2] = anglemod( v->z );
}

void SendProxy_VectorToVector( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	Vector& v = *(Vector*)pData;
	Assert( v.IsValid() );
	pOut->m_Vector[0] = v[0];
	pOut->m_Vector[1] = v[1];
	pOut->m_Vector[2] = v[2];
}

void SendProxy_Int8ToInt32( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((char*)pData);
}

void SendProxy_Int16ToInt32( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((short*)pData);
}

void SendProxy_Int32ToInt32( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((int*)pData);
}

void SendProxy_UInt8ToInt32( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((unsigned char*)pData);
}

void SendProxy_UInt16ToInt32( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((unsigned short*)pData);
}

void SendProxy_UInt32ToInt32( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	*((unsigned long*)&pOut->m_Int) = *((unsigned long*)pData);
}

void SendProxy_StringToString( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_pString = (char*)pData;
}

void* SendProxy_DataTableToDataTable( const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	return (void*)pData;
}

void* SendProxy_DataTablePtrToDataTable( const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	return *((void**)pData);
}

static void SendProxy_Empty( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
}



// ---------------------------------------------------------------------- //
// Prop setup functions (for building tables).
// ---------------------------------------------------------------------- //
SendProp SendPropFloat(
	char *pVarName,		
	// Variable name.
	int offset,			// Offset into container structure.
	int sizeofVar,
	int nBits,			// Number of bits to use when encoding.
	int flags,
	float fLowValue,		// For floating point, low and high values.
	float fHighValue,		// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if(varProxy == SendProxy_FloatToFloat)
	{
		Assert(sizeofVar == 4);
	}

	if(fHighValue == HIGH_DEFAULT)
		fHighValue = (1 << nBits);

	if (flags & SPROP_ROUNDDOWN)
		fHighValue = fHighValue - ((fHighValue - fLowValue) / (1 << nBits));
	else if (flags & SPROP_ROUNDUP)
		fLowValue = fLowValue + ((fHighValue - fLowValue) / (1 << nBits));

	ret.m_Type = DPT_Float;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = fLowValue;
	ret.m_fHighValue = fHighValue;
	ret.m_fHighLowMul = ((1 << ret.m_nBits) - 1) / (fHighValue - fLowValue);
	ret.SetProxyFn( varProxy );
	if( ret.GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL) )
		ret.m_nBits = 0;

	return ret;
}

SendProp SendPropVector(
	char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,					// Number of bits to use when encoding.
	int flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue,			// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if(varProxy == SendProxy_VectorToVector)
	{
		Assert(sizeofVar == sizeof(Vector));
	}

	ret.m_Type = DPT_Vector;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = fLowValue;
	ret.m_fHighValue = fHighValue;
	ret.m_fHighLowMul = ((1 << ret.m_nBits) - 1) / (fHighValue - fLowValue);
	ret.SetProxyFn( varProxy );
	if( ret.GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL) )
		ret.m_nBits = 0;

	return ret;
}


SendProp SendPropAngle(
	char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	int flags,
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if(varProxy == SendProxy_AngleToFloat)
	{
		Assert(sizeofVar == 4);
	}

	ret.m_Type = DPT_Float;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = 0.0f;
	ret.m_fHighValue = 360.0f;
	ret.m_fHighLowMul = ((1 << ret.m_nBits) - 1) / 360.0f;
	ret.SetProxyFn( varProxy );

	return ret;
}

SendProp SendPropQAngles(
	char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	int flags,
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if(varProxy == SendProxy_AngleToFloat)
	{
		Assert(sizeofVar == 4);
	}

	ret.m_Type = DPT_Vector;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = 0.0f;
	ret.m_fHighValue = 360.0f;
	ret.m_fHighLowMul = ((1 << ret.m_nBits) - 1) / 360.0f;
	ret.SetProxyFn( varProxy );

	return ret;
}

SendProp SendPropInt(
	char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	int flags,
	SendVarProxyFn varProxy
	)
{
	SendProp ret;

	if ( !varProxy )
	{
		if ( sizeofVar == 1 )
		{
			varProxy = SendProxy_Int8ToInt32;
		}
		else if ( sizeofVar == 2 )
		{
			varProxy = SendProxy_Int16ToInt32;
		}
		else if ( sizeofVar == 4 )
		{
			varProxy = SendProxy_Int32ToInt32;
		}
		else
		{
			Assert(!"SendPropInt var has invalid size");
			varProxy = SendProxy_Int8ToInt32;	// safest one...
		}
	}

	// Figure out # of bits if the want us to.
	if ( nBits == -1 )
	{
		Assert( sizeofVar == 1 || sizeofVar == 2 || sizeofVar == 4 );
		nBits = sizeofVar * 8;
	}

	ret.m_Type = DPT_Int;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );

	// Use UInt proxies if they want unsigned data. This isn't necessary to encode
	// the values correctly, but it lets us check the ranges of the data to make sure
	// they're valid.
	ret.SetProxyFn( varProxy );
	if( ret.GetFlags() & SPROP_UNSIGNED )
	{
		if( varProxy == SendProxy_Int8ToInt32 )
			ret.SetProxyFn( SendProxy_UInt8ToInt32 );
		
		else if( varProxy == SendProxy_Int16ToInt32 )
			ret.SetProxyFn( SendProxy_UInt16ToInt32 );

		else if( varProxy == SendProxy_Int32ToInt32 )
			ret.SetProxyFn( SendProxy_UInt32ToInt32 );
	}

	return ret;
}

SendProp SendPropString(
	char *pVarName,
	int offset,
	int bufferLen,
	int flags,
	SendVarProxyFn varProxy)
{
	SendProp ret;

	Assert( bufferLen < DT_MAX_STRING_BUFFERSIZE ); // You can only have strings with 8-bits worth of length.
	
	ret.m_Type = DPT_String;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_StringBufferLen = bufferLen;
	ret.SetFlags( flags );
	ret.SetProxyFn( varProxy );

	return ret;
}

SendProp SendPropDataTable(
	char *pVarName,
	int offset,
	SendTable *pTable,
	SendTableProxyFn varProxy
	)
{
	SendProp ret;

	ret.m_Type = DPT_DataTable;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.SetDataTable( pTable );
	ret.SetDataTableProxyFn( varProxy );
	
	// Handle special proxy types where they always let all clients get the results.
	if ( varProxy == SendProxy_DataTableToDataTable || varProxy == SendProxy_DataTablePtrToDataTable )
	{
		ret.SetFlags( SPROP_PROXY_ALWAYS_YES );
	}

	return ret;
}


SendProp InternalSendPropArray(
	const int elementCount,
	const int elementStride,
	char *pName,
	ArrayLengthSendProxyFn arrayLengthFn
	)
{
	SendProp ret;

	ret.m_Type = DPT_Array;
	ret.m_nElements = elementCount;
	ret.m_ElementStride = elementStride;
	ret.m_pVarName = pName;
	ret.SetProxyFn( SendProxy_Empty );
	ret.m_pArrayProp = NULL;	// This gets set in SendTable_InitTable. It always points at the property that precedes
								// this one in the datatable's list.
	ret.SetArrayLengthProxy( arrayLengthFn );
		
	return ret;
}


SendProp SendPropExclude(
	char *pDataTableName,	// Data table name (given to BEGIN_SEND_TABLE and BEGIN_RECV_TABLE).
	char *pPropName		// Name of the property to exclude.
	)
{
	SendProp ret;

	ret.SetFlags( SPROP_EXCLUDE );
	ret.m_pExcludeDTName = pDataTableName;
	ret.m_pVarName = pPropName;

	return ret;
}



// ---------------------------------------------------------------------- //
// SendProp
// ---------------------------------------------------------------------- //
SendProp::SendProp()
{
	m_pVarName = NULL;
	m_Offset = 0;
	m_pDataTable = NULL;
	m_ProxyFn = NULL;
	m_pExcludeDTName = NULL;
	m_StringBufferLen = 0;
	
	m_Type = DPT_Int;
	m_Flags = 0;
	m_nBits = 0;
	m_fLowValue = 0.0f;
	m_fHighValue = 0.0f;
	m_pArrayProp = 0;
	m_ArrayLengthProxy = 0;
	m_nElements = 1;
	m_ElementStride = -1;
	m_DataTableProxyIndex = DATATABLE_PROXY_INDEX_INVALID; // set it to a questionable value.
}


SendProp::~SendProp()
{
}


int SendProp::GetNumArrayLengthBits() const
{
	Assert( GetType() == DPT_Array );
	return Q_log2( GetNumElements() ) + 1;
}


// ---------------------------------------------------------------------- //
// SendTable
// ---------------------------------------------------------------------- //
SendTable::SendTable()
{
	Construct( NULL, 0, NULL );
}


SendTable::SendTable(SendProp *pProps, int nProps, char *pNetTableName)
{
	Construct( pProps, nProps, pNetTableName );
}


SendTable::~SendTable()
{
//	Assert( !m_pPrecalc );
}


void SendTable::Construct( SendProp *pProps, int nProps, char *pNetTableName )
{
	m_pProps = pProps;
	m_nProps = nProps;
	m_pNetTableName = pNetTableName;
	m_pPrecalc = 0;
	m_bInitialized = false;
	m_nWriteSpawnCount = -1;
	m_nDataTableProxies = 0;
}

