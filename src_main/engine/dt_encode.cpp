//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include "dt.h"
#include "dt_encode.h"
#include "coordsize.h"


extern void DataTable_Warning( const char *pInMessage, ... );
extern bool ShouldWatchThisProp( int objectID, const char *pPropName );

// The engine implements this.
extern const char* GetObjectClassName( int objectID );


static inline void EncodeInt( const SendProp *pProp, long val, bf_write *pOut )
{
	if( pProp->IsSigned() )
		pOut->WriteSBitLong( val, pProp->m_nBits );
	else
		pOut->WriteUBitLong( (unsigned int)val, pProp->m_nBits );
}


// Check for special flags like SPROP_COORD, SPROP_NOSCALE, and SPROP_NORMAL.
// Returns true if it encoded the float.
static inline bool EncodeSpecialFloat( const SendProp *pProp, float fVal, bf_write *pOut )
{
	if( pProp->GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL) )
	{
		if(pProp->GetFlags() & SPROP_COORD)
		{
			pOut->WriteBitCoord(fVal);
			return true;
		}
		else if(pProp->GetFlags() & SPROP_NOSCALE)
		{
			pOut->WriteBitFloat(fVal);
			return true;
		}
		else if(pProp->GetFlags() & SPROP_NORMAL)
		{
			pOut->WriteBitNormal(fVal);
			return true;
		}
	}

	return false;
}


static inline void EncodeFloat( const SendProp *pProp, float fVal, bf_write *pOut, int objectID )
{
	// Check for special flags like SPROP_COORD, SPROP_NOSCALE, and SPROP_NORMAL.
	if( EncodeSpecialFloat( pProp, fVal, pOut ) )
	{
		return;
	}

	unsigned long ulVal;
	if( fVal < pProp->m_fLowValue )
	{
		// clamp < 0
		ulVal = 0;
		
		if(!(pProp->GetFlags() & SPROP_ROUNDUP))
		{
			DataTable_Warning("(class %s): Out-of-range value (%f) in SendPropFloat '%s', clamping.\n", GetObjectClassName( objectID ), fVal, pProp->m_pVarName );
		}
	}
	else if( fVal > pProp->m_fHighValue )
	{
		// clamp > 1
		ulVal = ((1 << pProp->m_nBits) - 1);

		if(!(pProp->GetFlags() & SPROP_ROUNDDOWN))
		{
			DataTable_Warning("%s: Out-of-range value (%f) in SendPropFloat '%s', clamping.\n", GetObjectClassName( objectID ), fVal, pProp->m_pVarName );
		}
	}
	else
	{
		float fRangeVal = (fVal - pProp->m_fLowValue) * pProp->m_fHighLowMul;
		ulVal = RoundFloatToUnsignedLong( fRangeVal );
	}
	
	pOut->WriteUBitLong(ulVal, pProp->m_nBits);
}


// Look for special flags like SPROP_COORD, SPROP_NOSCALE, and SPROP_NORMAL and
// decode if they're there. Fills in fVal and returns true if it decodes anything.
static inline bool DecodeSpecialFloat( SendProp const *pProp, bf_read *pIn, float &fVal )
{
	if(pProp->GetFlags() & SPROP_COORD)
	{
		fVal = pIn->ReadBitCoord();
		return true;
	}
	else if(pProp->GetFlags() & SPROP_NOSCALE)
	{
		fVal = pIn->ReadBitFloat();
		return true;
	}
	else if(pProp->GetFlags() & SPROP_NORMAL)
	{
		fVal = pIn->ReadBitNormal();
		return true;
	}
	else
	{
		fVal = 0;
		return false;
	}
}


static float DecodeFloat(SendProp const *pProp, bf_read *pIn)
{
	float fVal;
	unsigned long dwInterp;

	// Check for special flags..
	if( DecodeSpecialFloat( pProp, pIn, fVal ) )
	{
		return fVal;
	}

	dwInterp = pIn->ReadUBitLong(pProp->m_nBits);
	fVal = (float)dwInterp / ((1 << pProp->m_nBits) - 1);
	fVal = pProp->m_fLowValue + (pProp->m_fHighValue - pProp->m_fLowValue) * fVal;
	return fVal;
}


