//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "game_controls/iviewportmsgs.h"
#include "c_user_message_register.h"


void __MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf )
{
	gViewPortMsgs->MsgFunc_MOTD( pszName, iSize, pbuf );
}
USER_MESSAGE_REGISTER( MOTD );


void __MsgFunc_VGUIMenu( const char *pszName, int iSize, void *pbuf )
{
	gViewPortMsgs->MsgFunc_VGUIMenu( pszName, iSize, pbuf );
}
USER_MESSAGE_REGISTER( VGUIMenu );



