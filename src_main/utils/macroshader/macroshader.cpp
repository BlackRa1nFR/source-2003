//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include <windows.h>
#include "d3dx9.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <conio.h>
#include <ctype.h>
#include "UtlVector.h"
#include "UtlStringMap.h"
#include "UtlBuffer.h"

CUtlStringMap<int> g_Variables;
CUtlStringMap<int> g_AllocNameToRegister;

struct Combo
{
	int *m_pInt;
	int m_nMin;
	int m_nMax;
};

CUtlVector<Combo> g_Combos;

CUtlVector<bool> g_AllocatedRegisters;

enum NodeType
{
	CODETYPE_PLAIN,
	CODETYPE_IF,
	CODETYPE_ALLOC,
	CODETYPE_FREE,
};

enum CompareOp
{
	OP_LESSTHAN,
	OP_GREATERTHAN,
	OP_LESSTHANEQUAL,
	OP_GREATERTHANEQUAL,
	OP_EQUALS,
	OP_NOTEQUALS
};

char *g_OpNames[] = 
{
	"<", ">", "<=", ">=", "==", "!="
};

struct CodeNode
{
	int m_Type;
	int *m_pLeftIntValue;
	int *m_pRightIntValue;
	CompareOp m_Op;
	CUtlVector<char> m_Code;
	CodeNode *m_pChild;
	CodeNode *m_pSibling;
	CodeNode()
	{
		m_Type = CODETYPE_PLAIN;
		m_pChild = NULL;
		m_pSibling = NULL;
		m_pLeftIntValue = NULL;
		m_pRightIntValue = NULL;
	}
	CodeNode( NodeType type )
	{
		m_Type = type;
		m_pChild = NULL;
		m_pSibling = NULL;
		m_pLeftIntValue = NULL;
		m_pRightIntValue = NULL;
	}
};

const char *g_pInputFileName = NULL;
char *g_pInputFile = NULL;
char *g_pScan = NULL;
int g_nInputFileSize = 0;

CodeNode g_Root;

const char *GetLine( void )
{
	char *pStart = g_pScan;
	while( ( *g_pScan != 13 || *g_pScan == 10 ) && *g_pScan != '\0' )
	{
		g_pScan++;
	}
	if( *g_pScan == 13 )
	{
		*g_pScan = '\0';
		g_pScan++;
		if( *g_pScan == 10 )
		{
			g_pScan++;
		}
		return pStart;
	}
	if( pStart == g_pScan )
	{
		return NULL;
	}
	Assert( 0 );
	return NULL;
}

bool ParseCombo( const char *pLine )
{
	while( isspace( *pLine ) )
	{
		pLine++;
	}

	if( pLine[0] == '/' && pLine[1] == '/' )
	{
		pLine += 2;
	}
	else if( pLine[0] == '#' || pLine[0] == ';' )
	{
		pLine++;
	}
	else
	{
		return false;
	}

	while( isspace( *pLine ) )
	{
		pLine++;
	}

	if( *pLine != '\"' )
	{
		return false;
	}
	pLine++;

	const char *pStart = pLine;
	while( isalnum( *pLine ) )
	{
		pLine++;
	}

	char *pVarName = ( char * )stackalloc( pLine - pStart + 1 );
	strncpy( pVarName, pStart, pLine - pStart );
	pVarName[pLine - pStart] = '\0';

	if( *pLine != '\"' )
	{
		return false;
	}
	pLine++;

	if( !isspace( *pLine ) )
	{
		return false;
	}
	pLine++;

	while( isspace( *pLine ) )
	{
		pLine++;
	}

	if( *pLine != '\"' )
	{
		return false;
	}
	pLine++;

	pStart = pLine;

	while( isdigit( *pLine ) )
	{
		pLine++;
	}

	char *pMinString = ( char * )stackalloc( pLine - pStart + 1 );
	strncpy( pMinString, pStart, pLine - pStart );
	pMinString[pLine - pStart] = '\0';
	int min = atoi( pMinString );

	if( pLine[0] != '.' || pLine[1] != '.' )
	{
		return false;
	}
	pLine += 2;

	pStart = pLine;

	while( isdigit( *pLine ) )
	{
		pLine++;
	}

	char *pMaxString = ( char * )stackalloc( pLine - pStart + 1 );
	strncpy( pMaxString, pStart, pLine - pStart );
	pMaxString[pLine - pStart] = '\0';
	int max = atoi( pMaxString );

	if( *pLine != '\"' )
	{
		return false;
	}

	Combo newCombo;
	newCombo.m_pInt = &g_Variables[pVarName];
	newCombo.m_nMin = min;
	newCombo.m_nMax = max;

	g_Combos.AddToTail( newCombo );
	return true;
}