void PrintDebugInfo(int objectID, const char *pVarName, const float val)
{
	Con_DPrintf("DataTable Recv: ent %d, var %s: %f\n", objectID, pVarName, val);
}
void PrintDebugInfoInt(int objectID, const char *pVarName, const long val)
{
	Con_DPrintf("DataTable Recv: ent %d, var %s: %d\n", objectID, pVarName, val);
}
void PrintDebugInfo(int objectID, const char *pVarName, const Vector &val)
{
	Con_DPrintf("DataTable Recv: ent %d, var %s: (%.3f %.3f %.3f)\n", objectID, pVarName, val.x, val.y, val.z);
}
void PrintDebugInfo(int objectID, const char *pVarName, const char *val)
{
	Con_DPrintf("DataTable Recv: ent %d, var %s: %s\n", objectID, pVarName, val);
}


int	DecodeBits( DecodeInfo *pInfo, unsigned char *pOut )
{
	bf_read temp;

	// Read the property in (note: we don't return the bits from here because Decode returns
	// the decoded bits.. we're interested in getting the encoded bits).
	temp = *pInfo->m_pIn;
	pInfo->m_pRecvProp = NULL;
	pInfo->m_pData = NULL;
	g_PropTypeFns[pInfo->m_pProp->m_Type].Decode( pInfo );

	// Return the encoded bits.
	int nBits = pInfo->m_pIn->m_iCurBit - temp.m_iCurBit;
	temp.ReadBits(pOut, nBits);
	return nBits;
}


// ---------------------------------------------------------------------------------------- //
// Most of the prop types can use this generic FastCopy version. Arrays are a bit of a pain.
// ---------------------------------------------------------------------------------------- //

inline void Generic_FastCopy( 
	const SendProp *pSendProp, 
	const RecvProp *pRecvProp, 
	const unsigned char *pSendData, 
	unsigned char *pRecvData,
	int objectID )
{
	// Get the data out of the ent.
	CRecvProxyData recvProxyData;

	pSendProp->GetProxyFn()( 
		pSendData, 
		pSendData + pSendProp->GetOffset(),
		&recvProxyData.m_Value,
		0,
		objectID
		);

	// Fill in the data for the recv proxy.
	recvProxyData.m_pRecvProp = pRecvProp;
	recvProxyData.m_iElement = 0;
	recvProxyData.m_ObjectID = objectID;
	pRecvProp->GetProxyFn()( &recvProxyData, pRecvData, pRecvData + pRecvProp->GetOffset() );
}


// ---------------------------------------------------------------------------------------- //
// DecodeInfo implementation.
// ---------------------------------------------------------------------------------------- //

void DecodeInfo::CopyVars( const DecodeInfo *pOther )
{
	m_pStruct = pOther->m_pStruct;
	m_pData = pOther->m_pData;
	
	m_pRecvProp = pOther->m_pRecvProp;
	m_pProp = pOther->m_pProp;
	m_pIn = pOther->m_pIn;
	m_ObjectID = pOther->m_ObjectID;
	m_iElement = pOther->m_iElement;
}


void DecodeInfo::SetupForSkipping( const SendProp *pProp, bf_read *pIn )
{
	m_pStruct = 0;
	m_pData = 0;
	m_pRecvProp = 0;
	m_pProp = pProp;
	m_pIn = pIn;
	m_ObjectID = -9999;
}


// ---------------------------------------------------------------------------------------- //
// Int property type abstraction.
// ---------------------------------------------------------------------------------------- //

void Int_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	EncodeInt( pProp, pVar->m_Int, pOut );
}


