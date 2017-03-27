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

#include <windows.h>	    

#include "transitiontable.h"
#include "recording.h"
#include "shaderapidx8.h"
#include "materialsystem/materialsystem_config.h"
#include "IShaderUtil.h"
#include "CMaterialSystemStats.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "vertexshaderdx8.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
CTransitionTable *g_pTransitionTable = NULL;

#ifdef DEBUG_BOARD_STATE

inline ShadowState_t& BoardState()
{
	return g_pTransitionTable->BoardState();
}

#endif


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CTransitionTable::CTransitionTable() : m_DefaultStateSnapshot(-1),
	m_CurrentShadowId(-1), m_CurrentSnapshotId(-1), m_TransitionOps( 0, 1024 ), m_ShadowStateList( 0, 32 ),
	m_TransitionTable( 0, 32 ), m_SnapshotList( 0, 32 )
{
	Assert( !g_pTransitionTable );
	g_pTransitionTable = this;
	m_bUsingStateBlocks = false;

#ifdef DEBUG_BOARD_STATE
	memset( &m_BoardState, 0, sizeof( m_BoardState ) );
	memset( &m_BoardShaderState, 0, sizeof( m_BoardShaderState ) );
#endif
}

CTransitionTable::~CTransitionTable()
{
	Assert( g_pTransitionTable == this );
	g_pTransitionTable = NULL;
}


//-----------------------------------------------------------------------------
// Initialization, shutdown
//-----------------------------------------------------------------------------
bool CTransitionTable::Init( bool bUseStateBlocks )
{
	m_bUsingStateBlocks = false; // bUseStateBlocks;
	return true;
}

void CTransitionTable::Shutdown( )
{
	Reset();
}


//-----------------------------------------------------------------------------
// Creates a shadow, adding an entry into the shadow list and transition table
//-----------------------------------------------------------------------------
StateSnapshot_t CTransitionTable::CreateStateSnapshot( )
{
	return m_SnapshotList.AddToTail();
}


//-----------------------------------------------------------------------------
// Creates a shadow, adding an entry into the shadow list and transition table
//-----------------------------------------------------------------------------
CTransitionTable::ShadowStateId_t CTransitionTable::CreateShadowState( )
{
	int newShaderState = m_ShadowStateList.AddToTail();

	// all existing states must transition to the new state
	int i;
	for ( i = 0; i < newShaderState; ++i )
	{
		// Add a new transition to all existing states
		int newElem = m_TransitionTable[i].AddToTail();
		m_TransitionTable[i][newElem].m_FirstOperation = INVALID_TRANSITION_OP;
		m_TransitionTable[i][newElem].m_NumOperations = 0;
		m_TransitionTable[i][newElem].m_nOpCountInStateBlock = 0;

		m_TransitionTable[i][newElem].m_pStateBlock = NULL;
	}

	// Add a new vector for this transition
	int newTransitionElem = m_TransitionTable.AddToTail();
	m_TransitionTable[newTransitionElem].EnsureCapacity( 32 );
	Assert( newShaderState == newTransitionElem );

	for ( i = 0; i <= newShaderState; ++i )
	{
		// Add a new transition from all existing states
		int newElem = m_TransitionTable[newShaderState].AddToTail();
		m_TransitionTable[newShaderState][newElem].m_FirstOperation = INVALID_TRANSITION_OP;
		m_TransitionTable[newShaderState][newElem].m_NumOperations = 0;
		m_TransitionTable[newShaderState][newElem].m_nOpCountInStateBlock = 0;
		m_TransitionTable[newShaderState][newElem].m_pStateBlock = NULL;
	}

	return newShaderState;
}


//-----------------------------------------------------------------------------
// Finds a snapshot, if it exists. Or creates a new one if it doesn't.
//-----------------------------------------------------------------------------
CTransitionTable::ShadowStateId_t CTransitionTable::FindShadowState( const ShadowState_t& currentState ) const
{
	for (int i = m_ShadowStateList.Count(); --i >= 0; )
	{
		if (!memcmp(&m_ShadowStateList[i], &currentState, sizeof(ShadowState_t) ))
			return i;
	}

	// Need to create a new one
	return (ShadowStateId_t)-1;
}


//-----------------------------------------------------------------------------
// Finds a snapshot, if it exists. Or creates a new one if it doesn't.
//-----------------------------------------------------------------------------
StateSnapshot_t CTransitionTable::FindStateSnapshot( ShadowStateId_t id, const ShadowShaderState_t& currentState ) const
{
	for (int i = m_SnapshotList.Count(); --i >= 0; )
	{
		if ( (id == m_SnapshotList[i].m_ShadowStateId) && 
			!memcmp(&m_SnapshotList[i].m_ShaderState, &currentState, sizeof(ShadowShaderState_t)) )
		{
			return i;
		}
	}

	// Need to create a new one
	return (StateSnapshot_t)-1;
}


