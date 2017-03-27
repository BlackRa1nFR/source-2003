//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "PacketHeader.h"
#include "ReceiveMessage.h"
#include "Random.h"
#include "SendMessage.h"
#include "Sockets.h"
#include "Threads.h"
#include "ThreadedSocket.h"
#include "TrackerNET_Interface.h"
#include "TrackerProtocol.h"
#include "UtlLinkedList.h"
#include "UtlRBTree.h"

#include <math.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>

#pragma warning(disable: 4244)  // warning C4244: 'argument' : conversion from 'int' to 'unsigned char', possible loss of data
								// disabled for the CUtlLinkedList that uses an unsigned char as it's index
#pragma warning(disable: 4710)	// warning C4710: function not expanded

// networking constants
static const float ACK_DELAY = 0.2f;
static const float RESEND_TIME = 1.5f;		// time before resending an unacknowledged packet
static const int   MAX_SEND_ATTEMPTS = 5;	// max number of times a packet is sent before we fail
static const int   MESSAGE_WINDOW = 16;		// range of packets to hold in sliding message window

// used for mapping ip's to targets
struct TargetMapItem_t
{
	CNetAddress netAddress;
	int targetIndex;
};

//-----------------------------------------------------------------------------
// Purpose: implementation of the trackerNet interface
//-----------------------------------------------------------------------------
class CTrackerNET : public ITrackerNET
{
public:
	CTrackerNET();
	~CTrackerNET();

	// sets up the networking thread and the port to listen on
	bool Initialize(unsigned short portMin, unsigned short portMax);
	// closes the network; if flush is true, then it blocks until the worker threads have completed
	void Shutdown(bool flush);

	void RunFrame();

	// message receiving
	IReceiveMessage *GetIncomingData();		// Gets any info has been received, returns NULL if no more
	IReceiveMessage *GetFailedMessage();
	void ReleaseMessage(IReceiveMessage *); // frees a received message

	IBinaryBuffer *GetIncomingRawData(CNetAddress &address);	// gets any info that has been received from out-of-band engine packets
	IBinaryBuffer *CreateRawMessage();
	void SendRawMessage(IBinaryBuffer *message, CNetAddress &address);

	// message sending
	ISendMessage *CreateMessage(int msgID);
	ISendMessage *CreateMessage(int msgID, void const *pBuf, int bufSize);
	ISendMessage *CreateReply(int msgID, IReceiveMessage *msgToReplyTo);
	void SendMessage(ISendMessage *pMsg, Reliability_e state);

	// network addresses
	CNetAddress GetNetAddress(const char *stringAddress);
	CNetAddress GetLocalAddress();

	// thread API access
	IThreads *GetThreadAPI();
	int GetPort();

	virtual void deleteThis()
	{
		delete this;
	}

	// returns the number of buffers current in the input and output queues
	int BuffersInQueue();
	int PeakBuffersInQueue();
	int BytesSent();
	int BytesReceived();
	int BytesSentPerSecond();
	int BytesReceivedPerSecond();

	void SetWindowsEvent(unsigned long event);

private:
	// connection creation
	struct NetworkTarget_t;
	int FindTarget(CNetAddress &address, int targetID = -1, int sequenceNumber = -1);
	void InternalSendMessage(ISendMessage *pMsg, NetworkTarget_t *target, int sequenceNumber);
	void ParseIncomingPacket(IBinaryBuffer *packet, CNetAddress netAddress, bool encrypted);
	void CheckReliablePacketSending();
	void CheckSendingAcknowledgements();
	void UpdateSequence(NetworkTarget_t *target, int targetIndex, int incomingSequence);
	void HandleOutOfSequencePacket(IBinaryBuffer *packet, packet_header_t *pHeader, NetworkTarget_t *target, CNetAddress &netAddress, bool encrypted);

	int FindTargetIndexByAddress(CNetAddress &addr);
	
#ifdef _DEBUG
	// debug utility function
	void WriteToLog(const char *str, ...);
#else
	void WriteToLog(const char *str, ...) {}
#endif
	// pointers to our networking interfaces
	IThreadedSocket *m_pThreadedSocket;
	IThreads *m_pThreads;
	ISockets *m_pSockets;

	float m_flNextFrameTime;	// next time at which we should Run a frame

	unsigned long m_hEvent;		// handle to event to signal