void Int_Decode( DecodeInfo *pInfo )
{
	if(pInfo->m_pProp->GetFlags() & SPROP_UNSIGNED)
	{
		pInfo->m_Value.m_Int = pInfo->m_pIn->ReadUBitLong(pInfo->m_pProp->m_nBits);
		if ( pInfo->m_pRecvProp )
		{
			pInfo->m_pRecvProp->GetProxyFn()( pInfo, pInfo->m_pStruct, pInfo->m_pData );
		}
	}
	else
	{
		pInfo->m_Value.m_Int = pInfo->m_pIn->ReadSBitLong( pInfo->m_pProp->m_nBits );
		if ( pInfo->m_pRecvProp )
		{
			pInfo->m_pRecvProp->GetProxyFn()( pInfo, pInfo->m_pStruct, pInfo->m_pData );
		}
	}
}


int Int_CompareDeltas( const SendProp *pProp, bf_read *p1, bf_read *p2 )
{
	return p1->ReadUBitLong( pProp->m_nBits ) != p2->ReadUBitLong( pProp->m_nBits );
}


void Int_ShowRecvWatchInfo( DecodeInfo const *pInfo, const char *pPropName )
{
	PrintDebugInfoInt( pInfo->m_ObjectID, pPropName, pInfo->m_Value.m_Int );
}


void Int_ShowSendWatchInfo( DVariant const *pVar, SendProp const *pProp, int objectID )
{
	Con_DPrintf("DataTable Send: ent %d, var %s: %d\n", objectID, pProp->m_pVarName, pVar->m_Int);
}


const char* Int_GetTypeNameString()
{
	return "DPT_Int";
}


bool Int_IsAllZeros( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp )
{
	return (pVar->m_Int == 0);
}



// ---------------------------------------------------------------------------------------- //
// Float type abstraction.
// ---------------------------------------------------------------------------------------- //

void Float_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	EncodeFloat( pProp, pVar->m_Float, pOut, objectID );
}


void Float_Decode( DecodeInfo *pInfo )
{
	pInfo->m_Value.m_Float = DecodeFloat(pInfo->m_pProp, pInfo->m_pIn);

	if ( pInfo->m_pRecvProp )
		pInfo->m_pRecvProp->GetProxyFn()( pInfo, pInfo->m_pStruct, pInfo->m_pData );
}


int	Float_CompareDeltas( const SendProp *pProp, bf_read *p1, bf_read *p2 )
{
	if ( pProp->GetFlags() & SPROP_COORD )
	{
		return p1->ReadBitCoord() != p2->ReadBitCoord();
	}
	else if ( pProp->GetFlags() & SPROP_NOSCALE )
	{
		return p1->ReadUBitLong( 32 ) != p2->ReadUBitLong( 32 );
	}
	else if ( pProp->GetFlags() & SPROP_NORMAL )
	{
		return p1->ReadUBitLong( NORMAL_FRACTIONAL_BITS+1 ) != p2->ReadUBitLong( NORMAL_FRACTIONAL_BITS+1 );
	}
	else
	{
		return p1->ReadUBitLong( pProp->m_nBits ) != p2->ReadUBitLong( pProp->m_nBits );
	}
}


void Float_ShowRecvWatchInfo( DecodeInfo const *pInfo, const char *pPropName )
{
	PrintDebugInfo( pInfo->m_ObjectID, pPropName, pInfo->m_Value.m_Float );
}


void Float_ShowSendWatchInfo( DVariant const *pVar, SendProp const *pProp, int objectID )
{
	Con_DPrintf("DataTable Send: ent %d, var %s: %f\n", objectID, pProp->m_pVarName, pVar->m_Float);
}


const char* Float_GetTypeNameString()
{
	return "DPT_Float";
}


bool Float_IsAllZeros( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp )
{
	return (pVar->m_Float == 0);
}


// ---------------------------------------------------------------------------------------- //
// Vector type abstraction.
// ---------------------------------------------------------------------------------------- //

void Vector_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	EncodeFloat(pProp, pVar->m_Vector[0], pOut, objectID);
	EncodeFloat(pProp, pVar->m_Vector[1], pOut, objectID);

	// Don't write out the third component for normals
	if ((pProp->GetFlags() & SPROP_NORMAL) == 0)
	{
		EncodeFloat(pProp, pVar->m_Vector[2], pOut, objectID);
	}
	else
	{
		// Write a sign bit for z instead!
		int	signbit = (pVar->m_Vector[2] <= -NORMAL_RESOLUTION);
		pOut->WriteOneBit( signbit );
	}
}