//-----------------------------------------------------------------------------
// Used to clear out state blocks from the transition table
//-----------------------------------------------------------------------------
void CTransitionTable::ClearStateBlocks()
{
	// Release state blocks
	for ( int i = m_TransitionTable.Count(); --i >= 0;  )
	{
		for ( int j = m_TransitionTable[i].Count(); --j >= 0; )
		{
			if ( m_TransitionTable[i][j].m_pStateBlock )
			{
				m_TransitionTable[i][j].m_pStateBlock->Release();
				m_TransitionTable[i][j].m_pStateBlock = NULL;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Used to clear the transition table when we know it's become invalid.
//-----------------------------------------------------------------------------
void CTransitionTable::Reset()
{
	ClearStateBlocks();
	m_ShadowStateList.RemoveAll();
	m_SnapshotList.RemoveAll();
	m_TransitionTable.RemoveAll();
	m_TransitionOps.RemoveAll();
	m_CurrentShadowId = -1;
	m_CurrentSnapshotId = -1;
	m_DefaultStateSnapshot = -1;
}


//-----------------------------------------------------------------------------
// Sets the texture stage state
//-----------------------------------------------------------------------------
#pragma warning( disable : 4189 )

static inline void SetTextureStageState( int stage, D3DTEXTURESTAGESTATETYPE state, DWORD val )
{
	Assert( !ShaderAPI()->IsDeactivated() );

	RECORD_TEXTURE_STAGE_STATE( stage, state, val ); 

	HRESULT	hr = D3DDevice()->SetTextureStageState( stage, state, val );
	Assert( !FAILED(hr) );
}

static inline void SetRenderState( D3DRENDERSTATETYPE state, DWORD val )
{
	Assert( !ShaderAPI()->IsDeactivated() );

	RECORD_RENDER_STATE( state, val );
	
	HRESULT	hr = D3DDevice()->SetRenderState( state, val );
	Assert( !FAILED(hr) );
}

#pragma warning( default : 4189 )


//-----------------------------------------------------------------------------
// Methods that actually apply the state
//-----------------------------------------------------------------------------
#ifdef DEBUG_BOARD_STATE

static bool g_SpewTransitions = false;

#define APPLY_RENDER_STATE_FUNC( _d3dState, _state )					\
	void Apply ## _state( const ShadowState_t& shaderState, int arg ) 	\
	{																	\
		SetRenderState( _d3dState, shaderState.m_ ## _state );	\
		BoardState().m_ ## _state = shaderState.m_ ## _state;			\
		if (g_SpewTransitions)											\
		{																\
			char buf[128];												\
			sprintf( buf, "Apply %s : %d\n", #_d3dState, shaderState.m_ ## _state ); \
			OutputDebugString(buf);										\
		}																\
	}

#define APPLY_TEXTURE_STAGE_STATE_FUNC( _d3dState, _state )				\
	void Apply ## _state( const ShadowState_t& shaderState, int stage )	\
	{																	\
		SetTextureStageState( stage, _d3dState, shaderState.m_TextureStage[stage].m_ ## _state );	\
		BoardState().m_TextureStage[stage].m_ ## _state = shaderState.m_TextureStage[stage].m_ ## _state;	\
		if (g_SpewTransitions)											\
		{																\
			char buf[128];												\
			sprintf( buf, "Apply Tex %s (%d): %d\n", #_d3dState, stage, shaderState.m_TextureStage[stage].m_ ## _state ); \
			OutputDebugString(buf);										\
		}																\
	}																	\

#else

#define APPLY_RENDER_STATE_FUNC( _d3dState, _state )					\
	void Apply ## _state( const ShadowState_t& shaderState, int arg )	\
	{																	\
		SetRenderState( _d3dState, shaderState.m_ ## _state );	\
	}

#define APPLY_TEXTURE_STAGE_STATE_FUNC( _d3dState, _state )				\
	void Apply ## _state( const ShadowState_t& shaderState, int stage )	\
	{																	\
		SetTextureStageState( stage, _d3dState, shaderState.m_TextureStage[stage].m_ ## _state );	\
	}

#endif


APPLY_RENDER_STATE_FUNC( D3DRS_ZFUNC,				ZFunc )
APPLY_RENDER_STATE_FUNC( D3DRS_ZENABLE,				ZEnable )
APPLY_RENDER_STATE_FUNC( D3DRS_ZWRITEENABLE,		ZWriteEnable )
APPLY_RENDER_STATE_FUNC( D3DRS_COLORWRITEENABLE,	ColorWriteEnable )
APPLY_RENDER_STATE_FUNC( D3DRS_ALPHATESTENABLE,		AlphaTestEnable )
APPLY_RENDER_STATE_FUNC( D3DRS_ALPHAFUNC,			AlphaFunc )
APPLY_RENDER_STATE_FUNC( D3DRS_ALPHAREF,			AlphaRef )
APPLY_RENDER_STATE_FUNC( D3DRS_FILLMODE,			FillMode )
APPLY_RENDER_STATE_FUNC( D3DRS_LIGHTING,			Lighting )
APPLY_RENDER_STATE_FUNC( D3DRS_SPECULARENABLE,		SpecularEnable )
APPLY_RENDER_STATE_FUNC( D3DRS_SRGBWRITEENABLE,		SRGBWriteEnable )
APPLY_TEXTURE_STAGE_STATE_FUNC( D3DTSS_TEXCOORDINDEX, TexCoordIndex )

void ApplyAlphaBlend( const ShadowState_t& state, int stage )
{
	g_pTransitionTable->ApplyAlphaBlend( state.m_AlphaBlendEnable, state.m_SrcBlend, state.m_DestBlend );
}

void CTransitionTable::ApplyAlphaBlend( bool bEnable, D3DBLEND srcBlend, D3DBLEND destBlend )
{
	if (!bEnable)
	{
		if (m_CurrentState.m_AlphaBlendEnable)
		{
			SetRenderState( D3DRS_ALPHABLENDENABLE, bEnable );
			m_CurrentState.m_AlphaBlendEnable = bEnable;
		}
	}
	else
	{
		if (!m_CurrentState.m_AlphaBlendEnable)
		{
			SetRenderState( D3DRS_ALPHABLENDENABLE, bEnable );
			m_CurrentState.m_AlphaBlendEnable = bEnable;
		}

		// Set the blend state here...
		if (m_CurrentState.m_SrcBlend != srcBlend)
		{
			SetRenderState( D3DRS_SRCBLEND, srcBlend );
			m_CurrentState.m_SrcBlend = srcBlend;
		}

		if (m_CurrentState.m_DestBlend != destBlend)
		{
			SetRenderState( D3DRS_DESTBLEND, destBlend );
			m_CurrentState.m_DestBlend = destBlend;
		}
	}

#ifdef DEBUG_BOARD_STATE
	// This isn't quite true, but it's necessary for other error checking to work
	BoardState().m_AlphaBlendEnable = bEnable;
	BoardState().m_SrcBlend = srcBlend;
	BoardState().m_DestBlend = destBlend;
#endif
}

void ApplySeparateAlphaBlend( const ShadowState_t& state, int stage )
{
	g_pTransitionTable->ApplySeparateAlphaBlend( state.m_SeparateAlphaBlendEnable,
		state.m_SrcBlendAlpha, state.m_DestBlendAlpha );
}

void CTransitionTable::ApplySeparateAlphaBlend( bool bEnable, D3DBLEND srcBlendAlpha, D3DBLEND destBlendAlpha )
{
	if (!bEnable)
	{
		if (m_CurrentState.m_SeparateAlphaBlendEnable)
		{
			SetRenderState( D3DRS_SEPARATEALPHABLENDENABLE, bEnable );
			m_CurrentState.m_SeparateAlphaBlendEnable = bEnable;
		}
	}
	else
	{
		if (!m_CurrentState.m_SeparateAlphaBlendEnable)
		{
			SetRenderState( D3DRS_SEPARATEALPHABLENDENABLE, bEnable );
			m_CurrentState.m_SeparateAlphaBlendEnable = bEnable;
		}

		// Set the blend state here...
		if (m_CurrentState.m_SrcBlendAlpha != srcBlendAlpha)
		{
			SetRenderState( D3DRS_SRCBLENDALPHA, srcBlendAlpha );
			m_CurrentState.m_SrcBlendAlpha = srcBlendAlpha;
		}

		if (m_CurrentState.m_DestBlendAlpha != destBlendAlpha)
		{
			SetRenderState( D3DRS_DESTBLENDALPHA, destBlendAlpha );
			m_CurrentState.m_DestBlendAlpha = destBlendAlpha;
		}
	}

#ifdef DEBUG_BOARD_STATE
	// This isn't quite true, but it's necessary for other error checking to work
	BoardState().m_SeparateAlphaBlendEnable = bEnable;
	BoardState().m_SrcBlendAlpha = srcBlendAlpha;
	BoardState().m_DestBlendAlpha = destBlendAlpha;
#endif
}

//-----------------------------------------------------------------------------
// Applies alpha texture op
//-----------------------------------------------------------------------------
void ApplyColorTextureStage( const ShadowState_t& state, int stage )
{
	g_pTransitionTable->ApplyColorTextureStage( stage, state.m_TextureStage[stage].m_ColorOp,
		state.m_TextureStage[stage].m_ColorArg1, state.m_TextureStage[stage].m_ColorArg2 );
}

void ApplyAlphaTextureStage( const ShadowState_t& state, int stage )
{
	g_pTransitionTable->ApplyAlphaTextureStage( stage, state.m_TextureStage[stage].m_AlphaOp,
		state.m_TextureStage[stage].m_AlphaArg1, state.m_TextureStage[stage].m_AlphaArg2 );
}

void CTransitionTable::ApplyColorTextureStage( int stage, D3DTEXTUREOP op, int arg1, int arg2 )
{
	if (m_CurrentState.m_TextureStage[stage].m_ColorOp != op)
	{
		SetTextureStageState( stage, D3DTSS_COLOROP, op );
		m_CurrentState.m_TextureStage[stage].m_ColorOp = op;
	}

	if (op != D3DTOP_DISABLE)
	{
		if (m_CurrentState.m_TextureStage[stage].m_ColorArg1 != arg1)
		{
			SetTextureStageState( stage, D3DTSS_COLORARG1, arg1 );
			m_CurrentState.m_TextureStage[stage].m_ColorArg1 = arg1;
		}
		if (m_CurrentState.m_TextureStage[stage].m_ColorArg2 != arg2)
		{
			SetTextureStageState( stage, D3DTSS_COLORARG2, arg2 );
			m_CurrentState.m_TextureStage[stage].m_ColorArg2 = arg2;
		}
	}

#ifdef DEBUG_BOARD_STATE
	// This isn't quite true, but it's necessary for other error checking to work
	BoardState().m_TextureStage[stage].m_ColorOp = op;
	BoardState().m_TextureStage[stage].m_ColorArg1 = arg1;
	BoardState().m_TextureStage[stage].m_ColorArg2 = arg2;
#endif
}

void CTransitionTable::ApplyAlphaTextureStage( int stage, D3DTEXTUREOP op, int arg1, int arg2 )
{
	if (m_CurrentState.m_TextureStage[stage].m_AlphaOp != op)
	{
		SetTextureStageState( stage, D3DTSS_ALPHAOP, op );
		m_CurrentState.m_TextureStage[stage].m_AlphaOp = op;
	}

	if (op != D3DTOP_DISABLE)
	{
		if (m_CurrentState.m_TextureStage[stage].m_AlphaArg1 != arg1)
		{
			SetTextureStageState( stage, D3DTSS_ALPHAARG1, arg1 );
			m_CurrentState.m_TextureStage[stage].m_AlphaArg1 = arg1;
		}
		if (m_CurrentState.m_TextureStage[stage].m_AlphaArg2 != arg2)
		{
			SetTextureStageState( stage, D3DTSS_ALPHAARG2, arg2 );
			m_CurrentState.m_TextureStage[stage].m_AlphaArg2 = arg2;
		}
	}

#ifdef DEBUG_BOARD_STATE
	// This isn't quite true, but it's necessary for other error checking to work
	BoardState().m_TextureStage[stage].m_AlphaOp = op;
	BoardState().m_TextureStage[stage].m_AlphaArg1 = arg1;
	BoardState().m_TextureStage[stage].m_AlphaArg2 = arg2;
#endif
}


//-----------------------------------------------------------------------------
// NOTE: The following state setting methods are used only by state blocks
// instead of some of the transitions listed above
//-----------------------------------------------------------------------------
APPLY_RENDER_STATE_FUNC( D3DRS_ALPHABLENDENABLE,	AlphaBlendEnable )
APPLY_RENDER_STATE_FUNC( D3DRS_SRCBLEND,			SrcBlend )
APPLY_RENDER_STATE_FUNC( D3DRS_DESTBLEND,			DestBlend )

APPLY_TEXTURE_STAGE_STATE_FUNC( D3DTSS_COLOROP,		ColorOp )
APPLY_TEXTURE_STAGE_STATE_FUNC( D3DTSS_COLORARG1,	ColorArg1 )
APPLY_TEXTURE_STAGE_STATE_FUNC( D3DTSS_COLORARG2,	ColorArg2 )
APPLY_TEXTURE_STAGE_STATE_FUNC( D3DTSS_ALPHAOP,		AlphaOp )
APPLY_TEXTURE_STAGE_STATE_FUNC( D3DTSS_ALPHAARG1,	AlphaArg1 )
APPLY_TEXTURE_STAGE_STATE_FUNC( D3DTSS_ALPHAARG2,	AlphaArg2 )


//-----------------------------------------------------------------------------
// All transitions below this point depend on dynamic render state
// FIXME: Eliminate these virtual calls?
//-----------------------------------------------------------------------------
void ApplyZBias( const ShadowState_t& shadowState, int arg )
{
	ShaderAPI()->ApplyZBias( shadowState );

#ifdef DEBUG_BOARD_STATE
	BoardState().m_ZBias = shadowState.m_ZBias;
#endif
}

void ApplyTextureEnable( const ShadowState_t& state, int stage )
{														
	ShaderAPI()->ApplyTextureEnable( state, stage );

#ifdef DEBUG_BOARD_STATE
	BoardState().m_TextureStage[stage].m_TextureEnable = state.m_TextureStage[stage].m_TextureEnable;
#endif
}

void ApplyCullEnable( const ShadowState_t& state, int arg )
{
	ShaderAPI()->ApplyCullEnable( state.m_CullEnable );

#ifdef DEBUG_BOARD_STATE
	BoardState().m_CullEnable = state.m_CullEnable;
#endif
}

void ApplyVertexBlendEnable( const ShadowState_t& state, int stage )
{
	ShaderAPI()->SetVertexBlendState( state.m_VertexBlendEnable ? -1 : 0 );

#ifdef DEBUG_BOARD_STATE
	BoardState().m_VertexBlendEnable = state.m_VertexBlendEnable;
#endif
}

void ApplyVertexShaderOverbright( const ShadowState_t& state, int stage )
{
	ShaderAPI()->SetVertexShaderConstantC1( state.m_VertexShaderOverbright );

#ifdef DEBUG_BOARD_STATE
	BoardState().m_VertexShaderOverbright = state.m_VertexShaderOverbright;
#endif
}


//-----------------------------------------------------------------------------
// Creates an entry in the state transition table
//-----------------------------------------------------------------------------
inline void CTransitionTable::AddTransition( ApplyStateFunc_t func, int arg )
{
	int elem = m_TransitionOps.AddToTail();
	m_TransitionOps[elem].m_Op = func;
	m_TransitionOps[elem].m_Argument = arg;
}

#define ADD_RENDER_STATE_TRANSITION( _state )				\
	if (bForce || (toState.m_ ## _state != fromState.m_ ## _state))	\
	{														\
		AddTransition(::Apply ## _state, 0 );				\
		++numOps;											\
	}

#define ADD_TEXTURE_STAGE_STATE_TRANSITION( _stage, _state )\
	if (bForce || (toState.m_TextureStage[_stage].m_ ## _state != fromState.m_TextureStage[_stage].m_ ## _state))	\
	{														\
		Assert( _stage < NUM_TEXTURE_STAGES );				\
		AddTransition(::Apply ## _state, _stage );			\
		++numOps;											\
	}

int CTransitionTable::CreateStateBlockTransitions( const ShadowState_t& fromState, const ShadowState_t& toState, bool bForce )
{
	unsigned short numOps = 0;

	ADD_RENDER_STATE_TRANSITION( AlphaBlendEnable )
	ADD_RENDER_STATE_TRANSITION( SrcBlend )
	ADD_RENDER_STATE_TRANSITION( DestBlend )

	int nStageCount = HardwareConfig()->GetNumTextureStages();
	int i;
	for ( i = 0; i < nStageCount; ++i )
	{
		ADD_TEXTURE_STAGE_STATE_TRANSITION( i, TexCoordIndex );

		ADD_TEXTURE_STAGE_STATE_TRANSITION( i, ColorOp );
		ADD_TEXTURE_STAGE_STATE_TRANSITION( i, ColorArg1 );
		ADD_TEXTURE_STAGE_STATE_TRANSITION( i, ColorArg2 );

		ADD_TEXTURE_STAGE_STATE_TRANSITION( i, AlphaOp );
		ADD_TEXTURE_STAGE_STATE_TRANSITION( i, AlphaArg1 );
		ADD_TEXTURE_STAGE_STATE_TRANSITION( i, AlphaArg2 );
	}

	return numOps;
}

int CTransitionTable::CreateNormalTransitions( const ShadowState_t& fromState, const ShadowState_t& toState, bool bForce )
{
	int numOps = 0;

	// Special case for alpha blending to eliminate extra transitions
	bool blendEnableDifferent = (toState.m_AlphaBlendEnable != fromState.m_AlphaBlendEnable);
	bool srcBlendDifferent = toState.m_AlphaBlendEnable && (toState.m_SrcBlend != fromState.m_SrcBlend);
	bool destBlendDifferent = toState.m_AlphaBlendEnable && (toState.m_DestBlend != fromState.m_DestBlend);
	if (bForce || blendEnableDifferent || srcBlendDifferent || destBlendDifferent)
	{
		AddTransition( ::ApplyAlphaBlend, 0 );
		++numOps;
	}

	bool blendSeparateAlphaEnableDifferent = (toState.m_SeparateAlphaBlendEnable != fromState.m_SeparateAlphaBlendEnable);
	bool srcBlendAlphaDifferent = toState.m_SeparateAlphaBlendEnable && (toState.m_SrcBlendAlpha != fromState.m_SrcBlendAlpha);
	bool destBlendAlphaDifferent = toState.m_SeparateAlphaBlendEnable && (toState.m_DestBlendAlpha != fromState.m_DestBlendAlpha);
	if (bForce || blendSeparateAlphaEnableDifferent || srcBlendAlphaDifferent || destBlendAlphaDifferent)
	{
		AddTransition( ::ApplySeparateAlphaBlend, 0 );
		++numOps;
	}

	int nStageCount = HardwareConfig()->GetNumTextureStages();
	int i;
	for ( i = 0; i < nStageCount; ++i )
	{
		// Special case for texture stage ops to eliminate extra transitions
		const TextureStageShadowState_t& fromTexture = fromState.m_TextureStage[i];
		const TextureStageShadowState_t& toTexture = toState.m_TextureStage[i];

		bool fromEnabled = (fromTexture.m_ColorOp != D3DTOP_DISABLE);
		bool toEnabled = (toTexture.m_ColorOp != D3DTOP_DISABLE);
		if (fromEnabled || toEnabled || bForce)
		{
			bool opDifferent = (toTexture.m_ColorOp != fromTexture.m_ColorOp);
			bool arg1Different = (toTexture.m_ColorArg1 != fromTexture.m_ColorArg1);
			bool arg2Different = (toTexture.m_ColorArg2 != fromTexture.m_ColorArg2);
			if (opDifferent || arg1Different || arg2Different || bForce)
			{
				AddTransition( ::ApplyColorTextureStage, i );
				++numOps;
			}
		}

		fromEnabled = (fromTexture.m_AlphaOp != D3DTOP_DISABLE);
		toEnabled = (toTexture.m_AlphaOp != D3DTOP_DISABLE);
		if (fromEnabled || toEnabled || bForce)
		{
			bool opDifferent = (toTexture.m_AlphaOp != fromTexture.m_AlphaOp);
			bool arg1Different = (toTexture.m_AlphaArg1 != fromTexture.m_AlphaArg1);
			bool arg2Different = (toTexture.m_AlphaArg2 != fromTexture.m_AlphaArg2);
			if (opDifferent || arg1Different || arg2Different || bForce)
			{
				AddTransition( ::ApplyAlphaTextureStage, i );
				++numOps;
			}
		}

		ADD_TEXTURE_STAGE_STATE_TRANSITION( i, TexCoordIndex );
	}

	return numOps;
}

void CTransitionTable::CreateTransitionTableEntry( int to, int from )
{
	// If from < 0, that means add *all* transitions into it.

	unsigned short firstElem = m_TransitionOps.Count();
	unsigned short numOps = 0;

	const ShadowState_t& toState = m_ShadowStateList[to];
	const ShadowState_t& fromState = (from >= 0) ? m_ShadowStateList[from] : m_ShadowStateList[to];
	bool bForce = (from < 0);

	ADD_RENDER_STATE_TRANSITION( ZFunc )
	ADD_RENDER_STATE_TRANSITION( ZWriteEnable )
	ADD_RENDER_STATE_TRANSITION( ZEnable )
	ADD_RENDER_STATE_TRANSITION( ColorWriteEnable )
	ADD_RENDER_STATE_TRANSITION( AlphaTestEnable )
	ADD_RENDER_STATE_TRANSITION( AlphaFunc )
	ADD_RENDER_STATE_TRANSITION( AlphaRef )
	ADD_RENDER_STATE_TRANSITION( FillMode )
	ADD_RENDER_STATE_TRANSITION( Lighting )
	ADD_RENDER_STATE_TRANSITION( SpecularEnable )
	ADD_RENDER_STATE_TRANSITION( SRGBWriteEnable )

	// Some code for the non-trivial transitions
	if (m_bUsingStateBlocks)
	{
		numOps += CreateStateBlockTransitions( fromState, toState, bForce );
	}
	else
	{
		numOps += CreateNormalTransitions( fromState, toState, bForce );
	}

	int nOpsBeforeDynamicState = numOps;

	// NOTE: From here on down are transitions that depend on dynamic state
	// and which can therefore not appear in the state block
	ADD_RENDER_STATE_TRANSITION( ZBias )
	ADD_RENDER_STATE_TRANSITION( CullEnable )
	ADD_RENDER_STATE_TRANSITION( VertexBlendEnable )
	ADD_RENDER_STATE_TRANSITION( VertexShaderOverbright )

	int nStageCount = HardwareConfig()->GetNumTextureStages();
	for ( int i = 0; i < nStageCount; ++i )
	{
		ADD_TEXTURE_STAGE_STATE_TRANSITION( i, TextureEnable );
	}

	// Look for identical transition lists, and use those instead...
	TransitionList_t& transition = (from >= 0) ? 
							m_TransitionTable[to][from] : m_DefaultTransition;
	Assert( numOps <= 255 );
	transition.m_NumOperations = numOps;
	transition.m_nOpCountInStateBlock = nOpsBeforeDynamicState;
	transition.m_pStateBlock = NULL;

	// This condition can happen, and is valid. It occurs when we snapshot
	// state but do not generate a transition function for that state
	if (numOps == 0)
	{
		transition.m_FirstOperation = INVALID_TRANSITION_OP;
		return;
	}

	unsigned short identicalListFirstElem = FindIdenticalTransitionList( firstElem, numOps ); 
	if (identicalListFirstElem == INVALID_TRANSITION_OP)
	{
		transition.m_FirstOperation = firstElem;
		Assert( (int)firstElem + (int)numOps < 65535 );

		if( (int)firstElem + (int)numOps >= 65535 )
		{
			Warning("**** WARNING: Transition table overflow. Grab Brian\n");
		}
	}
	else
	{
		// Remove the transitions ops we made; use the duplicate copy
		transition.m_FirstOperation = identicalListFirstElem;
		m_TransitionOps.RemoveMultiple( firstElem, numOps );
	}

	if ( m_bUsingStateBlocks && transition.m_nOpCountInStateBlock != 0 )
	{
		// Find a transition list that matches *exactly* to share its state block 
		IDirect3DStateBlock9 *pStateBlock = (from >= 0) ? FindIdenticalStateBlock( &transition, to ) : NULL;
		if (pStateBlock)
		{
			transition.m_pStateBlock = pStateBlock;
			transition.m_pStateBlock->AddRef();
		}
		else
		{
			transition.m_pStateBlock = CreateStateBlock( transition, to );
		}
	}
}


//-----------------------------------------------------------------------------
// Tests a snapshot to see if it can be used
//-----------------------------------------------------------------------------

#define PERFORM_RENDER_STATE_TRANSITION( _state, _func )	\
	::Apply ## _func( _state, 0 );
#define PERFORM_TEXTURE_STAGE_STATE_TRANSITION( _state, _stage, _func )	\
	::Apply ## _func( _state, _stage );

bool CTransitionTable::TestShadowState( const ShadowState_t& state, const ShadowShaderState_t &shaderState )
{
	PERFORM_RENDER_STATE_TRANSITION( state, ZFunc )
	PERFORM_RENDER_STATE_TRANSITION( state, ZWriteEnable )
	PERFORM_RENDER_STATE_TRANSITION( state, ZEnable )
	PERFORM_RENDER_STATE_TRANSITION( state, ZBias )
	PERFORM_RENDER_STATE_TRANSITION( state, ColorWriteEnable )
	PERFORM_RENDER_STATE_TRANSITION( state, AlphaBlend )
	PERFORM_RENDER_STATE_TRANSITION( state, AlphaTestEnable )
	PERFORM_RENDER_STATE_TRANSITION( state, AlphaFunc )
	PERFORM_RENDER_STATE_TRANSITION( state, AlphaRef )
	PERFORM_RENDER_STATE_TRANSITION( state, FillMode )
	PERFORM_RENDER_STATE_TRANSITION( state, CullEnable )
	PERFORM_RENDER_STATE_TRANSITION( state, Lighting )
	PERFORM_RENDER_STATE_TRANSITION( state, SpecularEnable )
	PERFORM_RENDER_STATE_TRANSITION( state, SRGBWriteEnable )
	PERFORM_RENDER_STATE_TRANSITION( state, VertexBlendEnable )
	PERFORM_RENDER_STATE_TRANSITION( state, VertexShaderOverbright )

	int i;
	int nStageCount = HardwareConfig()->GetNumTextureStages();
	for ( i = 0; i < nStageCount; ++i )
	{
		PERFORM_TEXTURE_STAGE_STATE_TRANSITION( state, i, ColorTextureStage );
		PERFORM_TEXTURE_STAGE_STATE_TRANSITION( state, i, AlphaTextureStage );
		PERFORM_TEXTURE_STAGE_STATE_TRANSITION( state, i, TexCoordIndex );
		PERFORM_TEXTURE_STAGE_STATE_TRANSITION( state, i, TextureEnable );
	}

	// Just make sure we've got a good snapshot
	RECORD_COMMAND( DX8_VALIDATE_DEVICE, 0 );

	DWORD numPasses;
	HRESULT hr = D3DDevice()->ValidateDevice( &numPasses );
	bool ok = !FAILED(hr);

	// Now set the board state to match the default state
	ApplyTransition( m_DefaultTransition, m_DefaultStateSnapshot );

	ShaderManager()->SetVertexShader( shaderState.m_VertexShader, shaderState.m_nStaticVshIndex );
	ShaderManager()->SetPixelShader( shaderState.m_PixelShader, shaderState.m_nStaticPshIndex );

	return ok;
}


//-----------------------------------------------------------------------------
// Finds identical transition lists and shares them 
//-----------------------------------------------------------------------------
unsigned short CTransitionTable::FindIdenticalTransitionList( unsigned short firstElem, 
														   unsigned short numOps ) const
{
	// Look for a common list
	const TransitionOp_t &op = m_TransitionOps[firstElem];
	int maxTest = firstElem - numOps;
	for (int i = 0; i < maxTest;  )
	{
		// Fast out...
		if ( (m_TransitionOps[i].m_Op != op.m_Op) ||
			 (m_TransitionOps[i].m_Argument != op.m_Argument) )
		{
			++i;
			continue;
		}

		unsigned short potentialMatch = (unsigned short )i;
		int matchCount = 1;
		++i;

		while( matchCount < numOps )
		{
			if ( (m_TransitionOps[i].m_Op != m_TransitionOps[firstElem+matchCount].m_Op) ||
				 (m_TransitionOps[i].m_Argument != m_TransitionOps[firstElem+matchCount].m_Argument) )
			{
				break;
			}

			++i;
			++matchCount;
		}

		if (matchCount == numOps)
			return potentialMatch;
	}

	return INVALID_TRANSITION_OP;
}


//-----------------------------------------------------------------------------
// Create startup snapshot
//-----------------------------------------------------------------------------
void CTransitionTable::TakeDefaultStateSnapshot( )
{
	if (m_DefaultStateSnapshot == -1)
	{
		m_DefaultStateSnapshot = TakeSnapshot();

		// This will create a transition which sets *all* shadowed state
		CreateTransitionTableEntry( m_DefaultStateSnapshot, -1 );
	}
}


//-----------------------------------------------------------------------------
// Applies the transition list
//-----------------------------------------------------------------------------
void CTransitionTable::ApplyTransitionList( int snapshot, int nFirstOp, int nOpCount )
{
	// Don't bother if there's nothing to do
	if (nOpCount > 0)
	{
		// Trying to avoid function overhead here
		ShadowState_t& shadowState = m_ShadowStateList[snapshot];
		TransitionOp_t* pTransitionOp = &m_TransitionOps[nFirstOp];

		for (int i = 0; i < nOpCount; ++i )
		{
			// invoke the transition method
			(*pTransitionOp->m_Op)(shadowState, pTransitionOp->m_Argument);
			++pTransitionOp;
			MaterialSystemStats()->IncrementCountedStat( MATERIAL_SYSTEM_STATS_SHADOW_STATE, 1 );
		}
	}
}


//-----------------------------------------------------------------------------
// Apply startup snapshot
//-----------------------------------------------------------------------------
#pragma warning( disable : 4189 )

void CTransitionTable::ApplyTransition( TransitionList_t& list, int snapshot )
{
	if ( ShaderAPI()->IsDeactivated() )
		return;

	// Transition lists when using state blocks have 2 parts: the first
	// is the stateblock part, which is states that are not related to
	// dynamic state at all; followed by states that *are* affected by dynamic state
	int nFirstOp = list.m_FirstOperation;
	int nOpCount = list.m_NumOperations;

	if ( m_bUsingStateBlocks )
	{
		if ( list.m_pStateBlock )
		{
			HRESULT hr = list.m_pStateBlock->Apply();
			Assert( !FAILED(hr) );
		}

		nFirstOp += list.m_nOpCountInStateBlock;
		nOpCount -= list.m_nOpCountInStateBlock;
	}

	ApplyTransitionList( snapshot, nFirstOp, nOpCount );

	// Semi-hacky code to override what the transitions are doing
	PerformShadowStateOverrides();

	// Set the current snapshot id
	m_CurrentShadowId = snapshot;

#ifdef DEBUG_BOARD_STATE
	// Copy over the board states that aren't explicitly in the transition table
	// so the assertion works...
	int nStageCount = HardwareConfig()->GetNumTextureStages();
	for (int i = 0; i < nStageCount; ++i)
	{
		m_BoardState.m_TextureStage[i].m_GenerateSphericalCoords = 
			CurrentShadowState().m_TextureStage[i].m_GenerateSphericalCoords;
	}
	m_BoardState.m_AmbientCubeOnStage0 = CurrentShadowState().m_AmbientCubeOnStage0;
	m_BoardState.m_ModulateConstantColor = CurrentShadowState().m_ModulateConstantColor;
	m_BoardState.m_VertexUsage = CurrentShadowState().m_VertexUsage;
	m_BoardState.m_UsingFixedFunction = CurrentShadowState().m_UsingFixedFunction;

	// State blocks bypass the code that sets the board state
	if ( !m_bUsingStateBlocks )
	{
		Assert( !memcmp( &m_BoardState, &CurrentShadowState(), sizeof(m_BoardState) ) );
	}
#endif
}

#pragma warning( default : 4189 )


//-----------------------------------------------------------------------------
// Takes a snapshot, hooks it into the material
//-----------------------------------------------------------------------------
StateSnapshot_t CTransitionTable::TakeSnapshot( )
{
	// Do any final computation of the shadow state
	ShaderShadow()->ComputeAggregateShadowState();

	// Get the current snapshot
	const ShadowState_t& currentState = ShaderShadow()->GetShadowState();

	// Create a new snapshot
	ShadowStateId_t shadowStateId = FindShadowState( currentState );
	if (shadowStateId == -1)
	{
		// Create entry in state transition table
		shadowStateId = CreateShadowState();

		// Copy our snapshot into the list
		memcpy( &m_ShadowStateList[shadowStateId], &currentState, sizeof(ShadowState_t) );

		// Now create new transition entries
		for (int to = 0; to < shadowStateId; ++to)
		{
			CreateTransitionTableEntry( to, shadowStateId );
		}

		for (int from = 0; from < shadowStateId; ++from)
		{
			CreateTransitionTableEntry( shadowStateId, from );
		}
	}

	const ShadowShaderState_t& currentShaderState = ShaderShadow()->GetShadowShaderState();
 	StateSnapshot_t snapshotId = FindStateSnapshot( shadowStateId, currentShaderState );
	if (snapshotId == -1)
	{
		// Create entry in state transition table
		snapshotId = CreateStateSnapshot();

		// Copy our snapshot into the list
		m_SnapshotList[snapshotId].m_ShadowStateId = shadowStateId;
		memcpy( &m_SnapshotList[snapshotId].m_ShaderState, &currentShaderState, sizeof(ShadowShaderState_t) );
	}

	return snapshotId;
}


//-----------------------------------------------------------------------------
// Apply shader state (stuff that doesn't lie in the transition table)
//-----------------------------------------------------------------------------
void CTransitionTable::ApplyShaderState( const ShadowState_t &shadowState, const ShadowShaderState_t &shaderState )
{
	// Don't bother testing against the current state because there
	// could well be dynamic state modifiers affecting this too....
	if (!shadowState.m_UsingFixedFunction)
	{
		// FIXME: Improve early-binding of vertex shader index
		ShaderManager()->SetVertexShader( shaderState.m_VertexShader, shaderState.m_nStaticVshIndex );
		ShaderManager()->SetPixelShader( shaderState.m_PixelShader, shaderState.m_nStaticPshIndex );

#ifdef DEBUG_BOARD_STATE
		BoardShaderState().m_VertexShader = shaderState.m_VertexShader;
		BoardShaderState().m_PixelShader = shaderState.m_PixelShader;
		BoardShaderState().m_nStaticVshIndex = shaderState.m_nStaticVshIndex;
		BoardShaderState().m_nStaticPshIndex = shaderState.m_nStaticPshIndex;
#endif
	}
	else
	{
		ShaderManager()->SetVertexShader( INVALID_VERTEX_SHADER );
		ShaderManager()->SetPixelShader( INVALID_PIXEL_SHADER );

#ifdef DEBUG_BOARD_STATE
		BoardShaderState().m_VertexShader = INVALID_VERTEX_SHADER;
		BoardShaderState().m_PixelShader = INVALID_PIXEL_SHADER;
		BoardShaderState().m_nStaticVshIndex = 0;
		BoardShaderState().m_nStaticPshIndex = 0;
#endif
	}
}


//-----------------------------------------------------------------------------
// Makes the board state match the snapshot
//-----------------------------------------------------------------------------
void CTransitionTable::UseSnapshot( StateSnapshot_t snapshotId )
{
	ShadowStateId_t id = m_SnapshotList[snapshotId].m_ShadowStateId;
	if (m_CurrentSnapshotId != snapshotId)
	{
		// First apply things that are in the transition table
		if ( m_CurrentShadowId != id )
		{
			TransitionList_t& transition = m_TransitionTable[id][m_CurrentShadowId];
			ApplyTransition( transition, id );
		}

		// NOTE: There is an opportunity here to set non-dynamic state that we don't
		// store in the transition list if we ever need it.

		m_CurrentSnapshotId = snapshotId;
	}

	// NOTE: This occurs regardless of whether the snapshot changed because it depends
	// on dynamic state (namely, the dynamic vertex + pixel shader index)
	// Followed by things that are not
	ApplyShaderState( m_ShadowStateList[id], m_SnapshotList[snapshotId].m_ShaderState );

#ifdef _DEBUG
	// NOTE: We can't ship with this active because mod makers may well violate this rule
	// We don't want no stinking fixed-function on hardware that has vertex and pixel shaders. . 
	// This could cause a serious perf hit. 
	if( HardwareConfig()->SupportsVertexAndPixelShaders() )
	{
//		Assert( !CurrentShadowState().m_UsingFixedFunction );
	}
#endif
}


//-----------------------------------------------------------------------------
// Cause the board to match the default state snapshot
//-----------------------------------------------------------------------------
void CTransitionTable::UseDefaultState( )
{
	// Need to blat these out because they are tested during transitions
	m_CurrentState.m_AlphaBlendEnable = false;
	m_CurrentState.m_SrcBlend = D3DBLEND_ONE;
	m_CurrentState.m_DestBlend = D3DBLEND_ZERO;
	SetRenderState( D3DRS_ALPHABLENDENABLE, m_CurrentState.m_AlphaBlendEnable );
	SetRenderState( D3DRS_SRCBLEND, m_CurrentState.m_SrcBlend );
	SetRenderState( D3DRS_DESTBLEND, m_CurrentState.m_DestBlend );

	// GR
	m_CurrentState.m_SeparateAlphaBlendEnable = false;
	m_CurrentState.m_SrcBlendAlpha = D3DBLEND_ONE;
	m_CurrentState.m_DestBlendAlpha = D3DBLEND_ZERO;
	SetRenderState( D3DRS_SEPARATEALPHABLENDENABLE, m_CurrentState.m_SeparateAlphaBlendEnable );
	SetRenderState( D3DRS_SRCBLENDALPHA, m_CurrentState.m_SrcBlendAlpha );
	SetRenderState( D3DRS_DESTBLENDALPHA, m_CurrentState.m_DestBlendAlpha );

	int nTextureUnits = ShaderAPI()->GetActualNumTextureUnits();
	for ( int i = 0; i < nTextureUnits; ++i)
	{
		TextureStage(i).m_ColorOp = D3DTOP_DISABLE;
		TextureStage(i).m_ColorArg1 = D3DTA_TEXTURE;
		TextureStage(i).m_ColorArg2 = (i == 0) ? D3DTA_DIFFUSE : D3DTA_CURRENT;
		TextureStage(i).m_AlphaOp = D3DTOP_DISABLE;
		TextureStage(i).m_AlphaArg1 = D3DTA_TEXTURE;
		TextureStage(i).m_AlphaArg2 = (i == 0) ? D3DTA_DIFFUSE : D3DTA_CURRENT;

		SetTextureStageState( i, D3DTSS_COLOROP,	TextureStage(i).m_ColorOp );
		SetTextureStageState( i, D3DTSS_COLORARG1,	TextureStage(i).m_ColorArg1 );
		SetTextureStageState( i, D3DTSS_COLORARG2,	TextureStage(i).m_ColorArg2 );
		SetTextureStageState( i, D3DTSS_ALPHAOP,	TextureStage(i).m_AlphaOp );
		SetTextureStageState( i, D3DTSS_ALPHAARG1,	TextureStage(i).m_AlphaArg1 );
		SetTextureStageState( i, D3DTSS_ALPHAARG2,	TextureStage(i).m_AlphaArg2 );
	}

	// Disable z overrides...
	m_CurrentState.m_bOverrideDepthEnable = false;
	m_CurrentState.m_ForceDepthFuncEquals = false;

	ApplyTransition( m_DefaultTransition, m_DefaultStateSnapshot );

	ShaderManager()->SetVertexShader( INVALID_VERTEX_SHADER );
	ShaderManager()->SetPixelShader( INVALID_PIXEL_SHADER );
}


//-----------------------------------------------------------------------------
// Snapshotted state overrides
//-----------------------------------------------------------------------------
void CTransitionTable::ForceDepthFuncEquals( bool bEnable )
{
	if( bEnable != m_CurrentState.m_ForceDepthFuncEquals )
	{
		// Do this so that we can call this from within the rendering code
		// See OverrideDepthEnable + PerformShadowStateOverrides for a version
		// that isn't expected to be called from within rendering code
		if( !ShaderAPI()->IsRenderingMesh() )
		{
			ShaderAPI()->FlushBufferedPrimitives();
		}

		m_CurrentState.m_ForceDepthFuncEquals = bEnable;

		if( bEnable )
		{
			SetRenderState( D3DRS_ZFUNC, D3DCMP_EQUAL );
		}
		else
		{
			SetRenderState( D3DRS_ZFUNC, CurrentShadowState().m_ZFunc );
		}
	}
}

void CTransitionTable::OverrideDepthEnable( bool bEnable, bool bDepthEnable )
{
	if ( bEnable != m_CurrentState.m_bOverrideDepthEnable )
	{
		ShaderAPI()->FlushBufferedPrimitives();
		m_CurrentState.m_bOverrideDepthEnable = bEnable;
		m_CurrentState.m_OverrideZWriteEnable = bDepthEnable ? D3DZB_TRUE : D3DZB_FALSE;

		if ( m_CurrentState.m_bOverrideDepthEnable )
		{
			SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
			SetRenderState( D3DRS_ZWRITEENABLE, m_CurrentState.m_OverrideZWriteEnable );
		}
		else
		{
			SetRenderState( D3DRS_ZENABLE, CurrentShadowState().m_ZEnable );
			SetRenderState( D3DRS_ZWRITEENABLE, CurrentShadowState().m_ZFunc );
		}
	}
}


//-----------------------------------------------------------------------------
// Perform state block overrides
//-----------------------------------------------------------------------------
void CTransitionTable::PerformShadowStateOverrides( )
{
	// Deal with funky overrides here, because the state blocks can't...
	if ( m_CurrentState.m_ForceDepthFuncEquals )
	{
		SetRenderState( D3DRS_ZFUNC, D3DCMP_EQUAL );
	}

	if ( m_CurrentState.m_bOverrideDepthEnable )
	{
		SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
		SetRenderState( D3DRS_ZWRITEENABLE, m_CurrentState.m_OverrideZWriteEnable );
	}
}


//-----------------------------------------------------------------------------
// Finds identical transition lists and shares them 
//-----------------------------------------------------------------------------
#define TEST_RENDER_STATEBLOCK_VALUE( _state )	\
	if( pOp->m_Op == ::Apply ## _state )					\
	{														\
		if ( state1.m_ ## _state == state2.m_ ## _state )	\
			continue;										\
		return false;										\
	}

#define TEST_TEXTURE_STATEBLOCK_VALUE( _state, _stage )		\
	if( pOp->m_Op == ::Apply ## _state )					\
	{														\
		if ( state1.m_TextureStage[_stage].m_ ## _state == state2.m_TextureStage[_stage].m_ ## _state )	\
			continue;										\
		return false;										\
	}

bool CTransitionTable::DoStateBlocksMatch( const TransitionList_t *pTransition, int nToSnapshot1, int nToSnapshot2 ) const
{
	// We know they are the same if the dest snapshot is identical
	if ( nToSnapshot1 == nToSnapshot2 )
		return true;

	// FIXME: This is gross!!
	// Otherwise we gotta check the values associated with each state-setting method
	const TransitionOp_t *pOp = &m_TransitionOps[pTransition->m_FirstOperation];
	const ShadowState_t &state1 = m_ShadowStateList[nToSnapshot1];
	const ShadowState_t &state2 = m_ShadowStateList[nToSnapshot2];

	for ( int k = 0; k < pTransition->m_nOpCountInStateBlock; ++k, ++pOp )
	{
		TEST_RENDER_STATEBLOCK_VALUE( ZFunc );
		TEST_RENDER_STATEBLOCK_VALUE( ZEnable );
		TEST_RENDER_STATEBLOCK_VALUE( ZWriteEnable );
		TEST_RENDER_STATEBLOCK_VALUE( ColorWriteEnable );
		TEST_RENDER_STATEBLOCK_VALUE( AlphaTestEnable );
		TEST_RENDER_STATEBLOCK_VALUE( AlphaFunc );
		TEST_RENDER_STATEBLOCK_VALUE( AlphaRef );
		TEST_RENDER_STATEBLOCK_VALUE( FillMode );
		TEST_RENDER_STATEBLOCK_VALUE( Lighting );
		TEST_RENDER_STATEBLOCK_VALUE( SpecularEnable );
		TEST_RENDER_STATEBLOCK_VALUE( SRGBWriteEnable );
		TEST_RENDER_STATEBLOCK_VALUE( AlphaBlendEnable );
		TEST_RENDER_STATEBLOCK_VALUE( SrcBlend );
		TEST_RENDER_STATEBLOCK_VALUE( DestBlend );

		TEST_TEXTURE_STATEBLOCK_VALUE( TexCoordIndex, pOp->m_Argument );
		TEST_TEXTURE_STATEBLOCK_VALUE( ColorOp, pOp->m_Argument );
		TEST_TEXTURE_STATEBLOCK_VALUE( ColorArg1, pOp->m_Argument );
		TEST_TEXTURE_STATEBLOCK_VALUE( ColorArg2, pOp->m_Argument );
		TEST_TEXTURE_STATEBLOCK_VALUE( AlphaOp, pOp->m_Argument );
		TEST_TEXTURE_STATEBLOCK_VALUE( AlphaArg1, pOp->m_Argument );
		TEST_TEXTURE_STATEBLOCK_VALUE( AlphaArg2, pOp->m_Argument );

		// Should never get here!
		Assert( 0 );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Finds identical transition lists and shares them 
//-----------------------------------------------------------------------------
IDirect3DStateBlock9 *CTransitionTable::FindIdenticalStateBlock( TransitionList_t *pTransition, int toSnapshot ) const
{
	Assert( pTransition );

	// FIXME: Could we bake this into the identical transition list test?
	for ( int to = m_TransitionTable.Count(); --to >= 0; )
	{
		for ( int from = m_TransitionTable[to].Count(); --from >= 0; )
		{
			const TransitionList_t *pTest = &m_TransitionTable[to][from];

			// Ignore the transition we're currently working on
			if ( pTransition == pTest )
				continue;

			// Not only do the ops need to match, but so do the values the ops use in the snapshots
			if ( pTest->m_nOpCountInStateBlock != pTransition->m_nOpCountInStateBlock )
				continue;

			bool bFoundOpMatch = true;
			if ( pTest->m_FirstOperation != pTransition->m_FirstOperation )
			{
				const TransitionOp_t *pOp1 = &m_TransitionOps[pTest->m_FirstOperation];
				const TransitionOp_t *pOp2 = &m_TransitionOps[pTransition->m_FirstOperation];

				for ( int k = 0; k < pTransition->m_nOpCountInStateBlock; ++k )
				{
					if ( (pOp1->m_Op != pOp2->m_Op) || (pOp1->m_Argument != pOp2->m_Argument) )
					{
						bFoundOpMatch = false;
						break;
					}
					++pOp1, ++pOp2;
				}
			}

			if ( !bFoundOpMatch )
				continue;

			// Now we gotta check that the set values are the same.
			if ( !DoStateBlocksMatch( pTest, to, toSnapshot ) )
				continue;

			return pTest->m_pStateBlock;
		}
	}

	return NULL;
}

IDirect3DStateBlock9 *CTransitionTable::CreateStateBlock( TransitionList_t& list, int snapshot )
{
	Assert( list.m_NumOperations > 0 );
	
	HRESULT hr = D3DDevice()->BeginStateBlock( );
	Assert( !FAILED(hr) );

	// Necessary to set the bools so that the state blocks always have the info in them
	ApplyTransitionList( snapshot, list.m_FirstOperation, list.m_nOpCountInStateBlock );

	IDirect3DStateBlock9 *pStateBlock;
	hr = D3DDevice()->EndStateBlock( &pStateBlock );
	Assert( !FAILED(hr) );

	return pStateBlock;
}


