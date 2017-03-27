//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Interface of the CWeaponAward class, and its subclasses
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#ifndef WEAPONAWARDS_H
#define WEAPONAWARDS_H
#ifdef WIN32
#pragma once
#endif
#include "Award.h"

//------------------------------------------------------------------------------------------------------
// Purpose: CWeaponAward is the superclass for any award that is based simply
// on number of kills with a specific weapon. 
//------------------------------------------------------------------------------------------------------
class CWeaponAward: public CAward
{
protected:
	map<PID,int> accum;
	char* killtype;
public:
	CWeaponAward(char* awardname, char* killname):CAward(awardname),killtype(killname){}
	void getWinner();
};

//------------------------------------------------------------------------------------------------------
// Purpose: CFlamethrowerAward is an award given to the player who gets the 
// most kills with "flames"
//------------------------------------------------------------------------------------------------------
class CFlamethrowerAward: public CWeaponAward
{
protected:
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CFlamethrowerAward():CWeaponAward("Blaze of Glory","flames"){}
};

//------------------------------------------------------------------------------------------------------
// Purpose:  CAssaultCannonAward is an award given to the player who gets the
// most kills with "ac" (the assault cannon)
//------------------------------------------------------------------------------------------------------
class CAssaultCannonAward: public CWeaponAward
{
protected:
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CAssaultCannonAward():CWeaponAward("Swiss Cheese","ac"){}
};


//------------------------------------------------------------------------------------------------------
// Purpose: CKnifeAward is an award given to the player who gets the most kills
// with the "knife"
//------------------------------------------------------------------------------------------------------
class CKnifeAward: public CWeaponAward
{
protected:
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CKnifeAward():CWeaponAward("Assassin","knife"){}
};

//------------------------------------------------------------------------------------------------------
// Purpose: CRocketryAward is an award given to the player who gets the most kills
// with "rocket"s.
//------------------------------------------------------------------------------------------------------
class CRocketryAward: public CWeaponAward
{
protected:
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CRocketryAward():CWeaponAward("Rocketry","rocket"){}
};

//------------------------------------------------------------------------------------------------------
// Purpose: CGrenadierAward is an award given to the player who gets the most 
// kills with "gl_grenade"s
//------------------------------------------------------------------------------------------------------
class CGrenadierAward: public CWeaponAward
{
protected:
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CGrenadierAward():CWeaponAward("Grenadier","gl_grenade"){}
};

//------------------------------------------------------------------------------------------------------
// Purpose: CDemolitionsAward is an award given to the player who kills the most
// people with "detpack"s.
//------------------------------------------------------------------------------------------------------
class CDemolitionsAward: public CWeaponAward
{
protected:
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	explicit CDemolitionsAward():CWeaponAward("Demolitions","detpack"){}
};

//------------------------------------------------------------------------------------------------------
// Purpose: CBiologicalWarfareAward is given to the player who kills the most
// people with "infection"s
//------------------------------------------------------------------------------------------------------
class CBiologicalWarfareAward: public CWeaponAward
{
protected:
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	CBiologicalWarfareAward():CWeaponAward("Biological Warfare","infection"){}
};


//------------------------------------------------------------------------------------------------------
// Purpose:  CBestSentryAward is given to the player who kills the most people
// with sentry guns that he/she created ("sentrygun")
//------------------------------------------------------------------------------------------------------
class CBestSentryAward: public CWeaponAward
{
protected:
	void noWinner(CHTMLFile& html);
	void extendedinfo(CHTMLFile& html);
public:
	CBestSentryAward():CWeaponAward("Best Sentry Placement","sentrygun"){}
};



#endif // WEAPONAWARDS_H