void Vector_Decode(DecodeInfo *pInfo)
{
	float *v = pInfo->m_Value.m_Vector;

	v[0] = DecodeFloat(pInfo->m_pProp, pInfo->m_pIn);
	v[1] = DecodeFloat(pInfo->m_pProp, pInfo->m_pIn);

	// Don't read in the third component for normals
	if ((pInfo->m_pProp->GetFlags() & SPROP_NORMAL) == 0)
	{
		v[2] = DecodeFloat(pInfo->m_pProp, pInfo->m_pIn);
	}
	else
	{
		int signbit = pInfo->m_pIn->ReadOneBit();

		float v0v0v1v1 = v[0] * v[0] +
			v[1] * v[1];
		if (v0v0v1v1 < 1.0f)
			v[2] = sqrtf( 1.0f - v0v0v1v1 );
		else
			v[2] = 0.0f;

		if (signbit)
			v[2] *= -1.0f;
	}

	if( pInfo->m_pRecvProp )
		pInfo->m_pRecvProp->GetProxyFn()( pInfo, pInfo->m_pStruct, pInfo->m_pData );
}


int	Vector_CompareDeltas( const SendProp *pProp, bf_read *p1, bf_read *p2 )
{
	int c1 = Float_CompareDeltas( pProp, p1, p2 );
	int c2 = Float_CompareDeltas( pProp, p1, p2 );
	int c3;
	
	if ( pProp->GetFlags() & SPROP_NORMAL )
	{
		c3 = p1->ReadOneBit() != p2->ReadOneBit();
	}
	else
	{
		c3 = Float_CompareDeltas( pProp, p1, p2 );
	}

	return c1 | c2 | c3;
}


void Vector_ShowRecvWatchInfo( DecodeInfo const *pInfo, const char *pPropName)
{
	const float *v = (const float*)&pInfo->m_Value.m_Vector;
	Vector tmp( v[0], v[1], v[2] );
	PrintDebugInfo( pInfo->m_ObjectID, pPropName, tmp );
}


void Vector_ShowSendWatchInfo( DVariant const *pVar, SendProp const *pProp, int objectID )
{
	Con_DPrintf("DataTable Send: ent %d, var %s: (%.3f %.3f %.3f)\n", 
		objectID, pProp->m_pVarName, pVar->m_Vector[0], pVar->m_Vector[1], pVar->m_Vector[2]);
}


const char* Vector_GetTypeNameString()
{
	return "DPT_Vector";
}


bool Vector_IsAllZeros( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp )
{
	return ( pVar->m_Vector[0] == 0 ) && ( pVar->m_Vector[1] == 0 ) && ( pVar->m_Vector[2] == 0 );
}


// ---------------------------------------------------------------------------------------- //
// String type abstraction.
// ---------------------------------------------------------------------------------------- //

void String_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	// First count the string length, then do one WriteBits call.
	int len;
	for ( len=0; len < pProp->m_StringBufferLen; len++ )
	{
		if( pVar->m_pString[len] == 0 )
		{
			break;
		}
	}	

	Assert( len < DT_MAX_STRING_BUFFERSIZE );
		
	// Optionally write the length here so deltas can be compared faster.
	pOut->WriteUBitLong( len, DT_MAX_STRING_BITS );
	pOut->WriteBits( pVar->m_pString, len * 8 );
}


