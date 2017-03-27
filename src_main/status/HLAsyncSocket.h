#ifndef OEM_BUILD

#if !defined(AFX_HLASYNCSOCKET_H__54CF5B92_B52F_11D1_874F_006097A548BE__INCLUDED_)
#define AFX_HLASYNCSOCKET_H__54CF5B92_B52F_11D1_874F_006097A548BE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// HLAsyncSocket.h : header file
//
// Message Buffer Size.

#define QUERY_BUFSIZE 8192
// This class stores the asynchronous communication socket used to query various
//  Half-Life game servers for server information, player information, rule information, and
//  ping time.

class CMessageBuffer;

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket command target

class CHLAsyncSocket : public CAsyncSocket
{
// Attributes
public:
	CHLAsyncSocket( float *pPingIndex );
	virtual ~CHLAsyncSocket();

	void HL_ReceivePing();

// Operations
	CMessageBuffer *pMessageBuffer;
	float fSendTime, fRecTime;

// Overrides
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHLAsyncSocket)
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CHLAsyncSocket)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
protected:
	float *m_pPingIndex;  // Where to stow the ping
	
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HLASYNCSOCKET_H__54CF5B92_B52F_11D1_874F_006097A548BE__INCLUDED_)

#endif