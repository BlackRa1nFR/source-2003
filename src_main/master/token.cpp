// Token.cpp: implementation of the CToken class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Token.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CToken::CToken(char *pData)
{
	m_pData = pData;
	m_bSemiColonMode = FALSE;
	m_bQuoteMode = FALSE;
	m_bSkipComments = FALSE;
}

CToken::~CToken()
{
	m_bSemiColonMode = FALSE;
	m_pData = NULL;
}

/*
==============
ParseNextToken

Parse the next token out of data stream
The token is placed in the 'token' data element...
==============
*/
void CToken::ParseNextToken()
{
	int             c;
	int             len;
	BOOL            inColon;

	if ( !m_pData )
		return;

	// Skip comment lines.
	if (m_bSkipComments)
	{
		while (1)
		{
			// Skip white space at start of line.
			while (m_pData &&
				(*m_pData == '\r' ||
				 *m_pData == '\n' ||
				 *m_pData == ' '  ||
				 *m_pData == '\t'))
				 m_pData++;

			if (*m_pData == '/' && *(m_pData+1) == '/')
			{
				// Skip to end of line
				GetRemainder();
			}
			else
				break;
		}
	}
	// Special handling
	if (m_bSemiColonMode)
	{
		ParseNextSemiColonToken();
		return;
	}

	if (m_bQuoteMode)
	{
		ParseNextQuoteToken();
		return;
	}

	inColon = FALSE;
	len = 0;

	// The return token
	token[0] = 0;
	
	if (!m_pData)
		return;
		
	// skip whitespace.  Only space characters are white space
	while ( (c = *m_pData) == ' ')
	{
		if (c == 0)
			return; // end of stream;
		m_pData++;
	}

	// Skip cr/lf
	if ( (c = *m_pData) == '\r')
		m_pData++;

	if ( (c = *m_pData) == '\n')
		m_pData++;


	if (*m_pData == ':') // If we parse a leading colon, entire remainder of string is next
		inColon = TRUE;	 // token.

	// parse a regular word
	do
	{
		// Don't copy cr/lf pairs, just skip them.
		if (c != '\r' && c != '\n')
		{
			token[len] = c;
			len++;
		}
		m_pData++;
		c = *m_pData;

		// If we are reading from colon to end of string,
		//  only break at end of string
		if (inColon && !c)
			break;

		// If we get to end of line, stop parsing token
		if (c == '\n')
			break;

	} while ( ((c != 0) && (c != 32)) || inColon);
	
	token[len] = 0;
}

void CToken::SetData(char * pData)
{
	m_pData = pData;
}

void CToken::GetRemainder()
{

	int             c;
	int             len;

	token[0] = 0;
	len = 0;

	if (!m_pData)
		return;

	// skip whitespace.  Only space characters are white space
	while ( (c = *m_pData) == ' ')
	{
		if (c == 0)
			return; // end of stream;
		m_pData++;
	}

	// If only thing left is cr/lf, nothing is left
	c = *m_pData;
	if (( c == '\r' ) ||
		( c == '\n' ))
		return; 

	// parse rest of line
	do
	{
		// Don't copy cr/lf pair
		if (c != '\r' && c != '\n')
		{
			token[len] = c;
			len++;
		}
		m_pData++;
		c = *m_pData;

		// If we are reading from colon to end of string,
		//  only break at end of string
		if (!c)
			break;

		// If we get to end of line, stop parsing token
		if (c == '\n')
			break;

	} while (c != 0);
	
	token[len] = 0;
}

void CToken::SetSemicolonMode(BOOL bMode)
{
	m_bSemiColonMode = bMode;
}

void CToken::ParseNextSemiColonToken()
{
	int             c;
	int             len;
	BOOL            inColon;

	inColon = FALSE;
	len = 0;

	// The return token
	token[0] = 0;
	
	if (!m_pData)
		return;
		
	// skip whitespace.  Only ';' are white space
	while ( (c = *m_pData) == ';')
	{
		if (c == 0)
			return; // end of stream;
		m_pData++;
	}

	// parse a regular word
	do
	{
		token[len++] = c;
		m_pData++;
		c = *m_pData;
	} while ( c && (c != ';'));
	
	token[len] = 0;
}

// Treat everything inside double quotes as one token, omitting the 
//  quotes?
void CToken::SetQuoteMode(BOOL bMode)
{
	m_bQuoteMode = bMode;
}

void CToken::ParseNextQuoteToken()
{
	int             c;
	int             len;

	len = 0;

	// The return token
	token[0] = 0;
	
	if (!m_pData)
		return;
		
	// skip whitespace, including tabs.  Only space characters are white space
	while (1)
	{
		c = *m_pData;
		
		if (c != ' ' && c != '\t')
			break;

		if (c == 0)
			return; // end of stream;

		m_pData++;
	}

	// Skip cr/lf
	while (1)
	{
		c = *m_pData;

		if (c == '\r' ||
			c == '\n' ||
			c == '\t')
		{
			m_pData++;
			continue;
		};

		break;
	}

	// parse a regular word
	do
	{
		if (c == '"')   // If parse a double quote, then read until we get the closing double quote.
		{
			m_pData++;
			c = *m_pData;

			// !!!HACK to allow """ in parsing the key file
			if (c == '"' && *(m_pData+1) == '"')
			{
				token[len++] = c;
				m_pData++;
				c = *m_pData;
			}

			while (c != '"')
			{
				if (!c)  // Parsing error
				{
					token[len] = 0;
					return;
				}

				token[len] = c;
				len++;
				m_pData++;
				c = *m_pData;
			}

			m_pData++;

			token[len] = 0;

			// Skip cr/lf or tabs at end of line.
			while (1)
			{
				if ( (c = *m_pData) == '\r')
				{
					m_pData++;
					continue;
				}

				if ( (c = *m_pData) == '\n')
				{
					m_pData++;
					continue;
				}

				if ( (c = *m_pData) == '\t')
				{
					m_pData++;
					continue;
				}

				break;
			}
			return;
		}

		// Don't copy cr/lf pairs, just skip them.
		if (c != '\r' && c != '\n' && c!= '\t')
		{
			token[len] = c;
			len++;
		}
		m_pData++;
		c = *m_pData;

		// If we are reading from colon to end of string,
		//  only break at end of string
		if (!c)
			break;

		// If we get to end of line, stop parsing token
		if (c == '\n' || c == '\t')
			break;

	} while ( ((c != 0) && (c != 32)));
	
	token[len] = 0;
}

void CToken::SetCommentMode(BOOL bMode)
{
	m_bSkipComments = bMode;
}