void String_Decode(DecodeInfo *pInfo)
{
	// Read it in.
	int maxLen = DT_MAX_STRING_BUFFERSIZE;
	if( maxLen > pInfo->m_pProp->m_StringBufferLen )
		maxLen = pInfo->m_pProp->m_StringBufferLen;

	int len = pInfo->m_pIn->ReadUBitLong( DT_MAX_STRING_BITS );

	char *tempStr = pInfo->m_TempStr;

	if ( len <= (maxLen-1) )
	{
		pInfo->m_pIn->ReadBits( tempStr, len*8 );
		tempStr[len] = 0;
	}
	else
	{
		pInfo->m_pIn->ReadBits( tempStr, (maxLen-1)*8 );
		tempStr[maxLen - 1] = 0;

		pInfo->m_pIn->SeekRelative( len - (maxLen-1) );
	}

	// Give it to the RecvProxy.
	if ( pInfo->m_pRecvProp )
	{
		pInfo->m_Value.m_pString = tempStr;
		pInfo->m_pRecvProp->GetProxyFn()( pInfo, pInfo->m_pStruct, pInfo->m_pData );
	}
}


// Compare the bits in pBuf1 and pBuf2 and return 1 if they are different.
// This must always seek both buffers to wherever they start at + nBits.
static inline int AreBitsDifferent( bf_read *pBuf1, bf_read *pBuf2, int nBits )
{
	int nDWords = nBits >> 5;

	int diff = 0;
	for ( int iDWord=0; iDWord < nDWords; iDWord++ )
	{
		diff |= (pBuf1->ReadUBitLong(32) != pBuf2->ReadUBitLong(32));
	}

	int nRemainingBits = nBits - (nDWords<<5);
	if (nRemainingBits > 0)
		diff |= pBuf1->ReadUBitLong( nRemainingBits ) != pBuf2->ReadUBitLong( nRemainingBits );
	
	return diff;
}


int String_CompareDeltas( const SendProp *pProp, bf_read *p1, bf_read *p2 )
{
	int len1 = p1->ReadUBitLong( DT_MAX_STRING_BITS );
	int len2 = p2->ReadUBitLong( DT_MAX_STRING_BITS );

	if ( len1 == len2 )
	{
		if (len1 == 0)
			return true;

		// Ok, they're short and fast.
		return AreBitsDifferent( p1, p2, len1*8 );
	}
	else
	{
		p1->SeekRelative( len1 * 8 );
		p2->SeekRelative( len2 * 8 );
		return true;
	}
}


void String_ShowRecvWatchInfo( DecodeInfo const *pInfo, const char *pPropName )
{
	PrintDebugInfo( pInfo->m_ObjectID, pPropName, pInfo->m_TempStr );
}


void String_ShowSendWatchInfo( DVariant const *pVar, SendProp const *pProp, int objectID )
{
	Con_DPrintf("DataTable Send: ent %d, var %s: (%s)\n", objectID, pProp->m_pVarName, pVar->m_pString);
}


const char* String_GetTypeNameString()
{
	return "DPT_String";
}


bool String_IsAllZeros( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp )
{
	return ( pVar->m_pString[0] == 0 );
}


// ---------------------------------------------------------------------------------------- //
// Array abstraction.
// ---------------------------------------------------------------------------------------- //

int Array_GetLength( const unsigned char *pStruct, const SendProp *pProp, int objectID )
{
	// Get the array length from the proxy.
	ArrayLengthSendProxyFn proxy = pProp->GetArrayLengthProxy();
	
	if ( proxy )
	{
		int nElements = proxy( pStruct, objectID );
		
		// Make sure it's not too big.
		if ( nElements > pProp->GetNumElements() )
		{
			Assert( false );
			nElements = pProp->GetNumElements();
		}

		return nElements;
	}
	else
	{	
		return pProp->GetNumElements();
	}
}


void Array_Encode( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp, bf_write *pOut, int objectID )
{
	SendProp *pArrayProp = pProp->GetArrayProp();
	AssertMsg( pArrayProp, ("Array_Encode: missing m_pArrayProp for SendProp '%s'.", pProp->m_pVarName) );
	
	int nElements = Array_GetLength( pStruct, pProp, objectID );

	// Write the number of elements.
	pOut->WriteUBitLong( nElements, pProp->GetNumArrayLengthBits() );

	unsigned char *pCurStructOffset = (unsigned char*)pStruct + pArrayProp->GetOffset();
	for ( int iElement=0; iElement < nElements; iElement++ )
	{
		DVariant var;

		// Call the proxy to get the value, then encode.
		pArrayProp->GetProxyFn()( pStruct, pCurStructOffset, &var, iElement, objectID );
		g_PropTypeFns[pArrayProp->GetType()].Encode( pStruct, &var, pArrayProp, pOut, objectID ); 
		
		pCurStructOffset += pProp->GetElementStride();
	}
}


