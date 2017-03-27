#include <windows.h>
#include "gl/gl.h"
#include "ShaderGL.h"

// texture 0 = normal map
// texture 1 = cube map
void InitNVForBumpMapping( void )
{
    glCombinerParameteriNV( GL_NUM_GENERAL_COMBINERS_NV, 1 );

	//
	// first general combiner
	//
	// A        .  B
	// normal map dot cube map
	// Texture0 dot Texture1  => spare0

	// rgb portion, input A: Texture0 (normal map)
	glCombinerInputNV( GL_COMBINER0_NV,			// stage
		               GL_RGB,					// portion
					   GL_VARIABLE_A_NV,		// variable
		               GL_TEXTURE0_ARB,			// input
					   GL_EXPAND_NORMAL_NV,		// mapping
					   GL_RGB );				// componentUsage
	// rgb portion, input B: Texture1 (cube map)
	glCombinerInputNV( GL_COMBINER0_NV,			// stage
		               GL_RGB,					// portion
					   GL_VARIABLE_B_NV,		// variable
		               GL_TEXTURE1_ARB,			// input
					   GL_EXPAND_NORMAL_NV,		// mapping
					   GL_RGB );				// componentUsage
	
	// rgb portion: output spare0
	glCombinerOutputNV( GL_COMBINER0_NV,		// stage
		                GL_RGB,					// portion
						GL_SPARE0_NV,			// abOutput
						GL_DISCARD_NV,			// cdOutput
						GL_DISCARD_NV,			// sumOutput
						GL_NONE,				// scale
						GL_NONE,				// bias
						GL_TRUE,				// abDotProduct
						GL_FALSE,				// cdDotProduct
						GL_FALSE );				// muxSum

	// Final Combiner
	//
	// E * F
	//
	// A * B + ( 1 - A ) C + D
	//
	// E <= Spare0	 ( H dot N )
	// F <= Spare0   ( H dot N )
	//
	// A <= PrimaryColor (attenuation factor)
	// B <= E * F
	//
	// C <= 0
	// D <= 0
	
	// E <= Spare0	 ( H dot N )
	glFinalCombinerInputNV( GL_VARIABLE_E_NV,			// variable
							GL_SPARE0_NV,				// input
							GL_UNSIGNED_IDENTITY_NV,	// mapping
							GL_RGB );					// componentUsage
	// F <= Spare0	 ( H dot N )
	glFinalCombinerInputNV( GL_VARIABLE_F_NV,			// variable
							GL_SPARE0_NV,				// input
							GL_UNSIGNED_IDENTITY_NV,	// mapping
							GL_RGB );					// componentUsage
	// A <= PrimaryColor (attenuation factor)
	glFinalCombinerInputNV( GL_VARIABLE_A_NV,			// variable
							GL_PRIMARY_COLOR_NV,		// input
							GL_UNSIGNED_IDENTITY_NV,	// mapping
							GL_RGB );					// componentUsage
	// B <= E * F
	glFinalCombinerInputNV( GL_VARIABLE_B_NV,			// variable
							GL_E_TIMES_F_NV,			// input
							GL_UNSIGNED_IDENTITY_NV,	// mapping
							GL_RGB );					// componentUsage
	// C <= 0
	glFinalCombinerInputNV( GL_VARIABLE_C_NV,			// variable
							GL_ZERO,					// input
							GL_UNSIGNED_IDENTITY_NV,	// mapping
							GL_RGB );					// componentUsage
	// D <= 0
	glFinalCombinerInputNV( GL_VARIABLE_D_NV,			// variable
							GL_ZERO,					// input
							GL_UNSIGNED_IDENTITY_NV,	// mapping
							GL_RGB );					// componentUsage

    glEnable( GL_REGISTER_COMBINERS_NV );
}


// texture0 = bump map
// constantColor0 = light vector
// texture1 = lightmap
void InitNVForBumpDotConstantVectorTimesLightmap( void )
{
	//
	// first general combiner
	//
	// A        .  B
	// normal map dot light vector
	// Texture0 dot PrimaryColor  => spare0

	// rgb portion, input A: Texture0 (normal map)
	glCombinerInputNV( GL_COMBINER0_NV,			// stage
		               GL_RGB,					// portion
					   GL_VARIABLE_A_NV,		// variable
		               GL_TEXTURE0_ARB,			// input
					   GL_EXPAND_NORMAL_NV,		// mapping
					   GL_RGB );				// componentUsage
	// rgb portion, input B: PrimaryColor0 (light vector)
	glCombinerInputNV( GL_COMBINER0_NV,			// stage
		               GL_RGB,					// portion
					   GL_VARIABLE_B_NV,		// variable
		               GL_PRIMARY_COLOR_NV,		// input
					   GL_EXPAND_NORMAL_NV,		// mapping
					   GL_RGB );				// componentUsage
	// rgb portion: output spare0
	glCombinerOutputNV( GL_COMBINER0_NV,		// stage
		                GL_RGB,					// portion
						GL_SPARE0_NV,			// abOutput
						GL_DISCARD_NV,			// cdOutput
						GL_DISCARD_NV,			// sumOutput
						GL_NONE,				// scale
						GL_NONE,				// bias
						GL_TRUE,				// abDotProduct
						GL_FALSE,				// cdDotProduct
						GL_FALSE );				// muxSum
	
	// Final Combiner
	//
	// E * F (not used)
	//
	// out = A * B + ( 1 - A ) C + D
	//
	// A <= 0
	// B <= 0
	// C <= 0
	// D <= Spare0	 ( normal map dot light vector )
	
	// A <= Texture1 ( lightmap )
	glFinalCombinerInputNV( GL_VARIABLE_A_NV,			// variable
							GL_TEXTURE1_ARB,			// input
							GL_UNSIGNED_IDENTITY_NV,	// mapping
							GL_ZERO );					// componentUsage
	// B <= Texture1 ( lightmap )
	glFinalCombinerInputNV( GL_VARIABLE_B_NV,			// variable
							GL_TEXTURE1_ARB,			// input
							GL_UNSIGNED_IDENTITY_NV,	// mapping
							GL_ZERO );					// componentUsage
	// C <= Texture1 ( lightmap )
	glFinalCombinerInputNV( GL_VARIABLE_C_NV,			// variable
							GL_TEXTURE1_ARB,			// input
							GL_UNSIGNED_IDENTITY_NV,	// mapping
							GL_ZERO );					// componentUsage
	// D <= Spare0	 ( normal map dot light vector )
	glFinalCombinerInputNV( GL_VARIABLE_D_NV,			// variable
							GL_SPARE0_NV,				// input
							GL_UNSIGNED_IDENTITY_NV,	// mapping
							GL_RGB );					// componentUsage
    glEnable( GL_REGISTER_COMBINERS_NV );
}