bool ParseComment( const char *pLine )
{
	while( isspace( *pLine ) )
	{
		pLine++;
	}

	if( ( pLine[0] == '/' && pLine[1] == '/' ) || pLine[0] == '#' || pLine[0] == ';' )
	{
		return true;
	}
	else
	{
		return false;
	}
}

// return true if line is an "if" statement
bool ParseIf( const char *pLine, int **ppLeftInt, int **ppRightInt, CompareOp &op )
{
	while( isspace( *pLine ) )
	{
		pLine++;
	}
	if( *pLine != 'I' )
	{
		return false;
	}
	pLine++;
	if( *pLine != 'F' )
	{
		return false;
	}
	pLine++;
	if( !isspace( *pLine ) )
	{
		return false;
	}
	pLine++;
	
	while( isspace( *pLine ) )
	{
		pLine++;
	}

	const char *pStart = pLine;
	if( !isalnum( *pLine ) )
	{
		Error( "expected isalnum\n" );
	}

	// get the left side of the compare
	while( isalnum( *pLine ) )
	{
		pLine++;
	}

	// done getting left side of compare here.
	char *pLeftName = ( char * )stackalloc( pLine - pStart + 1 );
	strncpy( pLeftName, pStart, pLine - pStart );
	pLeftName[pLine - pStart] = '\0';
	*ppLeftInt = &g_Variables[pLeftName];
	**ppLeftInt = atoi( pLeftName );

	while( isspace( *pLine ) )
	{
		pLine++;
	}

	// Get the operator
	if( pLine[0] == '<' )
	{
		if( pLine[1] == '=' )
		{
			op = OP_LESSTHANEQUAL;
			pLine += 2;
		}
		else
		{
			op = OP_LESSTHAN;
			pLine++;
		}
	}
	if( pLine[0] == '>' )
	{
		if( pLine[1] == '=' )
		{
			op = OP_GREATERTHANEQUAL;
			pLine += 2;
		}
		else
		{
			op = OP_GREATERTHAN;
			pLine++;
		}
	}
	else if( pLine[0] == '!' )
	{
		if( pLine[1] == '=' )
		{
			op = OP_NOTEQUALS;
			pLine += 2;
		}
		else
		{
			Error( "= expected" );
		}
	}
	else if( pLine[0] == '=' )
	{
		if( pLine[1] == '=' )
		{
			op = OP_EQUALS;
			pLine += 2;
		}
		else
		{
			Error( "= expected" );
		}
	}
	else
	{
		Error( "syntax error\n" );
	}

	while( isspace( *pLine ) )
	{
		pLine++;
	}

	// Get the right side
	pStart = pLine;
	if( !isalnum( *pLine ) )
	{
		Error( "expected alnum\n" );
	}

	// get the left side of the compare
	while( isalnum( *pLine ) )
	{
		pLine++;
	}

	// done getting right side of compare here.
	char *pRightName = ( char * )stackalloc( pLine - pStart + 1 );
	strncpy( pRightName, pStart, pLine - pStart );
	pLeftName[pLine - pStart] = '\0';
	*ppRightInt = &g_Variables[pRightName];
	**ppRightInt = atoi( pRightName );
	
	return true;
}