void Array_Decode( DecodeInfo *pInfo )
{
	SendProp *pArrayProp = pInfo->m_pProp->GetArrayProp();
	AssertMsg( pArrayProp, ("Array_Decode: missing m_pArrayProp for a property.") );

	// Setup a DecodeInfo that is used to decode each of the child properties.
	DecodeInfo subDecodeInfo;
	subDecodeInfo.CopyVars( pInfo );
	subDecodeInfo.m_pProp = pArrayProp;

	int elementStride = 0;	
	ArrayLengthRecvProxyFn lengthProxy = 0;
	if ( pInfo->m_pRecvProp )
	{
		RecvProp *pArrayRecvProp = pInfo->m_pRecvProp->GetArrayProp();
		subDecodeInfo.m_pRecvProp = pArrayRecvProp;
		
		// Note we get the OFFSET from the array element property and the STRIDE from the array itself.
		subDecodeInfo.m_pData = (char*)pInfo->m_pData + pArrayRecvProp->GetOffset();
		elementStride = pInfo->m_pRecvProp->GetElementStride();
		Assert( elementStride != -1 ); // (Make sure it was set..)

		lengthProxy = pInfo->m_pRecvProp->GetArrayLengthProxy();
	}

	int nElements = pInfo->m_pIn->ReadUBitLong( pInfo->m_pProp->GetNumArrayLengthBits() );

	if ( lengthProxy )
		lengthProxy( pInfo->m_pStruct, pInfo->m_ObjectID, nElements );

	for ( subDecodeInfo.m_iElement=0; subDecodeInfo.m_iElement < nElements; subDecodeInfo.m_iElement++ )
	{
		g_PropTypeFns[pArrayProp->GetType()].Decode( &subDecodeInfo );
		subDecodeInfo.m_pData = (char*)subDecodeInfo.m_pData + elementStride;
	}
}


int Array_CompareDeltas( const SendProp *pProp, bf_read *p1, bf_read *p2 )
{
	SendProp *pArrayProp = pProp->GetArrayProp();
	AssertMsg( pArrayProp, ("Array_CompareDeltas: missing m_pArrayProp for SendProp '%s'.", pProp->m_pVarName) );

	int nLengthBits = pProp->GetNumArrayLengthBits(); 
	int length1 = p1->ReadUBitLong( nLengthBits );
	int length2 = p2->ReadUBitLong( nLengthBits );

	int bDifferent = length1 != length2;
	
	// Compare deltas on the props that are the same.
	int nSame = min( length1, length2 );
	for ( int iElement=0; iElement < nSame; iElement++ )
	{
		bDifferent |= g_PropTypeFns[pArrayProp->GetType()].CompareDeltas( pArrayProp, p1, p2 );
	}

	// Now just eat up the remaining properties in whichever buffer was larger.
	if ( length1 != length2 )
	{
		DecodeInfo decodeInfo;
		
		decodeInfo.SetupForSkipping( pArrayProp, (length1 > length2) ? p1 : p2 );

		int nExtra = max( length1, length2 ) - nSame;
		for ( int iEatUp=0; iEatUp < nExtra; iEatUp++ )
		{
			g_PropTypeFns[pArrayProp->GetType()].Decode( &decodeInfo );
		}
	}
	
	return bDifferent;
}