	// holds a single sent message, in case it has to be resent
	struct SentMessage_t
	{
		int sequenceNumber;
		float resendTime;		// time to resend (in seconds)
		int resendAttempt;		// the number of resends sent
		int networkTargetHandle;	// handle to the network target this was sent from
		int networkTargetID;
		ISendMessage *message;
	};

	// holds messages we are received, but not ready to use
	struct FutureMessage_t
	{
		int sequenceNumber;
		IReceiveMessage *message;
	};

	// target list
	struct NetworkTarget_t
	{
		NetworkTarget_t() : m_MessageWindow(MESSAGE_WINDOW, 0) {}

		CNetAddress netAddress;
		int targetID;				// unique identifier of this connection

		int incomingSequence;		// the highest valid ID received from the target
		int incomingAcknowledged;	// the highest valid ID received from the target that we have sent an acknowledgement to

		int outgoingSequence;		// the highest (most recent) packet ID sent
		int outgoingAcknowledged;	// the highest packet ID that has been ack'd by the target
									// packets in the range (outgoingAcknowledged, outgoingSequence] will get auto-resent

		float ackTime;				// time at which the next ack can be sent
		bool needAck;				// true if an ack has been marked to be send

//		float createTime;			// time at which this connection was created (unused)
//		float activityTime;			// time at which there was last activity on this connection (unused for now)
		
		CUtlLinkedList<FutureMessage_t, unsigned char> m_MessageWindow;
	};

	// index-based linked list of network targets - essentially safe handles to targets
	CUtlLinkedList<NetworkTarget_t, int> m_TargetList;
	int m_iHighestAckTarget;

	// list of messages sent reliably, and may need to be resent
	CUtlLinkedList<SentMessage_t, int> m_ReliableMessages;

	// map to the targets
	CUtlRBTree<TargetMapItem_t, int> m_TargetMap;

	// holds the list of all failed messages
	struct FailedMsg_t
	{
		IReceiveMessage *message;
	};
	CUtlLinkedList<FailedMsg_t, int> m_FailedMsgs;

	// messages that we've received, in order
	struct ReceivedMsg_t
	{
		IReceiveMessage *message;
	};
	CUtlLinkedList<ReceivedMsg_t, int> m_ReceivedMsgs;
};

EXPOSE_INTERFACE(CTrackerNET, ITrackerNET, TRACKERNET_INTERFACE_VERSION);

