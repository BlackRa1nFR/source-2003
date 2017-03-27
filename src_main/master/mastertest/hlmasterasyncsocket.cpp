// HLMasterAsyncSocket.cpp : implementation file
//

#include "stdafx.h"
#include "MasterTest.h"
#include "MasterTestDlg.h"
#include "MessageBuffer.h"
#include "HLMasterAsyncSocket.h"
#include "..\HLMasterProtocol.h"
#include "proto_oob.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern BOOL bigendien;

short   (*BigShort) (short l);
short   (*LittleShort) (short l);
int32   (*BigLong) (int32 l);
int32     (*LittleLong) (int32 l);
float   (*BigFloat) (float l);
float   (*LittleFloat) (float l);

extern short   ShortSwap (short l);
extern short   ShortNoSwap (short l);
extern int32   LongSwap (int32 l);
extern int32     LongNoSwap (int32 l);
extern float   FloatSwap (float f);
extern float   FloatNoSwap (float f);

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket

//===================================================================
// CHLMasterAsyncSocket()
//
// CHLMasterAsyncSocket:  Constructur, keep a pointer to parent document
//===================================================================
CHLMasterAsyncSocket::CHLMasterAsyncSocket(CMasterTestDlg *pDlg)
{
	pDialog = pDlg;
	bInitialized = FALSE;
	unsigned char swaptest[2] = {1,0};

	// set the byte swapping variables in a portable manner 
	if ( *(short *)swaptest == 1)
	{
		bigendien = FALSE;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien = TRUE;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	};

	pMessageBuffer = new CMessageBuffer(MASTER_BUFSIZE);
	ASSERT(pMessageBuffer);
	pSendBuffer = new CMessageBuffer(MASTER_BUFSIZE);
	ASSERT(pMessageBuffer);
	pSendBuffer->SZ_Clear();
}

CHLMasterAsyncSocket::~CHLMasterAsyncSocket()
{
	ASSERT(pMessageBuffer);
	pMessageBuffer->SZ_Clear();
	delete pMessageBuffer;
	pSendBuffer->SZ_Clear();
	delete pSendBuffer;
}

// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(CHLMasterAsyncSocket, CAsyncSocket)
	//{{AFX_MSG_MAP(CHLMasterAsyncSocket)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif	// 0

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket member functions

//===================================================================
// OnReceive()
//
// OnReceive:  Receive message from master server.
//===================================================================
void CHLMasterAsyncSocket::OnReceive(int nErrorCode) 
{
	if (nErrorCode != 0)
	{
		CAsyncSocket::OnReceive(nErrorCode);
		return;
	};

	int iBytesRead;
	iBytesRead = Receive(pMessageBuffer->GetData(), pMessageBuffer->GetMaxSize());
	if (!iBytesRead)
		return;

	pDialog->DataReceived(iBytesRead);

	pMessageBuffer->SetCurSize(iBytesRead);

	// Check message type
	int control;
	pMessageBuffer->MSG_BeginReading();
	control = BigLong(*((int *)pMessageBuffer->GetData()));
	pMessageBuffer->MSG_ReadLong();

	// Control should be 0xFFFFFFFF (i.e., -1)
	if (control != -1)
		return;

	// Check type byte and ignore if not valid
	unsigned char c;
	c = pMessageBuffer->MSG_ReadByte();

	if ( c == M2A_CHALLENGE )
	{
		// skip the \n
		pMessageBuffer->MSG_ReadByte();

		unsigned int chall = pMessageBuffer->MSG_ReadLong();

		pDialog->SendHeartbeat( chall, this );
		return;
	}

	if (c != M2A_SERVER_BATCH )
		return;

	// Read batch #?
	
	CAsyncSocket::OnReceive(nErrorCode);
}

//===================================================================
// OnSend()
//
// OnSend:  Receive message from master server.
//===================================================================
void CHLMasterAsyncSocket::OnSend(int nErrorCode) 
{
	if (nErrorCode != 0)
	{
		CAsyncSocket::OnSend(nErrorCode);
		return;
	};

	int iBytesSent;
	iBytesSent = Send(pSendBuffer->GetData(), pSendBuffer->GetCurSize());
	if (!iBytesSent)
		return;

	pSendBuffer->SZ_Clear();

	pDialog->DataSent(iBytesSent);
}



