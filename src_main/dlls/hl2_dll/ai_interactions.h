//==================================================
// Definition for all AI interactions
//==================================================

#ifndef	AI_INTERACTIONS_H
#define	AI_INTERACTIONS_H
#pragma once

//Antlion
extern int	g_interactionAntlionKilled;

//Barnacle
extern int	g_interactionBarnacleVictimDangle;
extern int	g_interactionBarnacleVictimReleased;
extern int	g_interactionBarnacleVictimGrab;

//Bullsquid
//extern int	g_interactionBullsquidPlay;
extern int	g_interactionBullsquidThrow;

//Combine
extern int	g_interactionCombineKick;
extern int	g_interactionCombineRequestCover;

//Houndeye
extern int	g_interactionHoundeyeGroupAttack;
extern int	g_interactionHoundeyeGroupRetreat;
extern int	g_interactionHoundeyeGroupRalley;

//Scanner
extern int	g_interactionScannerInspect;
extern int	g_interactionScannerInspectBegin;
extern int	g_interactionScannerInspectDone;
extern int	g_interactionScannerInspectHandsUp;
extern int	g_interactionScannerInspectShowArmband;
extern int	g_interactionScannerSupportEntity;
extern int	g_interactionScannerSupportPosition;

//ScriptedTarget
extern int  g_interactionScriptedTarget;

//Stalker
extern int	g_interactionStalkerBurn;

//Vortigaunt
extern int	g_interactionVortigauntStomp;
extern int	g_interactionVortigauntStompFail;
extern int	g_interactionVortigauntStompHit;
extern int	g_interactionVortigauntKick;
extern int	g_interactionVortigauntClaw;

//Wasteland scanner
extern int	g_interactionWScannerBomb;

#endif	//AI_INTERACTIONS_H