void Array_FastCopy( 
	const SendProp *pSendProp, 
	const RecvProp *pRecvProp, 
	const unsigned char *pSendData, 
	unsigned char *pRecvData, 
	int objectID )
{
	const RecvProp *pArrayRecvProp = pRecvProp->GetArrayProp();
	const SendProp *pArraySendProp = pSendProp->GetArrayProp();

	CRecvProxyData recvProxyData;
	recvProxyData.m_pRecvProp =	pArrayRecvProp;
	recvProxyData.m_ObjectID = objectID;

	// Find out the array length and call the RecvProp's array-length proxy.
	int nElements = Array_GetLength( pSendData, pSendProp, objectID );
	ArrayLengthRecvProxyFn lengthProxy = pRecvProp->GetArrayLengthProxy();
	if ( lengthProxy )
		lengthProxy( pRecvData, objectID, nElements );

	const unsigned char *pCurSendPos = pSendData + pArraySendProp->GetOffset();
	unsigned char *pCurRecvPos = pRecvData + pArrayRecvProp->GetOffset();
	for ( recvProxyData.m_iElement=0; recvProxyData.m_iElement < nElements; recvProxyData.m_iElement++ )
	{
		// Get this array element out of the sender's data.
		pArraySendProp->GetProxyFn()( pSendData, pCurSendPos, &recvProxyData.m_Value, recvProxyData.m_iElement, objectID );
		pCurSendPos += pSendProp->GetElementStride();
		
		// Write it into the receiver.
		pArrayRecvProp->GetProxyFn()( &recvProxyData, pRecvData, pCurRecvPos );
		pCurRecvPos += pRecvProp->GetElementStride();
	}
}


void Array_ShowRecvWatchInfo( DecodeInfo const *pInfo, const char *pPropName )
{
	Con_DPrintf( "Array Recv: ent %d, var %s\n", pInfo->m_ObjectID, pPropName );
}


void Array_ShowSendWatchInfo( DVariant const *pVar, SendProp const *pProp, int objectID )
{
	Con_DPrintf( "Array Send: ent %d, var %s\n", objectID, pProp->m_pVarName );
}


const char* Array_GetTypeNameString()
{
	return "DPT_Array";
}


bool Array_IsAllZeros( const unsigned char *pStruct, DVariant *pVar, const SendProp *pProp )
{
	int nElements = Array_GetLength( pStruct, pProp, -1 );
	return ( nElements == 0 );
}


// ---------------------------------------------------------------------------------------- //
// Datatable type abstraction.
// ---------------------------------------------------------------------------------------- //

const char* DataTable_GetTypeNameString()
{
	return "DPT_DataTable";
}



PropTypeFns g_PropTypeFns[DPT_NUMSendPropTypes] =
{
	// DPT_Int
	{
		Int_Encode,
		Int_Decode,
		Int_CompareDeltas,
		Generic_FastCopy,
		Int_ShowRecvWatchInfo,
		Int_ShowSendWatchInfo,
		Int_GetTypeNameString,
		Int_IsAllZeros
	},

	// DPT_Float
	{
		Float_Encode,
		Float_Decode,
		Float_CompareDeltas,
		Generic_FastCopy,
		Float_ShowRecvWatchInfo,
		Float_ShowSendWatchInfo,
		Float_GetTypeNameString,
		Float_IsAllZeros
	},

	// DPT_Vector
	{
		Vector_Encode,
		Vector_Decode,
		Vector_CompareDeltas,
		Generic_FastCopy,
		Vector_ShowRecvWatchInfo,
		Vector_ShowSendWatchInfo,
		Vector_GetTypeNameString,
		Vector_IsAllZeros
	},

	// DPT_String
	{
		String_Encode,
		String_Decode,
		String_CompareDeltas,
		Generic_FastCopy,
		String_ShowRecvWatchInfo,
		String_ShowSendWatchInfo,
		String_GetTypeNameString,
		String_IsAllZeros
	},

	// DPT_Array
	{
		Array_Encode,
		Array_Decode,
		Array_CompareDeltas,
		Array_FastCopy,
		Array_ShowRecvWatchInfo,
		Array_ShowSendWatchInfo,
		Array_GetTypeNameString,
		Array_IsAllZeros
	},

	// DPT_DataTable
	{
		0,
		0,
		0,
		0,
		0,
		0,
		DataTable_GetTypeNameString
	},
};
