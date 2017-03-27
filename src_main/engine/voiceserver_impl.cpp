
//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: This module implements the IVoiceServer interface.
//
// $NoKeywords: $
//=============================================================================

#include "quakedef.h"
#include "server.h"
#include "ivoiceserver.h"


class CVoiceServer : public IVoiceServer
{
public:
	
	virtual bool	GetClientListening(int iReceiver, int iSender)
	{
		// Make into client indices..
		--iReceiver;
		--iSender;

		if(iReceiver < 0 || iReceiver >= svs.maxclients || iSender < 0 || iSender >= svs.maxclients)
			return false;

		return !!svs.clients[iSender].m_VoiceStreams[iReceiver >> 5] & (1 << (iReceiver & 31));
	}

	
	virtual bool	SetClientListening(int iReceiver, int iSender, bool bListen)
	{
		unsigned long *pDest;

		// Make into client indices..
		--iReceiver;
		--iSender;
		
		if(iReceiver < 0 || iReceiver >= svs.maxclients || iSender < 0 || iSender >= svs.maxclients)
			return false;

		pDest = &svs.clients[iSender].m_VoiceStreams[iReceiver >> 5];
		if(bListen)
			*pDest |=  (1 << (iReceiver & 31));
		else
			*pDest &= ~(1 << (iReceiver & 31));

		return true;
	}	
};


EXPOSE_SINGLE_INTERFACE(CVoiceServer, IVoiceServer, INTERFACEVERSION_VOICESERVER);
