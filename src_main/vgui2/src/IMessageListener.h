//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef IMESSAGELISTENER_H
#define IMESSAGELISTENER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

class KeyValues;

namespace vgui
{

enum MessageSendType_t
{
	MESSAGE_SENT = 0,
	MESSAGE_POSTED,
	MESSAGE_RECEIVED
};

class VPanel;


class IMessageListener
{
public:
	virtual void Message( VPanel* pSender, VPanel* pReceiver, 
		KeyValues* pKeyValues, MessageSendType_t type ) = 0;
};

IMessageListener* MessageListener();

}

#endif // IMESSAGELISTENER_H
