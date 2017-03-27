// Token.h: interface for the CToken class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TOKEN_H__95FD0512_C754_11D1_8756_006097A548BE__INCLUDED_)
#define AFX_TOKEN_H__95FD0512_C754_11D1_8756_006097A548BE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CToken  
{
public:
	void SetCommentMode(BOOL bMode);
	void ParseNextQuoteToken();
	void SetQuoteMode(BOOL bMode);
	void SetSemicolonMode(BOOL bMode);
	void GetRemainder();
	void SetData(char *pData);
	void ParseNextToken();
	CToken(char *);
	virtual ~CToken();

	char token[1024];

protected:
	BOOL m_bSkipComments;
	BOOL m_bQuoteMode;
	char * m_pData;
	BOOL m_bSemiColonMode;
	void ParseNextSemiColonToken();
};

#endif // !defined(AFX_TOKEN_H__95FD0512_C754_11D1_8756_006097A548BE__INCLUDED_)
