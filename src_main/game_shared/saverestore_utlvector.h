//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef SAVERESTORE_UTLVECTOR_H
#define SAVERESTORE_UTLVECTOR_H

#include "utlvector.h"
#include "isaverestore.h"

#if defined( _WIN32 )
#pragma once
#endif

//-------------------------------------

template <int FIELD_TYPE>
class CTypedescDeducer
{
public:
	template <class UTLVECTOR>
	static datamap_t *Deduce( UTLVECTOR *p )
	{
		return NULL;
	}

};

template<> 
class CTypedescDeducer<FIELD_EMBEDDED>
{
public:
	template <class UTLVECTOR>
	static datamap_t *Deduce( UTLVECTOR *p )
	{
		return &UTLVECTOR::ElemType_t::m_DataMap;
	}

};

//-------------------------------------

template <class UTLVECTOR, int FIELD_TYPE>
class CUtlVectorDataOps : public CDefSaveRestoreOps
{
public:
	CUtlVectorDataOps()
	{
		COMPILE_TIME_ASSERT( 
			// FIELD_VOID
			FIELD_TYPE == FIELD_FLOAT ||
			FIELD_TYPE == FIELD_STRING ||
			FIELD_TYPE == FIELD_CLASSPTR ||
			FIELD_TYPE == FIELD_EHANDLE ||
			FIELD_TYPE == FIELD_EDICT ||
			FIELD_TYPE == FIELD_VECTOR ||
			FIELD_TYPE == FIELD_QUATERNION ||
			FIELD_TYPE == FIELD_POSITION_VECTOR ||
			FIELD_TYPE == FIELD_INTEGER ||
			// FIELD_FUNCTION
			FIELD_TYPE == FIELD_BOOLEAN ||
			FIELD_TYPE == FIELD_SHORT ||
			FIELD_TYPE == FIELD_CHARACTER ||
			FIELD_TYPE == FIELD_TIME ||
			FIELD_TYPE == FIELD_TICK ||
			FIELD_TYPE == FIELD_MODELNAME ||
			FIELD_TYPE == FIELD_SOUNDNAME ||
			FIELD_TYPE == FIELD_COLOR32 ||
			//FIELD_INPUT
			//FIELD_CUSTOM
			FIELD_EMBEDDED
		);
	}

	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{		
		datamap_t *pArrayTypeDatamap = CTypedescDeducer<FIELD_TYPE>::Deduce( (UTLVECTOR *)NULL );
		typedescription_t dataDesc = 
		{
			(fieldtype_t)FIELD_TYPE, 
			"elems", 
			{ 0, 0 },
			1, 
			FTYPEDESC_SAVE, 
			NULL, 
			NULL, 
			NULL,
			pArrayTypeDatamap,
			-1,
		};
		
		datamap_t dataMap = 
		{
			&dataDesc,
			1,
			"uv",
			NULL,
			false,
			false,
			0,
#ifdef DEBUG
			true
#endif
		};
		
		UTLVECTOR *pUtlVector = (UTLVECTOR *)fieldInfo.pField;
		int nElems = pUtlVector->Count();
		
		pSave->WriteInt( &nElems, 1 );
		if ( pArrayTypeDatamap == NULL )
		{
			if ( nElems )
			{
				dataDesc.fieldSize = nElems;
				dataDesc.fieldSizeInBytes = nElems * CDatamapFieldSizeDeducer<FIELD_TYPE>::FieldSize();
				pSave->WriteFields("elems", &((*pUtlVector)[0]), &dataMap, &dataDesc, 1 );
			}
		}
		else
		{
			// @Note (toml 11-21-02): Save load does not support arrays of user defined types (embedded)
			dataDesc.fieldSizeInBytes = CDatamapFieldSizeDeducer<FIELD_TYPE>::FieldSize();
			for ( int i = 0; i < nElems; i++ )
				pSave->WriteAll( &((*pUtlVector)[i]), &dataMap );
		}
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		datamap_t *pArrayTypeDatamap = CTypedescDeducer<FIELD_TYPE>::Deduce( (UTLVECTOR *)NULL );
		typedescription_t dataDesc = 
		{
			(fieldtype_t)FIELD_TYPE, 
			"elems", 
			{ 0, 0 },
			1, 
			FTYPEDESC_SAVE, 
			NULL, 
			NULL, 
			NULL,
			CTypedescDeducer<FIELD_TYPE>::Deduce( (UTLVECTOR *)NULL ),
			-1,
		};
		
		datamap_t dataMap = 
		{
			&dataDesc,
			1,
			"uv",
			NULL,
			false,
			false,
			0,
#ifdef DEBUG
			true
#endif
		};
		
		UTLVECTOR *pUtlVector = (UTLVECTOR *)fieldInfo.pField;

		int nElems = pRestore->ReadInt();
		
		pUtlVector->SetCount( nElems );
		if ( pArrayTypeDatamap == NULL )
		{
			if ( nElems )
			{
				dataDesc.fieldSize = nElems;
				dataDesc.fieldSizeInBytes = nElems * CDatamapFieldSizeDeducer<FIELD_TYPE>::FieldSize();
				pRestore->ReadFields("elems", &((*pUtlVector)[0]), &dataMap, &dataDesc, 1 );
			}
		}
		else
		{
			// @Note (toml 11-21-02): Save load does not support arrays of user defined types (embedded)
			dataDesc.fieldSizeInBytes = CDatamapFieldSizeDeducer<FIELD_TYPE>::FieldSize();
			for ( int i = 0; i < nElems; i++ )
				pRestore->ReadAll( &((*pUtlVector)[i]), &dataMap );
		}		
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		UTLVECTOR *pUtlVector = (UTLVECTOR *)fieldInfo.pField;
		pUtlVector->SetCount( 0 );
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		UTLVECTOR *pUtlVector = (UTLVECTOR *)fieldInfo.pField;
		return ( pUtlVector->Count() == 0 );
	}
	
};

//-------------------------------------

template <int FIELD_TYPE>
class CDataopsInstantiator
{
public:
	template <class UTLVECTOR>
	static ISaveRestoreOps *GetDataOps(UTLVECTOR *)
	{
		static CUtlVectorDataOps<UTLVECTOR, FIELD_TYPE> ops;
		return &ops;
	}
};

//-------------------------------------

#define SaveUtlVector( pSave, pUtlVector, fieldtype) \
	CDataopsInstantiator<fieldtype>::GetDataOps( pUtlVector )->Save( pUtlVector, pSave );

#define RestoreUtlVector( pRestore, pUtlVector, fieldtype) \
	CDataopsInstantiator<fieldtype>::GetDataOps( pUtlVector )->Restore( pUtlVector, pRestore );

//-------------------------------------

#define DEFINE_UTLVECTOR(type,name,fieldtype) \
	{ FIELD_CUSTOM, #name, { offsetof(type,name), 0 }, 1, FTYPEDESC_SAVE, NULL, CDataopsInstantiator<fieldtype>::GetDataOps(&(((type *)0)->name)), NULL }
	
#endif // SAVERESTORE_UTLVECTOR_H