// return true if line is an endif
bool ParseEndIf( const char *pLine )
{
	while( isspace( *pLine ) )
	{
		pLine++;
	}
	if( strncmp( pLine, "ENDIF", 5 ) == 0 )
	{
		return true;
	}
	else
	{
		return false;
	}
}

// return true if line is an alloc statement
bool ParseAlloc( const char *pLine, char **ppVarName )
{
	char *pVarName = NULL;
	while( isspace( *pLine ) )
	{
		pLine++;
	}
	if( strncmp( pLine, "alloc", 5 ) == 0 )
	{
		pLine += 5;
		while( isspace( *pLine ) )
		{
			pLine++;
		}
		if( *pLine == 10 || *pLine == 13 )
		{
			Assert( 0 );
		}
		int len = strlen( pLine );
		pVarName = *ppVarName = new char[len+1];
		strcpy( pVarName, pLine );
		while( !isspace( *pVarName ) && *pVarName != '\0' )
		{
			pVarName++;
		}
		*pVarName = '\0';
		return true;
	}
	else
	{
		return false;
	}
}

// return true if line is a free statement
bool ParseFree( const char *pLine, char **ppVarName )
{
	char *pVarName = NULL;
	while( isspace( *pLine ) )
	{
		pLine++;
	}
	if( strncmp( pLine, "free", 4 ) == 0 )
	{
		pLine += 4;
		while( isspace( *pLine ) )
		{
			pLine++;
		}
		if( *pLine == 10 || *pLine == 13 )
		{
			Assert( 0 );
		}
		int len = strlen( pLine );
		pVarName = *ppVarName = new char[len+1];
		strcpy( pVarName, pLine );
		while( !isspace( *pVarName ) && *pVarName != '\0' )
		{
			pVarName++;
		}
		*pVarName = '\0';
		return true;
	}
	else
	{
		return false;
	}
}

void AddCode( CUtlVector<char> &code, const char *pCode, bool bNewLine = true, bool bNullTerminate = false )
{
	code.AddMultipleToTail( strlen( pCode ), pCode );
	if( bNewLine )
	{
		code.AddToTail( 13 );
		code.AddToTail( 10 );
	}
	if( bNullTerminate )
	{
		code.AddToTail( 0 );
	}
}

void ParseFile( CodeNode *pNode, int nestLevel = 0 )
{
	const char *pLine;
	int *pLeftInt, *pRightInt;
	CompareOp op;
	while( ( pLine = GetLine() ) != NULL )
	{
		char *pVarName;
		
		if( ParseCombo( pLine ) )
		{
			continue;
		}
		else if( ParseComment( pLine ) )
		{
			continue;
		}
		else if( ParseIf( pLine, &pLeftInt, &pRightInt, op ) )
		{
			pNode->m_pSibling = new CodeNode( CODETYPE_IF );
			pNode->m_pSibling->m_pLeftIntValue = pLeftInt;
			pNode->m_pSibling->m_pRightIntValue = pRightInt;
			pNode->m_pSibling->m_Op = op;
			pNode->m_pSibling->m_pChild = new CodeNode();
			ParseFile( pNode->m_pSibling->m_pChild, nestLevel + 1 );
			pNode->m_pSibling->m_pSibling = new CodeNode();
			pNode = pNode->m_pSibling->m_pSibling;
		}
		else if( ParseEndIf( pLine ) )
		{
			return;
		}
		else if( ParseAlloc( pLine, &pVarName ) )
		{
			pNode->m_pSibling = new CodeNode( CODETYPE_ALLOC );
			pNode = pNode->m_pSibling;
			AddCode( pNode->m_Code, pVarName, false, true );
			delete [] pVarName;
			pNode->m_pSibling = new CodeNode();
			pNode = pNode->m_pSibling;
		}
		else if( ParseFree( pLine, &pVarName ) )
		{
			pNode->m_pSibling = new CodeNode( CODETYPE_FREE );
			pNode = pNode->m_pSibling;
			AddCode( pNode->m_Code, pVarName, false, true );
			delete [] pVarName;
			pNode->m_pSibling = new CodeNode();
			pNode = pNode->m_pSibling;
		}
		else
		{
			AddCode( pNode->m_Code, pLine );
		}
	}
}

