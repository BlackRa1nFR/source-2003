//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// The dx8 implementation of the transition table
//=============================================================================

#ifndef TRANSITION_TABLE_H
#define TRANSITION_TABLE_H

#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "shadershadowdx8.h"

// Required for DEBUG_BOARD_STATE
#include "shaderapidx8_global.h"

struct IDirect3DStateBlock9;


//-----------------------------------------------------------------------------
// Types related to transition table entries
//-----------------------------------------------------------------------------
typedef void (*ApplyStateFunc_t)( const ShadowState_t& shadowState, int arg );


//-----------------------------------------------------------------------------
// The DX8 implementation of the transition table
//-----------------------------------------------------------------------------
class CTransitionTable
{
public:
	// constructor, destructor
	CTransitionTable( );
	virtual ~CTransitionTable();

	// Initialization, shutdown
	bool Init( bool bUseStateBlocks );
	void Shutdown( );

	// Resets the snapshots...
	void Reset();

	// Takes a snapshot
	StateSnapshot_t TakeSnapshot( );

	// Take startup snapshot
	void TakeDefaultStateSnapshot( );

	// Makes the board state match the snapshot
	void UseSnapshot( StateSnapshot_t snapshotId );

	// Cause the board to match the default state snapshot
	void UseDefaultState();

	// Snapshotted state overrides
	void ForceDepthFuncEquals( bool bEnable );
	void OverrideDepthEnable( bool bEnable, bool bDepthEnable );

	// Returns a particular snapshot
	const ShadowState_t &GetSnapshot( StateSnapshot_t snapshotId ) const;
	const ShadowShaderState_t &GetSnapshotShader( StateSnapshot_t snapshotId ) const;

	// Gets the current shadow state
	const ShadowState_t &CurrentShadowState() const;

	// Return the current shapshot
	int CurrentSnapshot() const { return m_CurrentSnapshotId; }

#ifdef DEBUG_BOARD_STATE
	ShadowState_t& BoardState() { return m_BoardState; }
	ShadowShaderState_t& BoardShaderState() { return m_BoardShaderState; }
#endif

	// The following are meant to be used by the transition table only
public:
	// Applies alpha blending
	void ApplyAlphaBlend( bool bEnable, D3DBLEND srcBlend, D3DBLEND destBlend );
	// GR - separate alpha blend
	void ApplySeparateAlphaBlend( bool bEnable, D3DBLEND srcBlendAlpha, D3DBLEND destBlendAlpha );

	// Applies alpha texture op
	void ApplyColorTextureStage( int stage, D3DTEXTUREOP op, int arg1, int arg2 );
	void ApplyAlphaTextureStage( int stage, D3DTEXTUREOP op, int arg1, int arg2 );

private:
	enum
	{
		INVALID_TRANSITION_OP = 0xFFFF
	};

	typedef short ShadowStateId_t;

	// For the transition table
	struct TransitionList_t
	{
		unsigned short m_FirstOperation;
		unsigned char m_NumOperations;
		unsigned char m_nOpCountInStateBlock;
		IDirect3DStateBlock9 *m_pStateBlock;
	};

	struct TransitionOp_t
	{
		ApplyStateFunc_t m_Op;
		int m_Argument;
	};

	struct SnapshotShaderState_t
	{
		ShadowShaderState_t m_ShaderState;
		ShadowStateId_t m_ShadowStateId;
	};

	struct CurrentTextureStageState_t
	{
		D3DTEXTUREOP			m_ColorOp;
		int						m_ColorArg1;
		int						m_ColorArg2;
		D3DTEXTUREOP			m_AlphaOp;
		int						m_AlphaArg1;
		int						m_AlphaArg2;
	};

	struct CurrentState_t
	{
		// Alpha state
		bool			m_AlphaBlendEnable;
		D3DBLEND		m_SrcBlend;
		D3DBLEND		m_DestBlend;

		// GR - Separate alpha state
		bool			m_SeparateAlphaBlendEnable;
		D3DBLEND		m_SrcBlendAlpha;
		D3DBLEND		m_DestBlendAlpha;

		bool			m_ForceDepthFuncEquals;
		bool			m_bOverrideDepthEnable;
		D3DZBUFFERTYPE	m_OverrideZWriteEnable;

		// Texture stage state
		CurrentTextureStageState_t m_TextureStage[NUM_TEXTURE_STAGES];
	};