//-----------------------------------------------------------------------------
// Purpose: comparison function for TargetMapItem_t
//-----------------------------------------------------------------------------
bool TargetMapLessFunc(const TargetMapItem_t &t1, const TargetMapItem_t &t2)
{
	return (t1.netAddress < t2.netAddress);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTrackerNET::CTrackerNET()
{
	m_flNextFrameTime = 0.0f;
	m_iHighestAckTarget = -1;
	m_hEvent = 0;

	m_TargetMap.SetLessFunc(TargetMapLessFunc);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTrackerNET::~CTrackerNET()
{
	if (m_pThreadedSocket)
	{
		m_pThreadedSocket->deleteThis();
		m_pThreadedSocket = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the networking
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTrackerNET::Initialize(unsigned short portMin, unsigned short portMax)
{
	// initialize threading
	CreateInterfaceFn factory = Sys_GetFactoryThis();
	m_pThreads = (IThreads *)factory(THREADS_INTERFACE_VERSION, NULL);

	// initialize socket layer
	m_pThreadedSocket = (IThreadedSocket *)factory(THREADEDSOCKET_INTERFACE_VERSION, NULL);
	m_pSockets = m_pThreadedSocket->GetSocket();
	if (!m_pSockets->InitializeSockets(portMin, portMax))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: closes and shutsdown the network
// Input  : flush - if flush is true, then it blocks until the worker threads have completed
//-----------------------------------------------------------------------------
void CTrackerNET::Shutdown(bool flush)
{
	m_pThreadedSocket->Shutdown(flush);

	m_pThreadedSocket->deleteThis();
	m_pThreadedSocket = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Runs a networking frame, handling reliable packet delivery
//-----------------------------------------------------------------------------
void CTrackerNET::RunFrame()
{
	float time = (float)m_pThreads->GetTime();

	// don't Run the frame more than 20 times per second
	if (time < m_flNextFrameTime)
		return;

	m_flNextFrameTime = time + 0.05f;

	// read in any new packets
	IBinaryBuffer *recvBuf = NULL;
	bool encrypted = false;
	CNetAddress address;

	// don't read too many packets in one frame
	int packetsToRead = 10;
	while (m_pThreadedSocket->GetNewMessage(&recvBuf, &address, encrypted) && packetsToRead--)
	{
		ParseIncomingPacket(recvBuf, address, encrypted);
	}

	// check to see if we need to send any ack packets in response to reliable messages we've received
	CheckSendingAcknowledgements();

	// see if any of our packets need to be resent
	CheckReliablePacketSending();
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if we need to send any ack packets 
//			in response to reliable messages we've received
//-----------------------------------------------------------------------------
void CTrackerNET::CheckSendingAcknowledgements()
{
	float time = (float)m_pThreads->GetTime();

	// the target list is in order of ackTime's
	while (m_TargetList.Count())
	{
		int targetIndex = m_TargetList.Head();
		NetworkTarget_t &target = m_TargetList[targetIndex];

		if (!target.needAck)
			break;	// target does not need acknowledging, we're at the end of the list

		if (target.ackTime > time)
			break;	// this packet not yet ready to be acknowledged

		if (target.incomingSequence > target.incomingAcknowledged)
		{
			// send an acknowledgement
			ISendMessage *pMsg = CreateMessage(TMSG_ACK);
			pMsg->SetNetAddress(target.netAddress);
			SendMessage(pMsg, NET_UNRELIABLE);
		}

		// this target has been acknowledged, move to the end of the list
		target.needAck = false;
		m_TargetList.Unlink(targetIndex);
		m_TargetList.LinkToTail(targetIndex);

		// check to see if the highest ack target needs to be reset
		if (targetIndex == m_iHighestAckTarget)
		{
			m_iHighestAckTarget = -1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if we should resend any reliable packets
//-----------------------------------------------------------------------------
void CTrackerNET::CheckReliablePacketSending()
{
	float time = m_pThreads->GetTime();

	// reliable packets are held in the m_ReliableMessages queue in the order that they were sent
	// if a packet is resent, it is moved to the end of the queue
	// packets are also checked at this time to see if they have been acknowledged
	while (m_ReliableMessages.Count())
	{
		// get the first message
		int index = m_ReliableMessages.Head();

		// get the message
		SentMessage_t &msg = m_ReliableMessages[index];

		// check the resend time, if it's not time to resend then we have nothing more to do 
		if (msg.resendTime > time)
			break;	

		// get the network target for this message
		if (!m_TargetList.IsValidIndex(msg.networkTargetHandle) 
			|| !m_TargetList.IsInList(msg.networkTargetHandle) 
			|| m_TargetList[msg.networkTargetHandle].targetID != msg.networkTargetID)
		{
			// message's target has been removed, kill
			if (msg.message)
			{
				msg.message->deleteThis();
			}
			m_ReliableMessages.Remove(index);
			continue;
		}

		NetworkTarget_t &target = m_TargetList[msg.networkTargetHandle];

		// check to see if it's already been acknowledged
		if (msg.sequenceNumber <= target.outgoingAcknowledged)
		{
			// message has been acknowledged, kill
			if (msg.message)
			{
				msg.message->deleteThis();
			}
			m_ReliableMessages.Remove(index);

			// move onto next message
			continue;
		}

		// check to see if we should resend this packet
		msg.resendAttempt++;
		if (msg.resendAttempt < MAX_SEND_ATTEMPTS)
		{
			// only send the message if it's in the message send window
			if (msg.sequenceNumber < target.outgoingAcknowledged + MESSAGE_WINDOW)
			{
				WriteToLog("-> Resending '%d' (os:%d > %d) (%s)\n", msg.message->GetMsgID(), msg.sequenceNumber, target.outgoingAcknowledged, target.netAddress.ToStaticString());

				// send that it again
				InternalSendMessage(msg.message, &target, msg.sequenceNumber);
			}
			else
			{
				// hold back on sending the message
				WriteToLog("-> Holding back resend '%d' (os:%d > %d) (%s)\n", msg.message->GetMsgID(), msg.sequenceNumber, target.outgoingAcknowledged, target.netAddress.ToStaticString());
				msg.resendAttempt--;
			}

			// set the time before retrying again
			// send times: 0.0 1.5 1.5 1.5 (1.5) = 6.0 second timeout, 4 packets sent
			msg.resendTime = time + RESEND_TIME;

			// move it to the end of the list
			m_ReliableMessages.Unlink(index);
			m_ReliableMessages.LinkToTail(index);

			// next packet
			continue;
		}

		// target has failed to respond, remove the target and respond with a failed message
		WriteToLog("Could not deliver packet: %d (os:%d)\n", msg.message->GetMsgID(), msg.sequenceNumber);

		// send back a send failure message to the app
		// convert the send message into a receive message
		CSendMessage *sendMsg = dynamic_cast<CSendMessage *>(msg.message);
		if (sendMsg)
		{
			IBinaryBuffer *buf = sendMsg->GetBuffer();
			
			buf->SetPosition(buf->GetBufferData());
			buf->Advance(buf->GetReservedSize());
			CReceiveMessage *failMsg = new CReceiveMessage(buf, true);

			if (failMsg->IsValid())
			{
				failMsg->SetFailed();
				failMsg->SetNetAddress(target.netAddress);
				failMsg->SetSessionID(sendMsg->SessionID());

				int newIndex = m_FailedMsgs.AddToTail();
				FailedMsg_t &fmsg = m_FailedMsgs[newIndex];
				fmsg.message = failMsg;
			}
			else
			{
				delete failMsg;
			}
		}

		// target not responding, so cancel the connection
		// remove from target map
		int mapindex = FindTargetIndexByAddress(target.netAddress);
		assert(m_TargetMap.IsValidIndex(mapindex));
		m_TargetMap.RemoveAt(mapindex);

		// remove target from list
		m_TargetList.Remove(msg.networkTargetHandle);
		
		// message will automatically be delete since it's target is gone
	}
}

//-----------------------------------------------------------------------------
// Purpose: finds a network target by net address
//-----------------------------------------------------------------------------
int CTrackerNET::FindTargetIndexByAddress(CNetAddress &addr)
{
	TargetMapItem_t searchItem;
	searchItem.netAddress = addr;
	return m_TargetMap.Find(searchItem);
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of buffers current in the input and output queues
//-----------------------------------------------------------------------------
int CTrackerNET::BuffersInQueue()
{
	return m_pThreadedSocket->BuffersInQueue();
}

//-----------------------------------------------------------------------------
// Purpose: returns the peak number of buffers in the queues
//-----------------------------------------------------------------------------
int CTrackerNET::PeakBuffersInQueue()
{
	return m_pThreadedSocket->PeakBuffersInQueue();
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int CTrackerNET::BytesSent()
{
	return m_pThreadedSocket->BytesSent();
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int CTrackerNET::BytesReceived()
{
	return m_pThreadedSocket->BytesReceived();
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int CTrackerNET::BytesSentPerSecond()
{
	return m_pThreadedSocket->BytesSentPerSecond();
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int CTrackerNET::BytesReceivedPerSecond()
{
	return m_pThreadedSocket->BytesReceivedPerSecond();
}

//-----------------------------------------------------------------------------
// Purpose: sets the windows event to signal when we get a new message
//-----------------------------------------------------------------------------
void CTrackerNET::SetWindowsEvent(unsigned long event)
{
	m_hEvent = event;

	// set it in networking layer
	m_pThreadedSocket->SetWindowsEvent(event);
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message to a target address
//-----------------------------------------------------------------------------
void CTrackerNET::SendMessage(ISendMessage *pMsg, Reliability_e state)
{
	pMsg->SetReliable(state == NET_RELIABLE);

	if (state == NET_RELIABLE && (pMsg->NetAddress().IP() == 0 || pMsg->NetAddress().Port() == 0))
		return;

	assert(state != NET_RELIABLE || pMsg->NetAddress().IP() > 0);
	assert(state != NET_RELIABLE || pMsg->NetAddress().Port() > 0);

	CNetAddress address;
	int sequenceNumber = 0;
	int targetIndex = -1;
	NetworkTarget_t *target = NULL;
	if (state != NET_BROADCAST)
	{
		targetIndex = FindTarget(pMsg->NetAddress());
		target = &m_TargetList[targetIndex];

		// mark the activity time
//		target->activityTime = m_pThreads->GetTime();

		// setup the message with the target info
		address = target->netAddress;
	}

	if (pMsg->IsReliable())
	{
		// set and increment the sequence number
		if (target)
		{
			sequenceNumber = target->outgoingSequence++;
		}

		// save off the message
		int newIndex = m_ReliableMessages.AddToTail();
		SentMessage_t &msg = m_ReliableMessages[newIndex];
		msg.sequenceNumber = sequenceNumber;
		msg.message = pMsg;
		msg.resendAttempt = 0;
		msg.resendTime = m_pThreads->GetTime() + RESEND_TIME;
		msg.networkTargetHandle = targetIndex;
		msg.networkTargetID = target->targetID;

		// make sure we're in the message window before sending, other just hold until the resend attempt
		if (msg.sequenceNumber < target->outgoingAcknowledged + MESSAGE_WINDOW)
		{
			WriteToLog("-> Sending '%d' (os:%d) (%s)\n", pMsg->GetMsgID(), sequenceNumber, target->netAddress.ToStaticString());
		}
		else
		{
			// don't send yet
			WriteToLog("-> Holding for send '%d' (os:%d) (%s)\n", pMsg->GetMsgID(), sequenceNumber, target->netAddress.ToStaticString());
			return;
		}
	}
	else if (state == NET_BROADCAST)
	{
		WriteToLog("-> Sending '%d' (BROADCAST)\n", pMsg->GetMsgID(), sequenceNumber);
	}
	else
	{
		WriteToLog("-> Sending '%d' (UNRELIABLE)\n", pMsg->GetMsgID(), sequenceNumber);
	}

	InternalSendMessage(pMsg, target, sequenceNumber);
}

//-----------------------------------------------------------------------------
// Purpose: Internal message send
//-----------------------------------------------------------------------------
void CTrackerNET::InternalSendMessage(ISendMessage *pMsg, NetworkTarget_t *target, int sequenceNumber)
{
	// ugly cast
	CSendMessage *sendMsg = dynamic_cast<CSendMessage *>(pMsg);

	if (target)
	{
		// reply with the last received sequence number
		int sequenceReply = target->incomingSequence;

		// mark as have acknowledged this sequence
		target->incomingAcknowledged = target->incomingSequence;

		// send it
		m_pThreadedSocket->SendMessage(sendMsg->GetBuffer(), target->netAddress, sendMsg->SessionID(), target->targetID, sequenceNumber, sequenceReply, pMsg->IsEncrypted());
	}
	else
	{
		// no target, so it must be a broadcast msg
		m_pThreadedSocket->BroadcastMessage(sendMsg->GetBuffer(), pMsg->NetAddress().Port());
	}

	// delete the unreliable messages since we don't have the stored in the resend buffer
	if (!pMsg->IsReliable())
	{
		delete sendMsg;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks for any waiting messages
//-----------------------------------------------------------------------------
IReceiveMessage *CTrackerNET::GetIncomingData()
{
	// check the receive list
	if (m_ReceivedMsgs.Count())
	{
		int head = m_ReceivedMsgs.Head();
		IReceiveMessage *msg = m_ReceivedMsgs[head].message;
		m_ReceivedMsgs.Remove(head);
		return msg;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Gets any failed sends - returns any packet that could not be delivered
//-----------------------------------------------------------------------------
IReceiveMessage *CTrackerNET::GetFailedMessage()
{
	if (m_FailedMsgs.Count())
	{
		// pop the failed message from the front of the queue and return it
		FailedMsg_t &msg = m_FailedMsgs[m_FailedMsgs.Head()];
		IReceiveMessage *recvMsg = msg.message;
		m_FailedMsgs.Remove(m_FailedMsgs.Head());
		return recvMsg;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Creates a buffer
// Output : IBinaryBuffer
//-----------------------------------------------------------------------------
IBinaryBuffer *CTrackerNET::CreateRawMessage()
{
	return Create_BinaryBuffer(PACKET_DATA_MAX, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Sends a raw data buffer
// Input  : *message - 
//			&address - 
//-----------------------------------------------------------------------------
void CTrackerNET::SendRawMessage(IBinaryBuffer *message, CNetAddress &address)
{
	m_pThreadedSocket->SendRawMessage(message, address);
}


//-----------------------------------------------------------------------------
// Purpose: gets any info that has been received from out-of-band engine packets
//-----------------------------------------------------------------------------
IBinaryBuffer *CTrackerNET::GetIncomingRawData(CNetAddress &address)
{
	IBinaryBuffer *recvBuf = NULL;
		
	if (m_pThreadedSocket->GetNewNonTrackerMessage(&recvBuf, &address))
	{
		return recvBuf;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Constructs a network message out of the packet
//-----------------------------------------------------------------------------
void CTrackerNET::ParseIncomingPacket(IBinaryBuffer *packet, CNetAddress netAddress, bool encrypted)
{
	// construct a ReceiveMessage from it
	// parse out the packet header
	void *bufData = packet->GetBufferData();
	packet_header_t *pHeader = (packet_header_t *)bufData;

	// check the protocol version
	if (pHeader->protocolNumber != PROTOCOL_VERSION)
	{
		// just throw away the packet if protocol is bad
		WriteToLog("!! Packet with invalid protocol - dropped\n");
		packet->Release();
		return;
	}

	// check the packet header size
	if (pHeader->headerSize != PACKET_HEADER_SIZE)
	{
		// just throw away the packet if protocol is bad
		packet->Release();
		WriteToLog("!! Malformed packet - dropped\n");
		return;
	}

	// match to a target
	int targetIndex = FindTarget(netAddress, pHeader->targetID, pHeader->sequenceNumber);
	NetworkTarget_t *target = &m_TargetList[targetIndex];

	// mark the activity time
//	target->activityTime = m_pThreads->GetTime();

	// update received
	if (pHeader->sequenceReply)
	{
		if (pHeader->sequenceReply > target->outgoingAcknowledged)
		{
			WriteToLog("<- Sequence acknowledged (%d in target %d)\n", target->outgoingAcknowledged, target->targetID);

			// this affects which packets we have to resend
			target->outgoingAcknowledged = pHeader->sequenceReply;
		}

		if (pHeader->sequenceReply > target->outgoingSequence)
		{
			// we've got a mismatch in the network stream; the Client is acknowledging a packet we haven't sent yet
			// just bump our outgoing up to match
			WriteToLog("<- Mismatched sequence numbers; increasing local outgoingSequence to match (%d -> %d)\n", target->outgoingSequence, pHeader->sequenceReply);
			target->outgoingSequence = pHeader->sequenceReply + 1;
		}
	}
	if (pHeader->sequenceNumber)
	{
		// check it against the incoming sequence number
		// if it's not the next packet in the sequence, just throw it away
		if (pHeader->sequenceNumber != target->incomingSequence + 1 && (target->incomingSequence > 0))
		{
			HandleOutOfSequencePacket(packet, pHeader, target, netAddress, encrypted);
			return;
		}
	}

	// advance in the buffer over the packet header
	packet->Advance(pHeader->headerSize);
	
	//!! check for fragmentation/reassembly

	// build a receive message out of it
	CReceiveMessage *msg = new CReceiveMessage(packet, encrypted);
	msg->SetNetAddress(netAddress);
	msg->SetSessionID(pHeader->sessionID);

	// throw away bad messages
	if (!msg->IsValid())
	{
		delete msg;
		return;
	}

	WriteToLog("<- Received '%d' (is:%d reply:%d)\n", msg->GetMsgID(), pHeader->sequenceNumber, pHeader->sequenceReply);

	// throw away any Ack packets
	if (msg->GetMsgID() == TMSG_ACK)
	{
		delete msg;
		return;
	}

	// add message to received list
	int receivedIndex = m_ReceivedMsgs.AddToTail();
	m_ReceivedMsgs[receivedIndex].message = msg;

	if (pHeader->sequenceNumber)
	{
		// update the sequence, this may add more messages to the queue
		UpdateSequence(target, targetIndex, pHeader->sequenceNumber);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the current sequence info with the new packet
//			scans the message window to see if future packets can be used
//-----------------------------------------------------------------------------
void CTrackerNET::UpdateSequence(NetworkTarget_t *target, int targetIndex, int seqNum)
{
	// update sequence
	target->incomingSequence = seqNum;

	// check to see when we should send acknowledgement
	if (!target->needAck)
	{
		target->ackTime = m_pThreads->GetTime() + ACK_DELAY;
		target->needAck = true;

		// move the target to the end of the ack list
		m_TargetList.Unlink(targetIndex);

		if (m_TargetList.IsValidIndex(m_iHighestAckTarget) && m_TargetList.IsInList(m_iHighestAckTarget))
		{
			m_TargetList.LinkAfter(m_iHighestAckTarget, targetIndex);
		}
		else
		{
			m_TargetList.LinkToHead(targetIndex);
		}

		// set the need high mark for the packets that need ack
		m_iHighestAckTarget = targetIndex;
	}

	// check to see if packets in the window are now valid
	while (target->m_MessageWindow.Count())
	{
		int index = target->m_MessageWindow.Head();
		
		if (target->m_MessageWindow[index].sequenceNumber == target->incomingSequence + 1)
		{
			// this is the next message

			// update sequence numbers
			target->incomingSequence = target->m_MessageWindow[index].sequenceNumber;

			// add message to received list
			int receivedIndex = m_ReceivedMsgs.AddToTail();
			m_ReceivedMsgs[receivedIndex].message = target->m_MessageWindow[index].message;

			WriteToLog("-> Recv match in window (is: %d)\n", target->m_MessageWindow[index].sequenceNumber);

			// remove message from window
			target->m_MessageWindow.Remove(index);
		}
		else
		{
			// no more
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Deals with when we receive a reliable packet out of sequence
//-----------------------------------------------------------------------------
void CTrackerNET::HandleOutOfSequencePacket(IBinaryBuffer *packet, packet_header_t *pHeader, NetworkTarget_t *target, CNetAddress &netAddress, bool encrypted)
{
	if (pHeader->sequenceNumber <= target->incomingSequence)
	{
		// mark that we may need to re-acknowledge their packets (since they could lose the acknowledgement)
		target->incomingAcknowledged = (target->incomingSequence - 1);
		// throw away the packet, since we've already acknowledged it
		packet->Release();
		return;
	}

	// it's a message from the future, maybe we should hold it

	// if it's too far in the future, dump it
	if (pHeader->sequenceNumber > (target->incomingSequence + MESSAGE_WINDOW))
	{
		packet->Release();
		return;
	}
	// add to message window - we can use it later when we've got the missing packets from the sequence

	// find where the packet should go in the list
	// message should be in order of lowest sequence number to highest
	int insertAfterIndex = target->m_MessageWindow.Tail();
	while (target->m_MessageWindow.IsValidIndex(insertAfterIndex))
	{
		int incseq = target->m_MessageWindow[insertAfterIndex].sequenceNumber;
		if (incseq < pHeader->sequenceNumber)
		{
			// this has a lower sequence number, so we should insert after it
			break;
		}
		else if (incseq == pHeader->sequenceNumber)
		{
			// we already have this sequence number, so throw this duplicate away
			packet->Release();
			return;
		}

		insertAfterIndex = target->m_MessageWindow.Previous(insertAfterIndex);
	}

	// build into message
	// advance in the buffer over the packet header
	packet->Advance(pHeader->headerSize);
	// build a receive message out of it
	CReceiveMessage *msg = new CReceiveMessage(packet, encrypted);
	msg->SetNetAddress(netAddress);
	msg->SetSessionID(pHeader->sessionID);
	// throw away bad messages
	if (!msg->IsValid())
	{
		delete msg;
		return;
	}

	// add it into the list
	int newIndex = target->m_MessageWindow.InsertAfter(insertAfterIndex);
	target->m_MessageWindow[newIndex].sequenceNumber = pHeader->sequenceNumber;
	target->m_MessageWindow[newIndex].message = msg;

	WriteToLog("-> Inserting recv into window (is: %d)\n", target->m_MessageWindow[newIndex].sequenceNumber);
}


//-----------------------------------------------------------------------------
// Purpose: Frees the message buffer
//-----------------------------------------------------------------------------
void CTrackerNET::ReleaseMessage(IReceiveMessage *msg)
{
	msg->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: Returns an interface to the networkings thread API
// Output : IThreads *
//-----------------------------------------------------------------------------
IThreads *CTrackerNET::GetThreadAPI( void )
{
	return m_pThreads;
}

//-----------------------------------------------------------------------------
// Purpose: Creates an empty network message
//-----------------------------------------------------------------------------
ISendMessage *CTrackerNET::CreateMessage(int msgID)
{
	return new CSendMessage(msgID, Create_BinaryBuffer(PACKET_DATA_MAX, PACKET_HEADER_SIZE));
}

//-----------------------------------------------------------------------------
// Purpose: Creates a preconstructed message
//-----------------------------------------------------------------------------
ISendMessage *CTrackerNET::CreateMessage(int msgID, void const *pBuf, int bufSize)
{
	return new CSendMessage(msgID, pBuf, bufSize, Create_BinaryBuffer(PACKET_DATA_MAX, PACKET_HEADER_SIZE));
}

//-----------------------------------------------------------------------------
// Purpose: Creates a reply to the message
// Input  : *msgName - 
//			*msgToReplyTo - 
// Output : ISendMessage
//-----------------------------------------------------------------------------
ISendMessage *CTrackerNET::CreateReply(int msgID, IReceiveMessage *msgToReplyTo)
{
	ISendMessage *reply = CreateMessage(msgID);
	
	// setup reply
	reply->SetNetAddress(msgToReplyTo->NetAddress());
	reply->SetSessionID(msgToReplyTo->SessionID());
	reply->SetEncrypted(msgToReplyTo->WasEncrypted());

	return reply;
}

//-----------------------------------------------------------------------------
// Purpose: Converts a string to a net address
// Input  : *stringAddress - 
// Output : CNetAddress
//-----------------------------------------------------------------------------
CNetAddress CTrackerNET::GetNetAddress(const char *stringAddress)
{
	CNetAddress addr;
	int ip;
	unsigned short port;
	m_pSockets->StringToAddress( stringAddress, &ip, &port );
	addr.SetIP( ip );
	addr.SetPort( port );
	return addr;
}

//-----------------------------------------------------------------------------
// Purpose: Returns port number
// Output : int
//-----------------------------------------------------------------------------
int CTrackerNET::GetPort()
{
	return m_pSockets->GetListenPort();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the address of the local machine
// Output : CNetAddress
//-----------------------------------------------------------------------------
CNetAddress CTrackerNET::GetLocalAddress()
{
	return m_pSockets->GetLocalAddress();
}

//-----------------------------------------------------------------------------
// Purpose: Creates a target address, for sending messages to reliably
//-----------------------------------------------------------------------------
int CTrackerNET::FindTarget(CNetAddress &address, int targetID, int sequenceNumber)
{
	bool createNew = true;
	NetworkTarget_t *target = NULL;
	int listIndex = 0;

	// find the address in the map
	int targetIndex = FindTargetIndexByAddress(address);
	if (m_TargetMap.IsValidIndex(targetIndex))
	{
		listIndex = m_TargetMap[targetIndex].targetIndex;
		target = &(m_TargetList[listIndex]);

		// make sure the targetID's match, otherwise the connection has been invalidated
		if (targetID == -1 || target->targetID == targetID)
		{
			return listIndex;
		}
		else
		{
			if (sequenceNumber == 1)
			{
				//!! reset the connection
				WriteToLog("-> Connection Reset (%d) (%d, %d)\n", sequenceNumber, targetID, target->targetID);
			}
			else
			{
				WriteToLog("-> Connection Restart (%d) (%d, %d)\n", sequenceNumber, targetID, target->targetID);
			}
			createNew = false;
		}
	}

	if (createNew)
	{
		// add a new connection to the end of the list
		listIndex = m_TargetList.AddToTail();
		target = &(m_TargetList[listIndex]);
		target->needAck = false;

		// add the target to the map
		TargetMapItem_t newItem;
		newItem.targetIndex = listIndex;
		newItem.netAddress = address;
		m_TargetMap.Insert(newItem);

		// ensure that it's the first one in the sequence
		if (sequenceNumber > 1)
		{
			WriteToLog("-> Contacted by unknown old connection (targetID:%d)\n", targetID);
		}
	}

	// append to the end of the list
	target->netAddress = address;

	// initialize sequence numbers
	target->outgoingSequence = 1;
	target->outgoingAcknowledged = 0;
	target->incomingSequence = 0;
	target->incomingAcknowledged = 0;

	if (sequenceNumber != -1)
	{
		// this is the first packet we've received, but from and old connection; so jump the correct sequence number
		target->incomingSequence = sequenceNumber - 1;
	}

//	target->createTime = m_pThreads->GetTime();
	if (targetID >= 0)
	{
		target->targetID = targetID;
	}
	else
	{
		target->targetID = RandomLong(1, MAX_RANDOM_RANGE);
	}

	if (createNew && targetID >= 0)
	{
		WriteToLog("Creating new target ID (%d)\n", target->targetID);
	}
	else
	{
		WriteToLog("Establishing connection to target ID (%d)\n", target->targetID);
	}

	return listIndex;
}

#ifdef _DEBUG
//-----------------------------------------------------------------------------
// Purpose: logs networking info to a text file
// Input  : *str - 
//			... - 
//-----------------------------------------------------------------------------
void CTrackerNET::WriteToLog(const char *format, ...)
{
	static bool firstThrough = true;

	FILE *f;
	if (firstThrough)
	{
		f = fopen("netlog.txt", "wt");
		firstThrough = false;
	}
	else
	{
		f = fopen("netlog.txt", "at");
	}
	if (!f)
		return;

	char    buf[2048];
	va_list argList;

	va_start(argList,format);
	vsprintf(buf, format, argList);
	va_end(argList);


	float time = m_pThreads->GetTime();
	fprintf(f, "(%.2f) %s", time, buf);

	fclose(f);
}

#endif // _DEBUG
