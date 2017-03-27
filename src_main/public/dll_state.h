#ifndef DLL_STATE_H
#define DLL_STATE_H
#pragma once

//DLL State Flags

enum
{
	DLL_INACTIVE = 0,		// no dll
	DLL_ACTIVE,				// engine is focused
	DLL_CLOSE,				// closing down dll
	DLL_RESTART,			// engine is shutting down but will restart right away
};

#endif		// DLL_STATE_H
