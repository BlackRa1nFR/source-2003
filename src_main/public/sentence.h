//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Phoneme/Viseme sentence
//
// $NoKeywords: $
//=============================================================================
#ifndef SENTENCE_H
#define SENTENCE_H
#ifdef _WIN32
#pragma once
#endif


#include "utlvector.h"

class CUtlBuffer;

struct CEmphasisSample
{
	float		time;
	float		value;

	bool		selected;
};

class CPhonemeTag
{
public:
	enum
	{
		MAX_PHONEME_LENGTH = 8
	};

					CPhonemeTag( void );
					CPhonemeTag( const char *phoneme );
					CPhonemeTag( const CPhonemeTag& from );
					~CPhonemeTag( void );

	void			SetTag( const char *phoneme );

	float			m_flStartTime;
	float			m_flEndTime;
	float			m_flVolume;
	int				m_nPhonemeCode;
	char			m_szPhoneme[ MAX_PHONEME_LENGTH ];

	// Used by UI code
	unsigned int	m_uiStartByte;
	unsigned int	m_uiEndByte;
	bool			m_bSelected;
};

class CWordTag
{
public:
					CWordTag( void );
					CWordTag( const char *word );
					CWordTag( const CWordTag& from );
					~CWordTag( void );

	void			SetWord( const char *word );

	int				IndexOfPhoneme( CPhonemeTag *tag );

	float			m_flStartTime;
	float			m_flEndTime;

	char			*m_pszWord;

	CUtlVector	< CPhonemeTag *> m_Phonemes;

	bool			m_bSelected;
	unsigned int	m_uiStartByte;
	unsigned int	m_uiEndByte;
};

// A sentence can be closed captioned
// The default case is the entire sentence shown at start time
// 
// "<persist:2.0><clr:255,0,0,0>The <I>default<I> case"
// "<sameline>is the <U>entire<U> sentence shown at <B>start time<B>"

// Commands that aren't closed at end of phrase are automatically terminated
//
// Commands
// <linger:2.0>	The line should persist for 2.0 seconds beyond m_flEndTime
// <sameline>		Don't go to new line for next phrase on stack
// <clr:r,g,b,a>	Push current color onto stack and start drawing with new
//  color until we reach the next <clr> marker or a <clr> with no commands which
//  means restore the previous color
// <U>				Underline text (start/end)
// <I>				Italics text (start/end)
// <B>				Bold text (start/end)
// <position:where>	Draw caption at special location ??? needed
// <cr>				Go to new line

// Close Captioning Support
// The phonemes drive the mouth in english, but the CC text can
//  be one of several languages
enum
{
	CC_ENGLISH = 0,
	CC_FRENCH,
	CC_GERMAN,
	CC_SPANISH,
	CC_ITALIAN,
	CC_KOREAN,
	CC_JAPANESE,
	// etc etc

	CC_NUM_LANGUAGES
};

class CCloseCaptionPhrase
{
public:
	static char const	*NameForLanguage( int language );
	static int			LanguageForName( char const *name );

					CCloseCaptionPhrase( void );
					CCloseCaptionPhrase( const wchar_t *stream );
					CCloseCaptionPhrase( const CCloseCaptionPhrase& from );
					~CCloseCaptionPhrase( void );

	void			SetStream( const wchar_t *stream );
	const wchar_t	*GetStream( void ) const;

	void			SetSelected( bool selected );
	bool			GetSelected( void ) const;

	void			SetStartTime( float st );
	float			GetStartTime( void ) const;
	void			SetEndTime( float et );
	float			GetEndTime( void ) const;

	int				CountTokens( void ) const;
	const wchar_t	*GetToken( int token_number ) const;
	bool			SplitCommand( const wchar_t *in, wchar_t *cmd, wchar_t *args ) const;

private:
	wchar_t const	*ParseToken( const wchar_t *in, wchar_t *token, int tokensize ) const;

	void			Init( void );

	float			m_flStartTime;
	float			m_flEndTime;
	wchar_t			*m_pszStream;
	bool			m_bSelected;
};

//-----------------------------------------------------------------------------
// Purpose: A sentence is a box of words, and words contain phonemes
//-----------------------------------------------------------------------------
class CSentence
{
public:
	// Construction
					CSentence( void );
					~CSentence( void );

	// Assignment operator
	CSentence& operator =(const CSentence& src );

	void			SetText( const char *text );
	const char		*GetText( void );

	void			InitFromDataChunk( void *data, int size );
	void			SaveToBuffer( CUtlBuffer& buf );

	// Add word/phoneme to sentence
	void			AddPhonemeTag( CWordTag *word, CPhonemeTag *tag );
	void			AddWordTag( CWordTag *tag );

	void			Reset( void );

	void			ResetToBase( void );

	void			MarkNewPhraseBase( void );

	int				GetWordBase( void );

	int				CountPhonemes( void );

	// For legacy loading, try to find a word that contains the time
	CWordTag		*EstimateBestWord( float time );

	CWordTag		*GetWordForPhoneme( CPhonemeTag *phoneme );

	void			SetTextFromWords( void );

	float			GetIntensity( float time, float endtime );
	void			Resort( void );
	CEmphasisSample *GetBoundedSample( int number, float endtime );
	int				GetNumSamples( void );
	CEmphasisSample	*GetSample( int index );

	void			AddCloseCaptionPhrase( int language, CCloseCaptionPhrase *phrase );
	void			InsertCloseCaptionPhraseAtIndex( int language, CCloseCaptionPhrase *phrase, int index );

	int				GetCloseCaptionPhraseCount( int language ) const;
	CCloseCaptionPhrase *GetCloseCaptionPhrase( int language, int index );
	void			RemoveCloseCaptionPhrase( int language, int index );
	int				FindCloseCaptionPhraseIndex( int language, CCloseCaptionPhrase *phrase );

	void			SetCloseCaptionFromText( int language );
	// Compute start and endtime based on all words
	void			GetEstimatedTimes( float& start, float &end );

	void			SetVoiceDuck( bool shouldDuck ) { m_bShouldVoiceDuck = shouldDuck; }
	bool			GetVoiceDuck() const { return m_bShouldVoiceDuck; }

public:
	enum
	{
		MAX_SENTENCE_SIZE = 512,
	};

	char			m_szText[ MAX_SENTENCE_SIZE ];

	CUtlVector< CWordTag * >	m_Words;
	CUtlVector< CCloseCaptionPhrase * >	m_CloseCaption[ CC_NUM_LANGUAGES ];


	int				m_nResetWordBase;
	
	// Phoneme emphasis data
	CUtlVector< CEmphasisSample > m_EmphasisSamples;

private:
	void			ParseDataVersionOnePointZero( CUtlBuffer& buf );
	void			ParsePlaintext( CUtlBuffer& buf );
	void			ParseWords( CUtlBuffer& buf );
	void			ParseEmphasis( CUtlBuffer& buf );
	void			ParseCloseCaption( CUtlBuffer& buf );
	void			ParseOptions( CUtlBuffer& buf );

	void			ResetCloseCaptionAll( void );
	void			ResetCloseCaption( int language );

	friend class PhonemeEditor;
	bool			m_bShouldVoiceDuck;
};
#endif // SENTENCE_H
