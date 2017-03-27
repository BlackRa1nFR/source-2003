//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
//=============================================================================

#ifndef ADDONMESSAGES_H
#define ADDONMESSAGES_H
#pragma once

enum GameMessageIDs
{
	GAME_MSG_TEXT = 1,			// text chat message
	GAME_MSG_DATA,				// binary game data

	GAME_MSG_INVITE_REQUEST,
	GAME_MSG_INVITE_RESPONSE,
	GAME_MSG_REJOIN_REQUEST,
	GAME_MSG_REJOIN_RESPONSE,

	GAME_MSG_INVITE_PERMISSION,			// ask permission to invite someone
	GAME_MSG_INVITE_NOTIFY,				// tell everybody that we're inviting
	GAME_MSG_INVITE_DENIED,

	GAME_MSG_PLAYER_STATUS_UPDATE,
	GAME_MSG_SETUP_INFO,				// when user joins a game, host send the setup information of who's in the game
	GAME_MSG_INVITE_CANCEL,				// host has cancelled an invite
	GAME_MSG_GAME_START,				// if a game has a setup phase, this tells everybody the game has started

	GAME_MSG_STARTING_CARD_HAND = 300,
	GAME_MSG_STARTING_PLAYER,
	GAME_MSG_CARD_PLAY,

	GAME_MSG_CHEAT_POSSIBLE = 400,			// when host detects a possible cheat

	GAME_MSG_MOVE = 500,
	GAME_MSG_COLOR_CHOICE,
	GAME_MSG_RECONNECT_DATA,
	GAME_MSG_QUIT,
	GAME_MSG_PASS,

	GAME_MSG_ABORT,				// phase these out
	GAME_MSG_WAITING_ABORT,
	
	GAME_MSG_CLOSE_WINDOW,

	// special individual game messages should take IDs 1000 and over
};


#endif // ADDONMESSAGES_H

