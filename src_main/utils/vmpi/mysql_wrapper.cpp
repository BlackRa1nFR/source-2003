//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <windows.h>
extern "C"
{
	#include "mysql.h"
};

#include "mysql_wrapper.h"
#include <stdio.h>


// -------------------------------------------------------------------------------------------------------- //
// CMySQLQuery implementation.
// -------------------------------------------------------------------------------------------------------- //

void CMySQLQuery::Format( const char *pFormat, ... )
{
	#define QUERYTEXT_GROWSIZE	1024

	// This keeps growing the buffer and calling _vsnprintf until the buffer is 
	// large enough to hold all the data.
	m_QueryText.SetSize( QUERYTEXT_GROWSIZE );
	while ( 1 )
	{
		va_list marker;
		va_start( marker, pFormat );
		int ret = _vsnprintf( m_QueryText.Base(), m_QueryText.Count(), pFormat, marker );
		va_end( marker );

		if ( ret < 0 )
		{
			m_QueryText.SetSize( m_QueryText.Count() + QUERYTEXT_GROWSIZE );
		}
		else
		{
			m_QueryText[ m_QueryText.Count() - 1 ] = 0;
			break;
		}
	}
}


// -------------------------------------------------------------------------------------------------------- //
// CMySQL class.
// -------------------------------------------------------------------------------------------------------- //

class CMySQL : public IMySQL
{
public:
							CMySQL( MYSQL *pSQL );
	virtual					~CMySQL();

	virtual void			Release();
	virtual int				Execute( const char *pString );
	virtual int				Execute( CMySQLQuery &query );
	virtual unsigned long	InsertID();
	virtual int				NumFields();
	virtual const char*		GetFieldName( int iColumn );
	virtual bool			NextRow();
	virtual bool			SeekToFirstRow();
	virtual CColumnValue	GetColumnValue( int iColumn );
	virtual CColumnValue	GetColumnValue( const char *pColumnName );
	virtual int				GetColumnIndex( const char *pColumnName );

	// Cancels the storage of the rows from the latest query.
	void					CancelIteration();


public:

	MYSQL		*m_pSQL;
	MYSQL_RES	*m_pResult;
	MYSQL_ROW	m_Row;
	CUtlVector<MYSQL_FIELD>	m_Fields;
};



// -------------------------------------------------------------------------------------------------------- //
// CMySQL implementation.
// -------------------------------------------------------------------------------------------------------- //

CMySQL::CMySQL( MYSQL *pSQL )
{
	m_pSQL = pSQL;
	m_pResult = NULL;
	m_Row = NULL;
}


CMySQL::~CMySQL()
{
	CancelIteration();

	if ( m_pSQL )
	{
		mysql_close( m_pSQL );
		m_pSQL = NULL;
	}
}


void CMySQL::Release()
{
	delete this;
}


int CMySQL::Execute( const char *pString )
{
	CancelIteration();

	int result = mysql_query( m_pSQL, pString );
	if ( result == 0 )
	{
		// Is this a query with a result set?
		m_pResult = mysql_store_result( m_pSQL );
		if ( m_pResult )
		{
			// Store the field information.
			int count = mysql_field_count( m_pSQL );
			MYSQL_FIELD *pFields = mysql_fetch_fields( m_pResult );
			m_Fields.CopyArray( pFields, count );
			return 0;
		}
		else
		{
			// No result set. Was a set expected?
			if ( mysql_field_count( m_pSQL ) != 0 )
				return 1;	// error! The query expected data but didn't get it.
		}
	}
	else
	{
		const char *pError = mysql_error( m_pSQL );
		pError = pError;
	}

	return result;
}


int CMySQL::Execute( CMySQLQuery &query )
{
	int retVal = Execute( query.m_QueryText.Base() );
	query.m_QueryText.Purge();
	return retVal;
}


unsigned long CMySQL::InsertID()
{
	return mysql_insert_id( m_pSQL );
}


int CMySQL::NumFields()
{
	return m_Fields.Count();
}


const char* CMySQL::GetFieldName( int iColumn )
{
	return m_Fields[iColumn].name;
}


bool CMySQL::NextRow()
{
	if ( !m_pResult )
		return false;

	m_Row = mysql_fetch_row( m_pResult );
	if ( m_Row == 0 )
	{
		return false;
	}
	else
	{
		return true;
	}
}


bool CMySQL::SeekToFirstRow()
{
	if ( !m_pResult )
		return false;

	mysql_data_seek( m_pResult, 0 );
	return true;
}


CColumnValue CMySQL::GetColumnValue( int iColumn )
{
	return CColumnValue( this, iColumn );
}


CColumnValue CMySQL::GetColumnValue( const char *pColumnName )
{
	return CColumnValue( this, GetColumnIndex( pColumnName ) );
}


int CMySQL::GetColumnIndex( const char *pColumnName )
{
	for ( int i=0; i < m_Fields.Count(); i++ )
	{
		if ( stricmp( pColumnName, m_Fields[i].name ) == 0 )
		{
			return i;
		}
	}
	
	return -1;
}


void CMySQL::CancelIteration()
{
	m_Fields.Purge();
	
	if ( m_pResult )
	{
		mysql_free_result( m_pResult );
		m_pResult = NULL;
	}

	m_Row = NULL;
}


// -------------------------------------------------------------------------------------------------------- //
// Module interface.
// -------------------------------------------------------------------------------------------------------- //

IMySQL* InitMySQL( const char *pDBName, const char *pHostName, const char *pUserName, const char *pPassword )
{
	MYSQL *pSQL = mysql_init( NULL );
	if ( !pSQL )
		return NULL;

	if ( !mysql_real_connect( pSQL, pHostName, pUserName, pPassword, pDBName, 0, NULL, 0 ) )
	{
		const char *pError = mysql_error( pSQL );
		pError = pError;

		mysql_close( pSQL );
		return NULL;
	}

	return new CMySQL( pSQL );
}



// -------------------------------------------------------------------------------------------------------- //
// CColumnValue implementation.
// -------------------------------------------------------------------------------------------------------- //

const char* CColumnValue::String()
{
	if ( m_pSQL->m_Row && m_iColumn >= 0 && m_iColumn < m_pSQL->m_Fields.Count() )
		return m_pSQL->m_Row[m_iColumn];
	else
		return "";
}

long CColumnValue::Int32()
{
	return atoi( String() );
}

unsigned long CColumnValue::UInt32()
{
	return (unsigned long)atoi( String() );
}




