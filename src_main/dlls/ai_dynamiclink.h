//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		A link that can be turned on and off.  Unlike normal links
//				dyanimc links must be entities so they and receive messages.
//				They update the state of the actual links.  Allows us to save
//				a lot of memory by not making all links into entities
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_DYNAMICLINK_H
#define AI_DYNAMICLINK_H
#pragma once

enum DynamicLinkState_t
{
	LINK_OFF = 0,
	LINK_ON = 1,
};

//=============================================================================
//	>> CAI_DynanicLink
//=============================================================================
class CAI_DynamicLink : public CPointEntity
{
	DECLARE_CLASS( CAI_DynamicLink, CPointEntity );
public:

	static void					InitDynamicLinks(void);
	static void					ResetDynamicLinks(void);
	static void					PurgeDynamicLinks(void);
	static void 				GenerateControllerLinks();

	static CAI_DynamicLink*		GetDynamicLink(int nSrcID, int nDstID);

	static CAI_DynamicLink*		m_pAllDynamicLinks;		// A linked list of all dynamic link
	CAI_DynamicLink*			m_pNextDynamicLink;		// The next dynamic link in the list of dynamic links

	int							m_nSrcID;				// the node that 'owns' this link
	int							m_nDestID;				// the node on the other end of the link. 
	DynamicLinkState_t			m_nLinkState;			// 
	string_t					m_strAllowUse;

	bool						m_bFixedUpIds;

	void						SetLinkState( void );
	bool						IsLinkValid( void );

	// ----------------
	//	Inputs
	// ----------------
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );

	DECLARE_DATADESC();

	CAI_DynamicLink();
	~CAI_DynamicLink();
};

//=============================================================================
//	>> CAI_DynanicLinkVolume
//=============================================================================
class CAI_DynamicLinkController : public CPointEntity
{
	DECLARE_CLASS( CAI_DynamicLinkController, CPointEntity );
public:

	void GenerateLinksFromVolume();

	// ----------------
	//	Inputs
	// ----------------
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	
	CUtlVector< CHandle<CAI_DynamicLink> > m_ControlledLinks;
	DynamicLinkState_t			m_nLinkState;			// 
	string_t					m_strAllowUse;

	DECLARE_DATADESC();
};

#endif // AI_DYNAMICLINK_H