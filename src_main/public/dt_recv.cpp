//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "dt_recv.h"
#include "vector.h"
#include "vstdlib/strtools.h"

class CArrayRecvProp
{
public:
				CArrayRecvProp() :
					m_RecvTable( NULL, 0, "" )
				{
				}

	RecvTable	m_RecvTable;
	RecvProp	m_RecvProp;
	char		m_DataTableName[64];
};
	

// ---------------------------------------------------------------------- //
// RecvProp.
// ---------------------------------------------------------------------- //
RecvProp::RecvProp()
{
	m_pVarName = NULL;
	m_Offset = 0;
	m_RecvType = DPT_Int;
	m_Flags = 0;
	m_ProxyFn = NULL;
	m_DataTableProxyFn = NULL;
	m_pDataTable = NULL;
	m_nElements = 1;
	m_ElementStride = -1;
	m_pArrayProp = NULL;
	m_ArrayLengthProxy = NULL;
	m_bInsideArray = false;
}

// ---------------------------------------------------------------------- //
// RecvTable.
// ---------------------------------------------------------------------- //
RecvTable::RecvTable()
{
	Construct( NULL, 0, NULL );
}

RecvTable::RecvTable(RecvProp *pProps, int nProps, char *pNetTableName)
{
	Construct( pProps, nProps, pNetTableName );
}

RecvTable::~RecvTable()
{
}

void RecvTable::Construct( RecvProp *pProps, int nProps, char *pNetTableName )
{
	m_pProps = pProps;
	m_nProps = nProps;
	m_pDecoder = NULL;
	m_pNetTableName = pNetTableName;
	m_bInitialized = false;
	m_bInMainList = false;
}


// ---------------------------------------------------------------------- //
// Prop setup functions (for building tables).
// ---------------------------------------------------------------------- //

RecvProp RecvPropFloat(
	char *pVarName, 
	int offset, 
	int sizeofVar,
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	// Debug type checks.
	#ifdef _DEBUG
		if(varProxy == RecvProxy_FloatToFloat)
		{
			Assert(sizeofVar == 4);
		}
	#endif

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_Float;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropVector(
	char *pVarName, 
	int offset, 
	int sizeofVar,
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	// Debug type checks.
	#ifdef _DEBUG
		if(varProxy == RecvProxy_VectorToVector)
		{
			Assert(sizeofVar == sizeof(Vector));
		}
	#endif

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_Vector;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropInt(
	char *pVarName, 
	int offset, 
	int sizeofVar,
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	// If they didn't specify a proxy, then figure out what type we're writing to.
	if(varProxy == NULL)
	{
		if(sizeofVar == 1)
		{
			varProxy = RecvProxy_Int32ToInt8;
		}
		else if(sizeofVar == 2)
		{
			varProxy = RecvProxy_Int32ToInt16;
		}
		else if(sizeofVar == 4)
		{
			varProxy = RecvProxy_Int32ToInt32;
		}
		else
		{
			Assert(!"RecvPropInt var has invalid size");
			varProxy = RecvProxy_Int32ToInt8;	// safest one...
		}
	}

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_Int;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropString(
	char *pVarName,
	int offset,
	int bufferSize,
	int flags,
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_String;
	ret.m_Flags = flags;
	ret.m_StringBufferSize = bufferSize;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropDataTable(
	char *pVarName,
	int offset,
	int flags,
	RecvTable *pTable,
	DataTableRecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_DataTable;
	ret.m_Flags = flags;
	ret.SetDataTableProxyFn( varProxy );
	ret.SetDataTable( pTable );

	return ret;
}

RecvProp InternalRecvPropArray(
	const int elementCount,
	const int elementStride,
	char *pName,
	ArrayLengthRecvProxyFn proxy
	)
{
	RecvProp ret;

	ret.InitArray( elementCount, elementStride );
	ret.m_pVarName = pName;
	ret.SetArrayLengthProxy( proxy );

	return ret;
}


// ---------------------------------------------------------------------- //
// Proxies.
// ---------------------------------------------------------------------- //

void RecvProxy_FloatToFloat( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	Assert( IsFinite( pData->m_Value.m_Float ) );
	*((float*)pOut) = pData->m_Value.m_Float;
}

void RecvProxy_VectorToVector( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	const float *v = pData->m_Value.m_Vector;
	
	Assert( IsFinite( v[0] ) && IsFinite( v[1] ) && IsFinite( v[2] ) );
	((float*)pOut)[0] = v[0];
	((float*)pOut)[1] = v[1];
	((float*)pOut)[2] = v[2];
}

void RecvProxy_Int32ToInt8( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((unsigned char*)pOut) = *((unsigned char*)&pData->m_Value.m_Int);
}

void RecvProxy_Int32ToInt16( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((unsigned short*)pOut) = *((unsigned short*)&pData->m_Value.m_Int);
}

void RecvProxy_Int32ToInt32( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((unsigned long*)pOut) = *((unsigned long*)&pData->m_Value.m_Int);
}

void RecvProxy_StringToString( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	char *pStrOut = (char*)pOut;
	if ( pData->m_pRecvProp->m_StringBufferSize <= 0 )
	{
		return;
	}

	for ( int i=0; i < pData->m_pRecvProp->m_StringBufferSize; i++ )
	{
		pStrOut[i] = pData->m_Value.m_pString[i];
		if(pStrOut[i] == 0)
			break;
	}
	
	pStrOut[pData->m_pRecvProp->m_StringBufferSize-1] = 0;
}

void DataTableRecvProxy_StaticDataTable(void **pOut, void *pData, int objectID)
{
	*pOut = pData;
}

void DataTableRecvProxy_PointerDataTable(void **pOut, void *pData, int objectID)
{
	*pOut = *((void**)pData);
}



