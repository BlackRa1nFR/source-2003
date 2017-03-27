//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "shaderlib/CShader.h"

BEGIN_SHADER( Wireframe, 
			  "Help for Wireframe" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( POLYOFFSET, SHADER_PARAM_TYPE_BOOL, "0", "Enable/disable poly offset" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		SET_FLAGS( MATERIAL_VAR_NOFOG );
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->PolyMode( SHADER_POLYMODEFACE_FRONT_AND_BACK, SHADER_POLYMODE_LINE );

			SetModulationShadowState();
			SetDefaultBlendingShadowState();

			int flags = SHADER_DRAW_POSITION;
			if ( IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR) || IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA) )
				flags |= SHADER_DRAW_COLOR;
			
			pShaderShadow->DrawFlags( flags );
			
			if( params[POLYOFFSET]->IsDefined() && params[POLYOFFSET]->GetIntValue() )
				pShaderShadow->EnablePolyOffset( true );
			else
				pShaderShadow->EnablePolyOffset( false );
		}
		DYNAMIC_STATE
		{
			SetModulationDynamicState();
//			DefaultFog();
		}
		Draw();
	}
END_SHADER