void Usage( void )
{
	fprintf( stderr, "Usage: macroshader blah.vsh\n" );
	exit( -1 );
}

void FileNotFound( void )
{
	fprintf( stderr, "File not found: \"%s\"\n", g_pInputFileName );
	exit( -1 );
}

void ReadInputFile( void )
{
	struct	_stat buf;
	if( _stat( g_pInputFileName, &buf ) != -1 )
	{
		g_nInputFileSize = buf.st_size;
	}
	else
	{
		FileNotFound();
	}
	
	FILE *fp = fopen( g_pInputFileName, "rb" );
	if( !fp )
	{
		FileNotFound();
	}

	g_pInputFile = new char[g_nInputFileSize+1];

	fread( g_pInputFile, g_nInputFileSize, 1, fp );
	g_pInputFile[g_nInputFileSize] = '\0';

	fclose( fp );
}

inline void PrintSpace( int i )
{
/*
	for( ; i; i-- )
	{
		printf( "\t" );
	}
*/
}

bool EvaluateIfStatement( CodeNode *pNode )
{
	Assert( pNode->m_Type == CODETYPE_IF );
	switch( pNode->m_Op )
	{
	case OP_LESSTHAN:
		return *pNode->m_pLeftIntValue < *pNode->m_pRightIntValue;
	case OP_GREATERTHAN:
		return *pNode->m_pLeftIntValue > *pNode->m_pRightIntValue;
	case OP_LESSTHANEQUAL:
		return *pNode->m_pLeftIntValue <= *pNode->m_pRightIntValue;
	case OP_GREATERTHANEQUAL:
		return *pNode->m_pLeftIntValue >= *pNode->m_pRightIntValue;
	case OP_EQUALS:
		return *pNode->m_pLeftIntValue == *pNode->m_pRightIntValue;
	case OP_NOTEQUALS:
		return *pNode->m_pLeftIntValue != *pNode->m_pRightIntValue;
	default:
		Assert( 0 );
		return false;
	}
}

int AllocateRegister( void )
{
	int i;
	for( i = 0; i < g_AllocatedRegisters.Count(); i++ )
	{
		if( g_AllocatedRegisters[i] == false )
		{
			g_AllocatedRegisters[i] = true;
			return i;
		}
	}
	g_AllocatedRegisters.AddToTail( true );
	return i;
}

void FreeRegister( int reg )
{
	Assert( g_AllocatedRegisters[reg] == true );
	g_AllocatedRegisters[reg] = false;
}

void ReplaceRegistersAndEmitString( const char *pString, CUtlBuffer &asmCode )
{
	const char *pStart = pString;
	while( *pString != '\0' )
	{
		while( *pString != '\0' && *pString != '$' )
		{
			pString++;
		}
		if( pString - pStart > 0 )
		{
			char *tmp = ( char * )stackalloc( pString - pStart + 1 );
			strncpy( tmp, pStart, pString - pStart );
			tmp[pString - pStart] = '\0';
			asmCode.Printf( "%s", tmp );
		}
		if( *pString == '$' )
		{
			const char *pNameStart = pString;
			pString++;
			while( isalnum( *pString ) )
			{
				pString++;
			}
			Assert( pString - pNameStart > 0 );
			char *tmp = ( char * )stackalloc( pString - pNameStart + 1 );
			strncpy( tmp, pNameStart, pString - pNameStart );
			tmp[pString - pNameStart] = '\0';
			if( g_AllocNameToRegister.Defined( tmp ) )
			{
				asmCode.Printf( "r%d", g_AllocNameToRegister[tmp] );
			}
			else
			{
				Assert( 0 );
			}
			pStart = pString;
		}
	}
}

