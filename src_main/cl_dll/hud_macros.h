//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( HUD_MACROS_H )
#define HUD_MACROS_H
#ifdef _WIN32
#pragma once
#endif

#include "usermessages.h"

// Macros to hook function calls into the HUD object
#define HOOK_MESSAGE(x) usermessages->HookMessage(#x, __MsgFunc_##x );
// Message declaration for non-CHudElement classes
#define DECLARE_MESSAGE(y, x) void __MsgFunc_##x(const char *pszName, int iSize, void *pbuf) \
	{											\
		y.MsgFunc_##x(pszName, iSize, pbuf );	\
	}
// Message declaration for CHudElement classes that use the hud element factory for creation
#define DECLARE_HUD_MESSAGE(y, x) void __MsgFunc_##x(const char *pszName, int iSize, void *pbuf) \
	{																\
		CHudElement *pElement = gHUD.FindElement( #y );				\
		if ( pElement )												\
		{															\
			((##y##*)pElement)->MsgFunc_##x(pszName, iSize, pbuf );	\
		}															\
	}

// Commands
#define HOOK_COMMAND(x, y) static ConCommand x( #x, __CmdFunc_##y );
// Command declaration for non CHudElement classes
#define DECLARE_COMMAND(y, x) void __CmdFunc_##x( void ) \
	{							\
		y.UserCmd_##x( );		\
	}
// Command declaration for CHudElement classes that use the hud element factory for creation
#define DECLARE_HUD_COMMAND(y, x) void __CmdFunc_##x( void )									\
	{																\
		CHudElement *pElement = gHUD.FindElement( #y );				\
		{															\
			((##y##*)pElement)->UserCmd_##x( );						\
		}															\
	}

#define DECLARE_HUD_COMMAND_NAME(y, x, name) void __CmdFunc_##x( void )									\
	{																\
		CHudElement *pElement = gHUD.FindElement( name );			\
		{															\
			((##y##*)pElement)->UserCmd_##x( );						\
		}															\
	}


#endif // HUD_MACROS_H