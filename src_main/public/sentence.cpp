#include <assert.h>
#include "commonmacros.h"
#include "basetypes.h"
#include "sentence.h"
#include "utlbuffer.h"
#include <stdlib.h>
#include "vector.h"
#include "mathlib.h"
#include <ctype.h>

//-----------------------------------------------------------------------------
// Purpose: converts an english string to unicode
//-----------------------------------------------------------------------------
int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSize);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWordTag::CWordTag( void )
{
	m_pszWord = NULL;

	m_uiStartByte = 0;
	m_uiEndByte = 0;

	m_flStartTime = 0.0f;
	m_flEndTime = 0.0f;

	m_bSelected = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : from - 
//-----------------------------------------------------------------------------
CWordTag::CWordTag( const CWordTag& from )
{
	m_pszWord = NULL;
	SetWord( from.m_pszWord );

	m_uiStartByte = from.m_uiStartByte;
	m_uiEndByte = from.m_uiEndByte;

	m_flStartTime = from.m_flStartTime;
	m_flEndTime = from.m_flEndTime;

	m_bSelected = from.m_bSelected;

	for ( int p = 0; p < from.m_Phonemes.Size(); p++ )
	{
		CPhonemeTag *newPhoneme = new CPhonemeTag( *from.m_Phonemes[ p ] );
		m_Phonemes.AddToTail( newPhoneme );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *word - 
//-----------------------------------------------------------------------------
CWordTag::CWordTag( const char *word )
{
	m_uiStartByte = 0;
	m_uiEndByte = 0;

	m_flStartTime = 0.0f;
	m_flEndTime = 0.0f;

	m_pszWord = NULL;

	m_bSelected = false;

	SetWord( word );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWordTag::~CWordTag( void )
{
	delete[] m_pszWord;

	while ( m_Phonemes.Size() > 0 )
	{
		delete m_Phonemes[ 0 ];
		m_Phonemes.Remove( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tag - 
// Output : int
//-----------------------------------------------------------------------------
int CWordTag::IndexOfPhoneme( CPhonemeTag *tag )
{
	for ( int i = 0 ; i < m_Phonemes.Size(); i++ )
	{
		CPhonemeTag *p = m_Phonemes[ i ];
		if ( p == tag )
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *word - 
//-----------------------------------------------------------------------------
void CWordTag::SetWord( const char *word )
{
	delete[] m_pszWord;
	m_pszWord = new char[ strlen( word ) + 1 ];
	strcpy( m_pszWord, word );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPhonemeTag::CPhonemeTag( void )
{
	m_szPhoneme[ 0 ] = 0;

	m_uiStartByte = 0;
	m_uiEndByte = 0;

	m_flStartTime = 0.0f;
	m_flEndTime = 0.0f;

	m_flVolume = 1.0f;

	m_bSelected = false;

	m_nPhonemeCode = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : from - 
//-----------------------------------------------------------------------------
CPhonemeTag::CPhonemeTag( const CPhonemeTag& from )
{
	m_uiStartByte = from.m_uiStartByte;
	m_uiEndByte = from.m_uiEndByte;

	m_flStartTime = from.m_flStartTime;
	m_flEndTime = from.m_flEndTime;

	m_flVolume = from.m_flVolume;

	m_bSelected = from.m_bSelected;

	m_nPhonemeCode = from.m_nPhonemeCode;

	strcpy( m_szPhoneme, from.m_szPhoneme );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *phoneme - 
//-----------------------------------------------------------------------------
CPhonemeTag::CPhonemeTag( const char *phoneme )
{
	m_uiStartByte = 0;
	m_uiEndByte = 0;

	m_flStartTime = 0.0f;
	m_flEndTime = 0.0f;

	m_flVolume = 1.0f;

	m_bSelected = false;

	m_nPhonemeCode = 0;

	SetTag( phoneme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPhonemeTag::~CPhonemeTag( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *phoneme - 
//-----------------------------------------------------------------------------
void CPhonemeTag::SetTag( const char *phoneme )
{
	assert( strlen( phoneme ) <= MAX_PHONEME_LENGTH - 1 );
	strcpy( m_szPhoneme, phoneme );
}

//-----------------------------------------------------------------------------
// Purpose: Simple language to string and string to language lookup dictionary
//-----------------------------------------------------------------------------
struct CCLanguage
{
	int			type;
	char const	*name;
};

static CCLanguage g_CCLanguageLookup[] =
{
	{ CC_ENGLISH,	"english" },
	{ CC_FRENCH,	"french" },
	{ CC_GERMAN,	"german" },
	{ CC_SPANISH,	"spanish" },
	{ CC_ITALIAN,	"italian" },
	{ CC_KOREAN,	"korean" },
	{ CC_JAPANESE,	"japanese" },
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : language - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CCloseCaptionPhrase::NameForLanguage( int language )
{
	if ( language < 0 || language >= CC_NUM_LANGUAGES )
		return "unknown_language";

	CCLanguage *entry = &g_CCLanguageLookup[ language ];
	Assert( entry->type == language );
	return entry->name;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CCloseCaptionPhrase::LanguageForName( char const *name )
{
	int l;
	for ( l = 0; l < CC_NUM_LANGUAGES; l++ )
	{
		CCLanguage *entry = &g_CCLanguageLookup[ l ];
		Assert( entry->type == l );
		if ( !stricmp( entry->name, name ) )
			return l;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCloseCaptionPhrase::CCloseCaptionPhrase( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : from - 
//-----------------------------------------------------------------------------
CCloseCaptionPhrase::CCloseCaptionPhrase( const CCloseCaptionPhrase& from )
{
	m_pszStream = NULL;
	SetStream( from.m_pszStream );
	m_flStartTime = from.m_flStartTime;
	m_flEndTime = from.m_flEndTime;
	m_bSelected = from.m_bSelected;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *word - 
//-----------------------------------------------------------------------------
CCloseCaptionPhrase::CCloseCaptionPhrase( const wchar_t *phrase )
{
	Init();
	SetStream( phrase );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCloseCaptionPhrase::~CCloseCaptionPhrase( void )
{
	delete[] m_pszStream;
}

void CCloseCaptionPhrase::Init( void )
{
	m_flStartTime = 0.0f;
	m_flEndTime = 0.0f;
	m_pszStream = NULL;
	m_bSelected = false;
}

void CCloseCaptionPhrase::SetStream( const wchar_t *stream )
{
	delete[] m_pszStream;
	m_pszStream = new wchar_t[ wcslen( stream ) + 1 ];
	wchar_t const *in;
	wchar_t *out;
	in = stream;
	out = m_pszStream;

	// Removing leading spaces
	while ( *in && *in == L' ' )
	{
		in++;
	}

	// Copy body
	while ( *in )
	{
		*out++ = *in++;
	}

	// Terminate
	*out = L'\0';

	// Back up one character and check for and remove trailing spaces
	--out;
	while ( *out == L' ' && out >= m_pszStream )
	{
		*out-- = L'\0';
	}
}

wchar_t const *CCloseCaptionPhrase::GetStream( void ) const
{
	return m_pszStream ? m_pszStream : L"";
}

void CCloseCaptionPhrase::SetSelected( bool selected )
{
	m_bSelected = selected;
}

bool CCloseCaptionPhrase::GetSelected( void ) const
{
	return m_bSelected;
}

void CCloseCaptionPhrase::SetStartTime( float st )
{
	m_flStartTime = st;
}

float CCloseCaptionPhrase::GetStartTime( void ) const
{
	return m_flStartTime;
}

void CCloseCaptionPhrase::SetEndTime( float et )
{
	m_flEndTime = et;
}

float CCloseCaptionPhrase::GetEndTime( void ) const
{
	return m_flEndTime;
}

int CCloseCaptionPhrase::CountTokens( void ) const
{
	int count = 0;
	wchar_t token[ 1024 ];
	wchar_t const *in = GetStream();

	while ( in )
	{
		in = ParseToken( in, token, sizeof( token ) );
		if ( wcslen( token ) > 0 )
		{
			count++;
		}
		if ( !in || *in == L'\0' )
			break;
	}

	return count;
}

wchar_t const *CCloseCaptionPhrase::ParseToken( wchar_t const *in, wchar_t *token, int tokensize ) const
{
	if ( !in )
		return NULL;

	// Strip leading spaces
	while ( *in != L'\0' && *in == L' ' )
	{
		in++;
	}

	if ( *in == L'\0' )
		return NULL;

	wchar_t *out = token;

	// Check specially for command token
	if ( *in == L'<' )
	{
		while ( *in && *in != L'>' )
		{
			*out++ = *in++;
		}

		if ( !*in )
		{
			*out = L'\0';
			return NULL;
		}

		*out++ = *in++;
		*out = L'\0';
		return in;
	}
	
	if ( iswpunct( *in ) )
	{
		*out++ = *in++;
		*out = L'\0';
		return in;
	}

	while ( *in && *in != L' ' && *in != L'<' && !iswpunct( *in ) )
	{
		*out++ = *in++;
	}

	*out = L'\0';

	return in;
}

wchar_t const *CCloseCaptionPhrase::GetToken( int token_number ) const
{
	int count = 0;
	static wchar_t token[ 1024 ];
	wchar_t const *in = GetStream();

	token[ 0 ] = L'\0';

	while ( in )
	{
		in = ParseToken( in, token, sizeof( token ) );
		if ( wcslen( token ) > 0 )
		{
			if ( count == token_number )
				break;
			count++;
		}
		if ( !in || *in == L'\0' )
			break;
	}

	return token;
}

bool CCloseCaptionPhrase::SplitCommand( wchar_t const *in, wchar_t *cmd, wchar_t *args ) const
{
	if ( in[0] != L'<' )
		return false;

	args[ 0 ] = 0;
	cmd[ 0 ]= 0;
	wchar_t *out = cmd;
	in++;
	while ( *in != L'\0' && *in != L':' && *in != L'>' )
	{
		*out++ = *in++;
	}
	*out = L'\0';

	if ( *in != L':' )
		return true;

	in++;
	out = args;
	while ( *in != L'\0' && *in != L'>' )
	{
		*out++ = *in++;
	}
	*out = L'\0';

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSentence::CSentence( void )
{
	m_nResetWordBase = 0;
	m_szText[ 0 ] = 0;
	m_bShouldVoiceDuck = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSentence::~CSentence( void )
{
	Reset();
}


void CSentence::ParsePlaintext( CUtlBuffer& buf )
{
	char token[ 4096 ];
	char text[ 4096 ];
	text[ 0 ] = 0;
	while ( 1 )
	{
		buf.GetString( token );
		if ( !stricmp( token, "}" ) )
			break;

		strcat( text, token );
		strcat( text, " " );
	}

	SetText( text );
}

void CSentence::ParseWords( CUtlBuffer& buf )
{
	char token[ 4096 ];
	char word[ 256 ];
	float start, end;

	while ( 1 )
	{
		buf.GetString( token );
		if ( !stricmp( token, "}" ) )
			break;

		if ( stricmp( token, "WORD" ) )
			break;

		buf.GetString( token );
		strcpy( word, token );

		buf.GetString( token );
		start = atof( token );
		buf.GetString( token );
		end = atof( token );

		CWordTag *wt = new CWordTag( word );
		assert( wt );
		wt->m_flStartTime = start;
		wt->m_flEndTime = end;

		AddWordTag( wt );

		buf.GetString( token );
		if ( stricmp( token, "{" ) )
			break;

		while ( 1 )
		{
			buf.GetString( token );
			if ( !stricmp( token, "}" ) )
				break;

			// Parse phoneme
			int code;
			char phonemename[ 256 ];
			float start, end;
			float volume;

			code = atoi( token );

			buf.GetString( token );
			strcpy( phonemename, token );
			buf.GetString( token );
			start = atof( token );
			buf.GetString( token );
			end = atof( token );
			buf.GetString( token );
			volume = atof( token );

			CPhonemeTag *pt = new CPhonemeTag();
			assert( pt );
			pt->m_nPhonemeCode = code;
			strcpy( pt->m_szPhoneme, phonemename );
			pt->m_flStartTime = start;
			pt->m_flEndTime = end;
			pt->m_flVolume = volume;

			AddPhonemeTag( wt, pt );
		}
	}
}

void CSentence::ParseEmphasis( CUtlBuffer& buf )
{
	char token[ 4096 ];
	while ( 1 )
	{
		buf.GetString( token );
		if ( !stricmp( token, "}" ) )
			break;

		char t[ 256 ];
		strcpy( t, token );
		buf.GetString( token );

		char value[ 256 ];
		strcpy( value, token );

		CEmphasisSample sample;
		sample.selected = false;
		sample.time = atof( t );
		sample.value = atof( value );


		m_EmphasisSamples.AddToTail( sample );

	}
}

void CSentence::ParseCloseCaption( CUtlBuffer& buf )
{
	char token[ 4096 ];
	while ( 1 )
	{
		// Format is 
		// language_name
		// {
		//   PHRASE char streamlength "streambytes" starttime endtime
		//   PHRASE unicode streamlength "streambytes" starttime endtime
		// }
		buf.GetString( token );
		if ( !stricmp( token, "}" ) )
			break;

		char language_name[ 128 ];
		int language_id;
		strcpy( language_name, token );
		language_id = CCloseCaptionPhrase::LanguageForName( language_name );
		Assert( language_id != -1 );

		buf.GetString( token );
		if ( stricmp( token, "{" ) )
			break;

		buf.GetString( token );
		while ( 1 )
		{

			if ( !stricmp( token, "}" ) )
				break;

			if ( stricmp( token, "PHRASE" ) )
				break;

			char cc_type[32];
			char cc_stream[ 512 ];
			int cc_length;
			float start, end;

			memset( cc_stream, 0, sizeof( cc_stream ) );

			buf.GetString( token );
			strcpy( cc_type, token );

			bool unicode = false;
			if ( !stricmp( cc_type, "unicode" ) )
			{
				unicode = true;
			}
			else if ( stricmp( cc_type, "char" ) )
			{
				Assert( 0 );
			}

			buf.GetString( token );
			cc_length = atoi( token );
			Assert( cc_length >= 0 && cc_length < sizeof( cc_stream ) );
			// Skip space
			buf.GetChar();
			buf.Get( cc_stream, cc_length );
			cc_stream[ cc_length ] = 0;
			
			wchar_t converted[ 256 ];
			// FIXME: Do unicode parsing, raw data parsing
			if ( unicode )
			{
				memcpy( converted, cc_stream, cc_length );
				cc_length /= 2;
				converted[ cc_length ] = L'\0';
			}
			else
			{
#ifndef SWDS // FIXME!!!!!!
				ConvertANSIToUnicode( cc_stream, converted, sizeof( converted ) );
#endif
			}
			
			// Skip space
			buf.GetChar();
			buf.GetString( token );
			start = atof( token );
			buf.GetString( token );
			end = atof( token );

			CCloseCaptionPhrase *phrase = new CCloseCaptionPhrase( converted );
			phrase->SetStartTime( start );
			phrase->SetEndTime( end );
			//phrase->SetType( cc_type );

			AddCloseCaptionPhrase( language_id, phrase );
			buf.GetString( token );
		}

	}
}

void CSentence::ParseOptions( CUtlBuffer& buf )
{
	char token[ 4096 ];
	while ( 1 )
	{
		buf.GetString( token );
		if ( !stricmp( token, "}" ) )
			break;

		char key[ 256 ];
		strcpy( key, token );
		char value[ 256 ];
		buf.GetString( token );
		strcpy( value, token );

		if ( !strcmpi( key, "voice_duck" ) )
		{
			SetVoiceDuck( atoi(value) ? true : false );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: VERSION 1.0 parser, need to implement new ones if 
//  file format changes!!!
// Input  : buf - 
//-----------------------------------------------------------------------------
void CSentence::ParseDataVersionOnePointZero( CUtlBuffer& buf )
{
	char token[ 4096 ];

	while ( 1 )
	{
		buf.GetString( token );
		if ( strlen( token ) <= 0 )
			break;

		char section[ 256 ];
		strcpy( section, token );

		buf.GetString( token );
		if ( stricmp( token, "{" ) )
			break;

		if ( !stricmp( section, "PLAINTEXT" ) )
		{
			ParsePlaintext( buf );
		}
		else if ( !stricmp( section, "WORDS" ) )
		{
			ParseWords( buf );
		}
		else if ( !stricmp( section, "EMPHASIS" ) )
		{
			ParseEmphasis( buf );
		}		
		else if ( !stricmp( section, "CLOSECAPTION" ) )
		{
			ParseCloseCaption( buf );
		}
		else if ( !stricmp( section, "OPTIONS" ) )
		{
			ParseOptions( buf );
		}
	}
}

void CSentence::SaveToBuffer( CUtlBuffer& buf )
{
	int i, j;

	buf.Printf( "VERSION 1.0\n" );
	buf.Printf( "PLAINTEXT\n" );
	buf.Printf( "{\n" );
	buf.Printf( "%s\n", GetText() );
	buf.Printf( "}\n" );
	buf.Printf( "WORDS\n" );
	buf.Printf( "{\n" );
	for ( i = 0; i < m_Words.Size(); i++ )
	{
		CWordTag *word = m_Words[ i ];
		Assert( word );

		buf.Printf( "WORD %s %.3f %.3f\n", 
			word->m_pszWord[ 0 ] ? word->m_pszWord : "???",
			word->m_flStartTime,
			word->m_flEndTime );

		buf.Printf( "{\n" );
		for ( j = 0; j < word->m_Phonemes.Size(); j++ )
		{
			CPhonemeTag *phoneme = word->m_Phonemes[ j ];
			Assert( phoneme );

			buf.Printf( "%i %s %.3f %.3f %.3f\n", 
				phoneme->m_nPhonemeCode, 
				phoneme->m_szPhoneme[ 0 ] ? phoneme->m_szPhoneme : "???",
				phoneme->m_flStartTime,
				phoneme->m_flEndTime,
				phoneme->m_flVolume );
		}

		buf.Printf( "}\n" );
	}
	buf.Printf( "}\n" );
	buf.Printf( "EMPHASIS\n" );
	buf.Printf( "{\n" );
	int c = m_EmphasisSamples.Count();
	for ( i = 0; i < c; i++ )
	{
		CEmphasisSample *sample = &m_EmphasisSamples[ i ];
		Assert( sample );

		buf.Printf( "%f %f\n", sample->time, sample->value );
	}

	buf.Printf( "}\n" );
	buf.Printf( "CLOSECAPTION\n" );
	buf.Printf( "{\n" );

	for ( int l = 0; l < CC_NUM_LANGUAGES; l++ )
	{
		int count = GetCloseCaptionPhraseCount( l );
		if ( count == 0 && l == CC_ENGLISH )
		{
			SetCloseCaptionFromText( l );
			count = GetCloseCaptionPhraseCount( l );
		}

		if ( count <= 0 )
			continue;

		buf.Printf( "%s\n", CCloseCaptionPhrase::NameForLanguage( l ) );
		buf.Printf( "{\n" );

		for ( int j = 0 ; j < count; j++ )
		{
			CCloseCaptionPhrase *phrase = GetCloseCaptionPhrase( l, j );
			Assert( phrase );

			int len = wcslen( phrase->GetStream() );
			len <<= 1; // double because it's a wchar_t

			buf.Printf( "PHRASE unicode %i ", len );
			buf.Put( phrase->GetStream(), len );
			buf.Printf( " %.3f %.3f\n",
				phrase->GetStartTime(),
				phrase->GetEndTime() );
		}

		buf.Printf( "}\n" );
	}
	buf.Printf( "}\n" );

	buf.Printf( "OPTIONS\n" );
	buf.Printf( "{\n" );
	buf.Printf( "voice_duck %d\n", GetVoiceDuck() ? 1 : 0 );
	buf.Printf( "}\n" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *data - 
//			size - 
//-----------------------------------------------------------------------------
void CSentence::InitFromDataChunk( void *data, int size )
{
	Reset();

	CUtlBuffer buf( 0, 0, true );
	buf.EnsureCapacity( size );
	buf.Put( data, size );
	buf.SeekPut( CUtlBuffer::SEEK_HEAD, size );

	char token[ 4096 ];
	buf.GetString( token );

	if ( stricmp( token, "VERSION" ) )
		return;

	buf.GetString( token );
	if ( atof( token ) == 1.0f )
	{
		ParseDataVersionOnePointZero( buf );
	}
	else
	{
		assert( 0 );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CSentence::GetWordBase( void )
{
	return m_nResetWordBase;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::ResetToBase( void )
{
	// Delete everything after m_nResetWordBase
	while ( m_Words.Size() > m_nResetWordBase )
	{
		delete m_Words[ m_Words.Size() - 1 ];
		m_Words.Remove( m_Words.Size() - 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::MarkNewPhraseBase( void )
{
	m_nResetWordBase = max( m_Words.Size(), 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::Reset( void )
{
	m_nResetWordBase = 0;

	while ( m_Words.Size() > 0 )
	{
		delete m_Words[ 0 ];
		m_Words.Remove( 0 );
	}

	m_EmphasisSamples.RemoveAll();

	ResetCloseCaptionAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::ResetCloseCaptionAll( void )
{
	for ( int l = 0; l < CC_NUM_LANGUAGES; l++ )
	{
		ResetCloseCaption( l );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::ResetCloseCaption( int language )
{
	while ( m_CloseCaption[ language ].Size() > 0 )
	{
		delete m_CloseCaption[ language ][ 0 ];
		m_CloseCaption[ language ].Remove( 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tag - 
//-----------------------------------------------------------------------------
void CSentence::AddPhonemeTag( CWordTag *word, CPhonemeTag *tag )
{
	word->m_Phonemes.AddToTail( tag );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tag - 
//-----------------------------------------------------------------------------
void CSentence::AddWordTag( CWordTag *tag )
{
	m_Words.AddToTail( tag );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CSentence::CountPhonemes( void )
{
	int c = 0;
	for( int i = 0; i < m_Words.Size(); i++ )
	{
		CWordTag *word = m_Words[ i ];
		c += word->m_Phonemes.Size();
	}
	return c;
}

//-----------------------------------------------------------------------------
// Purpose: // For legacy loading, try to find a word that contains the time
// Input  : time - 
// Output : CWordTag
//-----------------------------------------------------------------------------
CWordTag *CSentence::EstimateBestWord( float time )
{
	CWordTag *bestWord = NULL;

	for( int i = 0; i < m_Words.Size(); i++ )
	{
		CWordTag *word = m_Words[ i ];
		if ( !word )
			continue;

		if ( word->m_flStartTime <= time && word->m_flEndTime >= time )
			return word;

		if ( time < word->m_flStartTime )
		{
			bestWord = word;
		}

		if ( time > word->m_flEndTime && bestWord )
			return bestWord;
	}

	// return best word if we found one
	if ( bestWord )
	{
		return bestWord;
	}

	// Return last word
	if ( m_Words.Size() >= 1 )
	{
		return m_Words[ m_Words.Size() - 1 ];
	}

	// Oh well
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *phoneme - 
// Output : CWordTag
//-----------------------------------------------------------------------------
CWordTag *CSentence::GetWordForPhoneme( CPhonemeTag *phoneme )
{
	for( int i = 0; i < m_Words.Size(); i++ )
	{
		CWordTag *word = m_Words[ i ];
		if ( !word )
			continue;

		for ( int j = 0 ; j < word->m_Phonemes.Size() ; j++ )
		{
			CPhonemeTag *p = word->m_Phonemes[ j ];
			if ( p == phoneme )
			{
				return word;
			}
		}

	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Assignment operator
// Input  : src - 
// Output : CSentence&
//-----------------------------------------------------------------------------
CSentence& CSentence::operator=( const CSentence& src )
{
	// Clear current stuff
	Reset();

	// Copy everything
	for ( int i = 0 ; i < src.m_Words.Size(); i++ )
	{
		CWordTag *word = src.m_Words[ i ];

		CWordTag *newWord = new CWordTag( *word );

		AddWordTag( newWord );
	}

	SetText( src.m_szText );
	m_nResetWordBase = src.m_nResetWordBase;

	int c = src.m_EmphasisSamples.Size();
	for ( i = 0; i < c; i++ )
	{
		CEmphasisSample s = src.m_EmphasisSamples[ i ];
		m_EmphasisSamples.AddToTail( s );
	}

	for ( int l = 0; l < CC_NUM_LANGUAGES; l++ )
	{
		c = src.m_CloseCaption[ l ].Count();
		for ( i = 0; i < c; i++ )
		{
			CCloseCaptionPhrase *phrase = new CCloseCaptionPhrase( (*src.m_CloseCaption[ l ][ i ]) );
			m_CloseCaption[ l ].AddToTail( phrase );
		}
	}

	m_bShouldVoiceDuck = src.m_bShouldVoiceDuck;
	return (*this);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *text - 
//-----------------------------------------------------------------------------
void CSentence::SetText( const char *text )
{
	strcpy( m_szText, text );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CSentence::GetText( void )
{
	return m_szText;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::SetTextFromWords( void )
{
	m_szText[ 0 ] = 0;
	for ( int i = 0 ; i < m_Words.Size(); i++ )
	{
		CWordTag *word = m_Words[ i ];

		strcat( m_szText, word->m_pszWord );

		if ( i != m_Words.Size() )
		{
			strcat( m_szText, " " );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::Resort( void )
{
	int c = m_EmphasisSamples.Size();
	for ( int i = 0; i < c; i++ )
	{
		for ( int j = i + 1; j < c; j++ )
		{
			CEmphasisSample src = m_EmphasisSamples[ i ];
			CEmphasisSample dest = m_EmphasisSamples[ j ];

			if ( src.time > dest.time )
			{
				m_EmphasisSamples[ i ] = dest;
				m_EmphasisSamples[ j ] = src;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : number - 
// Output : CEmphasisSample
//-----------------------------------------------------------------------------
CEmphasisSample *CSentence::GetBoundedSample( int number, float endtime )
{
	// Search for two samples which span time f
	static CEmphasisSample nullstart;
	nullstart.time = 0.0f;
	nullstart.value = 0.5f;
	static CEmphasisSample nullend;
	nullend.time = endtime;
	nullend.value = 0.5f;
	
	if ( number < 0 )
	{
		return &nullstart;
	}
	else if ( number >= GetNumSamples() )
	{
		return &nullend;
	}
	
	return GetSample( number );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//			type - 
// Output : float
//-----------------------------------------------------------------------------
float CSentence::GetIntensity( float time, float endtime )
{
	float zeroValue = 0.5f;
	
	int c = GetNumSamples();
	
	if ( c <= 0 )
	{
		return zeroValue;
	}
	
	int i;
	for ( i = -1 ; i < c; i++ )
	{
		CEmphasisSample *s = GetBoundedSample( i, endtime );
		CEmphasisSample *n = GetBoundedSample( i + 1, endtime );
		if ( !s || !n )
			continue;

		if ( time >= s->time && time <= n->time )
		{
			break;
		}
	}

	int prev = i - 1;
	int start = i;
	int end = i + 1;
	int next = i + 2;

	prev = max( -1, prev );
	start = max( -1, start );
	end = min( end, GetNumSamples() );
	next = min( next, GetNumSamples() );

	CEmphasisSample *esPre = GetBoundedSample( prev, endtime );
	CEmphasisSample *esStart = GetBoundedSample( start, endtime );
	CEmphasisSample *esEnd = GetBoundedSample( end, endtime );
	CEmphasisSample *esNext = GetBoundedSample( next, endtime );

	float dt = esEnd->time - esStart->time;
	dt = clamp( dt, 0.01f, 1.0f );

	Vector vPre( esPre->time, esPre->value, 0 );
	Vector vStart( esStart->time, esStart->value, 0 );
	Vector vEnd( esEnd->time, esEnd->value, 0 );
	Vector vNext( esNext->time, esNext->value, 0 );

	float f2 = ( time - esStart->time ) / ( dt );
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	Catmull_Rom_Spline( 
		vPre,
		vStart,
		vEnd,
		vNext,
		f2, 
		vOut );

	float retval = clamp( vOut.y, 0.0f, 1.0f );
	return retval;
}

int CSentence::GetNumSamples( void )
{
	return m_EmphasisSamples.Count();
}

CEmphasisSample *CSentence::GetSample( int index )
{
	if ( index < 0 || index >= GetNumSamples() )
		return NULL;

	return &m_EmphasisSamples[ index ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *phrase - 
//-----------------------------------------------------------------------------
void CSentence::AddCloseCaptionPhrase( int language, CCloseCaptionPhrase *phrase )
{
	m_CloseCaption[ language ].AddToTail( phrase );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : language - 
//			*phrase - 
//			index - 
//-----------------------------------------------------------------------------
void CSentence::InsertCloseCaptionPhraseAtIndex( int language, CCloseCaptionPhrase *phrase, int index )
{
	m_CloseCaption[ language ].InsertBefore( index, phrase );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSentence::SetCloseCaptionFromText( int language )
{
#ifndef SWDS // FIXME!!!!!!
	ResetCloseCaption( language );
	wchar_t converted[ 1024 ];
	ConvertANSIToUnicode( m_szText, converted, sizeof( converted ) );

	CCloseCaptionPhrase *phrase = new CCloseCaptionPhrase( converted );
	float s, e;
	GetEstimatedTimes( s, e );
	phrase->SetStartTime( s );
	phrase->SetEndTime( e );
	AddCloseCaptionPhrase( language, phrase );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CSentence::GetCloseCaptionPhraseCount( int language ) const
{
	return m_CloseCaption[ language ].Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : CCloseCaptionPhrase
//-----------------------------------------------------------------------------
CCloseCaptionPhrase *CSentence::GetCloseCaptionPhrase( int language, int index )
{
	if ( index < 0 || index >= m_CloseCaption[ language ].Count() )
		return NULL;
	return m_CloseCaption[ language ][ index ];
}

void CSentence::GetEstimatedTimes( float& start, float &end )
{
	float beststart = 100000.0f;
	float bestend = -100000.0f;

	int c = m_Words.Count();
	if ( !c )
	{
		start = end = 0.0f;
		return;
	}

	for ( int i = 0; i< c; i++ )
	{
		CWordTag *w = m_Words[ i ];
		Assert( w );
		if ( w->m_flStartTime < beststart )
		{
			beststart = w->m_flStartTime;
		}
		if ( w->m_flEndTime > bestend )
		{
			bestend = w->m_flEndTime;
		}
	}

	if ( beststart == 100000.0f )
	{
		Assert( 0 );
		beststart = 0.0f;
	}
	if ( bestend == -100000.0f )
	{
		Assert( 0 );
		bestend = 1.0f;
	}
	start = beststart;
	end = bestend;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : language - 
//			index - 
//-----------------------------------------------------------------------------
void CSentence::RemoveCloseCaptionPhrase( int language, int index )
{
	Assert( language >= 0 && language < CC_NUM_LANGUAGES );

	if ( index < 0 || index >= m_CloseCaption[ language ].Count() )
		return;

	CCloseCaptionPhrase *cur = m_CloseCaption[ language ][ index ];
	m_CloseCaption[ language ].Remove( index );
	delete cur;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : language - 
//			*phrase - 
// Output : int
//-----------------------------------------------------------------------------
int CSentence::FindCloseCaptionPhraseIndex( int language, CCloseCaptionPhrase *phrase )
{
	int idx = m_CloseCaption[ language ].Find( phrase );
	if ( idx == m_CloseCaption[ language ].InvalidIndex() )
		return -1;
	return idx;
}