	CurrentTextureStageState_t &TextureStage( int stage ) { return m_CurrentState.m_TextureStage[stage]; }
	const CurrentTextureStageState_t &TextureStage( int stage ) const { return m_CurrentState.m_TextureStage[stage]; }

	// creates state snapshots
	ShadowStateId_t  CreateShadowState();
	StateSnapshot_t  CreateStateSnapshot();

	// finds state snapshots
	ShadowStateId_t FindShadowState( const ShadowState_t& currentState ) const;
	StateSnapshot_t FindStateSnapshot( ShadowStateId_t id, const ShadowShaderState_t& currentState ) const;

	// Finds identical transition lists
	unsigned short FindIdenticalTransitionList( unsigned short firstElem, 
		unsigned short numOps ) const;

	// Adds a transition
	void AddTransition( ApplyStateFunc_t func, int arg );

	// Apply a transition
	void ApplyTransition( TransitionList_t& list, int snapshot );

	// Creates an entry in the transition table
	void CreateTransitionTableEntry( int to, int from );

	// Checks if a state is valid
	bool TestShadowState( const ShadowState_t& state, const ShadowShaderState_t &shaderState );

	// Perform state block overrides
	void PerformShadowStateOverrides( );

	// Used to clear out state blocks from the transition table
	void ClearStateBlocks();

	// Applies the transition list
	void ApplyTransitionList( int snapshot, int nFirstOp, int nOpCount );

	// Apply shader state (stuff that doesn't lie in the transition table)
	void ApplyShaderState( const ShadowState_t &shadowState, const ShadowShaderState_t &shaderState );

	// Wrapper for the non-standard transitions for stateblock + non-stateblock cases
	int CreateStateBlockTransitions( const ShadowState_t& fromState, const ShadowState_t& toState, bool bForce );
	int CreateNormalTransitions( const ShadowState_t& fromState, const ShadowState_t& toState, bool bForce );

	// Check to see if state blocks match
	bool DoStateBlocksMatch( const TransitionList_t *pTransition, int nToSnapshot1, int nToSnapshot2 ) const;

	// Finds identical state blocks
	IDirect3DStateBlock9 *FindIdenticalStateBlock( TransitionList_t *pTransition, int toSnapshot ) const;

	// Creates a state block
	IDirect3DStateBlock9 *CreateStateBlock( TransitionList_t& list, int snapshot );

private:
	// State blocks
	bool	m_bUsingStateBlocks;

	// Sets up the default state
	StateSnapshot_t m_DefaultStateSnapshot;
	TransitionList_t m_DefaultTransition;
	
	// The current snapshot id
	ShadowStateId_t m_CurrentShadowId;
	StateSnapshot_t m_CurrentSnapshotId;

	// Maintains a list of all used snapshot transition states
	CUtlVector< ShadowState_t >	m_ShadowStateList;

	// The snapshot transition table
	CUtlVector< CUtlVector< TransitionList_t > >	m_TransitionTable;

	// Stores all state transition operations
	CUtlVector< TransitionOp_t > m_TransitionOps;

	// Stores all state for a particular snapshot
	CUtlVector< SnapshotShaderState_t >	m_SnapshotList;

	// The current board state.
	CurrentState_t m_CurrentState;

#ifdef DEBUG_BOARD_STATE
	// Maintains the total shadow state
	ShadowState_t m_BoardState;
	ShadowShaderState_t m_BoardShaderState;
#endif
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline const ShadowState_t &CTransitionTable::GetSnapshot( StateSnapshot_t snapshotId ) const
{
	Assert( (snapshotId >= 0) && (snapshotId < m_SnapshotList.Count()) );
	return m_ShadowStateList[m_SnapshotList[snapshotId].m_ShadowStateId];
}

inline const ShadowShaderState_t &CTransitionTable::GetSnapshotShader( StateSnapshot_t snapshotId ) const
{
	Assert( (snapshotId >= 0) && (snapshotId < m_SnapshotList.Count()) );
	return m_SnapshotList[snapshotId].m_ShaderState;
}


inline const ShadowState_t & CTransitionTable::CurrentShadowState() const
{
	Assert( (m_CurrentShadowId >= 0) && (m_CurrentShadowId < m_ShadowStateList.Count()) );
	return m_ShadowStateList[m_CurrentShadowId];
}


#endif // TRANSITION_TABLE_H