void EmitAsmCode( CodeNode *pNode, int depth, CUtlBuffer &asmCode )
{
	while( pNode )
	{
		if( pNode->m_Type == CODETYPE_ALLOC )
		{
			int reg = AllocateRegister();
//			asmCode.Printf( "#define %s r%d\n", &pNode->m_Code[0], reg );
			g_AllocNameToRegister[&pNode->m_Code[0]] = reg;
		}
		else if( pNode->m_Type == CODETYPE_FREE )
		{
//			asmCode.Printf( "#undef %s\n", &pNode->m_Code[0] );
			FreeRegister( g_AllocNameToRegister[&pNode->m_Code[0]] );
		}
		else if( pNode->m_Type == CODETYPE_IF )
		{
			if( EvaluateIfStatement( pNode ) )
			{
				if( pNode->m_pChild )
				{
					EmitAsmCode( pNode->m_pChild, depth + 1, asmCode );
				}
			}
		}
		if( pNode->m_Type == CODETYPE_PLAIN && pNode->m_Code.Count() > 1 )
		{
			PrintSpace( depth );
//			asmCode.Printf( "%s", &pNode->m_Code[0] );
			ReplaceRegistersAndEmitString( ( const char * )&pNode->m_Code[0], asmCode );
		}
		pNode = pNode->m_pSibling;
	}
}

int CalcNumCombos( void )
{
	int numCombos = 1;
	int i;
	for( i = 0; i < g_Combos.Count(); i++ )
	{
		numCombos *= g_Combos[i].m_nMax - g_Combos[i].m_nMin + 1;
	}
	return numCombos;
}

void SetVarsForCombo( int combo )
{
	int i;
	for( i = 0; i < g_Combos.Count(); i++ )
	{
		int val = ( combo % ( g_Combos[i].m_nMax - g_Combos[i].m_nMin + 1 ) ) + g_Combos[i].m_nMin;
		*(g_Combos[i].m_pInt) = val;
		combo = combo / ( g_Combos[i].m_nMax - g_Combos[i].m_nMin + 1 );
	}
}

void NullTerminateCode( CodeNode *pNode )
{
	while( pNode )
	{
		pNode->m_Code.AddToTail( 0 );
		if( pNode->m_pChild )
		{
			NullTerminateCode( pNode->m_pChild );
		}
		pNode = pNode->m_pSibling;
	}
}

int main( int argc, char **argv )
{
	if( argc != 2 )
	{
		Usage();
	}
	g_pInputFileName = argv[1];
	ReadInputFile();

	g_pScan = g_pInputFile;
	ParseFile( &g_Root );

	NullTerminateCode( &g_Root );

	int numCombos = CalcNumCombos();
	
	int i;
	for( i = 0; i < numCombos; i++ )
	{
		CUtlBuffer outputCode( 0, 0, true );
		SetVarsForCombo( i );
		EmitAsmCode( &g_Root, 0, outputCode );

		fwrite( outputCode.Base(), outputCode.TellPut(), 1, stdout );

#if 1
		LPD3DXBUFFER pByteCodeBuffer, pErrorMessages;

//		DWORD flags = D3DXSHADER_DEBUG;
		DWORD flags = 0;
		const char *pTerd = ( const char * )outputCode.Base();
		
		HRESULT hr = D3DXAssembleShader( ( const char * )outputCode.Base(), outputCode.TellPut(), 
			NULL /*pDefines*/, NULL /*pInclude*/, flags, &pByteCodeBuffer, &pErrorMessages );
		if( pErrorMessages )
		{
			char *pErrorText = ( char * )pErrorMessages->GetBufferPointer();
			int size = pErrorMessages->GetBufferSize();
			fwrite( pErrorText, size, 1, stdout );
		}
		switch( hr )
		{
		case D3D_OK:
			break;
		case D3DERR_INVALIDCALL:
			printf( "D3DERR_INVALIDCALL\n" );
			break;
		case D3DXERR_INVALIDDATA:
			printf( "D3DXERR_INVALIDDATA\n" );
			break;
		case E_OUTOFMEMORY:
			printf( "E_OUTOFMEMORY\n" );
			break;
		}
#endif

		//		printf( "//--------------\n" );
	}

	getch();
	return 0;
}
