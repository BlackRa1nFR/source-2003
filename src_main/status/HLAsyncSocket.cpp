// HLAsyncSocket.cpp : implementation file
//

#include "stdafx.h"
#include "MessageBuffer.h"
#include "HLAsyncSocket.h"
#include "SVTest.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define A2A_ACK 'j'	

extern double Sys_DoubleTime(void);

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket

//===================================================================
// CHLAsyncSocket()
//
// CHLAsyncSocket constructor.  Keep a local pointer to the parent
//  server where data should be stuffed.
//===================================================================
CHLAsyncSocket::CHLAsyncSocket( float *pPingIndex )
{
	m_pPingIndex = pPingIndex;

	fSendTime = fRecTime = 0.0;

	pMessageBuffer = new CMessageBuffer(QUERY_BUFSIZE);
	ASSERT(pMessageBuffer);
}

CHLAsyncSocket::~CHLAsyncSocket()
{
	ASSERT(pMessageBuffer);
	pMessageBuffer->SZ_Free();
	delete pMessageBuffer;
}

// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(CHLAsyncSocket, CAsyncSocket)
	//{{AFX_MSG_MAP(CHLAsyncSocket)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif	// 0

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket member functions

//===================================================================
// OnReceive()
//
// OnReceive receives the requested data and parses it as necessary
//  filling in the server structure with the parsed data.
//===================================================================
void CHLAsyncSocket::OnReceive(int nErrorCode) 
{
	if (nErrorCode == WSAENETDOWN)
	{
		CAsyncSocket::OnReceive(nErrorCode);	
		return;
	};

	HL_ReceivePing();
	
	CAsyncSocket::OnReceive(nErrorCode);
}

//===================================================================
// OnSend()
//
//===================================================================
void CHLAsyncSocket::OnSend(int nErrorCode) 
{
	if (nErrorCode == WSAENETDOWN)
	{
		CAsyncSocket::OnSend(nErrorCode);	
		return;
	};

	fSendTime = Sys_DoubleTime();

	CAsyncSocket::OnSend(nErrorCode);
}
//===================================================================
// HL_ReceivePing()
//
// HL_ReceivePing computes server ping response time.
//===================================================================
void CHLAsyncSocket::HL_ReceivePing()
{
	// Attempt to receive data
	int iBytesRead;
	iBytesRead = Receive(pMessageBuffer->GetData(), pMessageBuffer->GetMaxSize());

	// Didn't receive anything, nothing to process
	if (!iBytesRead)
		return;

	// Set buffer size appropriately
	pMessageBuffer->SetCurSize(iBytesRead);

	// Parse message buffer
	int control;
	pMessageBuffer->MSG_BeginReading();
	control = pMessageBuffer->MSG_ReadLong();
	if (control != -1)
		return;

	// Check message type and ignore if not appropriate
	if (pMessageBuffer->MSG_ReadByte() != A2A_ACK)
		return;

	fRecTime = Sys_DoubleTime();

	*m_pPingIndex = 1000.0 * (fRecTime - fSendTime);
}

