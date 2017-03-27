//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "filesystem.h"
#include "sentence.h"
#include "hud_closecaption.h"
#include "engine/ivmodelinfo.h"

IMPLEMENT_CLIENTCLASS_DT(C_BaseFlex, DT_BaseFlex, CBaseFlex)
	RecvPropArray(
		RecvPropFloat(RECVINFO(m_flexWeight[0])),
		m_flexWeight),

	RecvPropInt(RECVINFO(m_blinktoggle)),
	RecvPropVector(RECVINFO(m_viewtarget)),

#ifdef HL2_CLIENT_DLL
	RecvPropFloat( RECVINFO(m_vecViewOffset[0]) ),
	RecvPropFloat( RECVINFO(m_vecViewOffset[1]) ),
	RecvPropFloat( RECVINFO(m_vecViewOffset[2]) ),
#endif

END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_BaseFlex )

/*
	// DEFINE_FIELD( C_BaseFlex, m_viewtarget, FIELD_VECTOR ),
	// DEFINE_ARRAY( C_BaseFlex, m_flexWeight, FIELD_FLOAT, 64 ),
	// DEFINE_FIELD( C_BaseFlex, m_blinktoggle, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_blinktime, FIELD_FLOAT ),
	// DEFINE_FIELD( C_BaseFlex, m_prevviewtarget, FIELD_VECTOR ),
	// DEFINE_ARRAY( C_BaseFlex, m_prevflexWeight, FIELD_FLOAT, 64 ),
	// DEFINE_FIELD( C_BaseFlex, m_prevblinktoggle, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_iBlink, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_iEyeUpdown, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_iEyeRightleft, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_iEyeAttachment, FIELD_INTEGER ),
	// DEFINE_FIELD( C_BaseFlex, m_FileList, CUtlVector < CFlexSceneFile * > ),
*/

END_PREDICTION_DATA()

C_BaseFlex::C_BaseFlex()
{
#ifdef _DEBUG
	((Vector&)m_viewtarget).Init();
#endif

	AddVar( &m_viewtarget, &m_iv_viewtarget, LATCH_ANIMATION_VAR );
	AddVar( m_flexWeight, &m_iv_flexWeight, LATCH_ANIMATION_VAR );

	m_iBlink = -1;  // init to invalid setting
	m_iEyeUpdown = -1;
	m_iEyeRightleft = -1;
	m_iEyeAttachment = -1;

	// Fill in phoneme class lookup
	memset( m_PhonemeClasses, 0, sizeof( m_PhonemeClasses ) );

	Emphasized_Phoneme *weak = &m_PhonemeClasses[ PHONEME_CLASS_WEAK ];
	strcpy( weak->classname, "phonemes_weak" );
	weak->required = false;
	Emphasized_Phoneme *normal = &m_PhonemeClasses[ PHONEME_CLASS_NORMAL ];
	strcpy( normal->classname, "phonemes" );
	normal->required = true;
	Emphasized_Phoneme *strong = &m_PhonemeClasses[ PHONEME_CLASS_STRONG ];
	strcpy( strong->classname, "phonemes_strong" );
	strong->required = false;

	/// Make sure size is correct
	Assert( PHONEME_CLASS_STRONG + 1 == NUM_PHONEME_CLASSES );
}

C_BaseFlex::~C_BaseFlex()
{
	DeleteSceneFiles();
}

#define BLINKDURATION 0.3

ConVar g_CV_PhonemeDelay("phonemedelay", "0", FCVAR_ARCHIVE );
ConVar g_CV_PhonemeFilter("phonemefilter", "0.08", FCVAR_ARCHIVE );
ConVar g_CV_FlexRules("flex_rules", "1", FCVAR_ARCHIVE );

void C_BaseFlex::RunFlexRules( float *dest )
{
	int i, j;

	if (!g_CV_FlexRules.GetInt())
		return;
	
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	// FIXME: this shouldn't be needed, flex without rules should be stripped in studiomdl
	for (i = 0; i < hdr->numflexdesc; i++)
	{
		dest[i] = 0;
	}

	for (i = 0; i < hdr->numflexrules; i++)
	{
		float stack[32];
		int k = 0;
		mstudioflexrule_t *prule = hdr->pFlexRule( i );

		mstudioflexop_t *pops = prule->iFlexOp( 0 );

		// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), i + 1, 0, "%2d:%d\n", i, prule->flex );

		for (j = 0; j < prule->numops; j++)
		{
			switch (pops->op)
			{
			case STUDIO_ADD: stack[k-2] = stack[k-2] + stack[k-1]; k--; break;
			case STUDIO_SUB: stack[k-2] = stack[k-2] - stack[k-1]; k--; break;
			case STUDIO_MUL: stack[k-2] = stack[k-2] * stack[k-1]; k--; break;
			case STUDIO_DIV:
				if (stack[k-1] > 0.0001)
				{
					stack[k-2] = stack[k-2] / stack[k-1];
				}
				else
				{
					stack[k-2] = 0;
				}
				k--; 
				break;
			case STUDIO_CONST: stack[k] = pops->d.value; k++; break;
			case STUDIO_FETCH1: 
				{ 
				int m = hdr->pFlexcontroller(pops->d.index)->link;
				stack[k] = g_flexweight[m];
				k++; 
				break;
				}
			case STUDIO_FETCH2: stack[k] = dest[pops->d.index]; k++; break;
			}

			pops++;
		}
		dest[prule->flex] = stack[0];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void *C_BaseFlex::FindSceneFile( const char *filename )
{
	// See if it's already loaded
	int i;
	for ( i = 0; i < m_FileList.Size(); i++ )
	{
		CFlexSceneFile *file = m_FileList[ i ];
		if ( file && !stricmp( file->filename, filename ) )
		{
			return file->buffer;
		}
	}
	
	// Load file into memory
	FileHandle_t file = filesystem->Open( VarArgs( "expressions/%s.vfe", filename ), "rb" );

	if ( !file )
		return NULL;

	int len = filesystem->Size( file );

	// read the file
	byte *buffer = new unsigned char[ len ];
	Assert( buffer );
	filesystem->Read( buffer, len, file );
	filesystem->Close( file );

	// Create scene entry
	CFlexSceneFile *pfile = new CFlexSceneFile();
	// Remember filename
	strcpy( pfile->filename, filename );
	// Remember data pointer
	pfile->buffer = buffer;
	// Add to list
	m_FileList.AddToTail( pfile );

	// Fill in translation table
	flexsettinghdr_t *pSettinghdr = ( flexsettinghdr_t * )pfile->buffer;
	Assert( pSettinghdr );
	for (i = 0; i < pSettinghdr->numkeys; i++)
	{
		*(pSettinghdr->pLocalToGlobal(i)) = AddGlobalFlexController( pSettinghdr->pLocalName( i ) );
	}

	// Return data
	return pfile->buffer;
}

//-----------------------------------------------------------------------------
// Purpose: Free up any loaded files
//-----------------------------------------------------------------------------
void C_BaseFlex::DeleteSceneFiles( void )
{
	while ( m_FileList.Size() > 0 )
	{
		CFlexSceneFile *file = m_FileList[ 0 ];
		m_FileList.Remove( 0 );
		delete[] file->buffer;
		delete file;
	}
}

static void Extract_FileBase( const char *in, char *out )
{
	int len, start, end;

	len = strlen( in );
	
	// scan backward for '.'
	end = len - 1;
	while ( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;
	
	if ( in[end] != '.' )		// no '.', copy to end
		end = len-1;
	else 
		end--;					// Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len-1;
	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if ( start < 0 || ( in[start] != '/' && in[start] != '\\' ) )
		start = 0;
	else 
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	Q_strncpy( out, &in[start], len );
}

//-----------------------------------------------------------------------------
// Purpose: make sure the eyes are within 30 degrees of forward
//-----------------------------------------------------------------------------
void C_BaseFlex::SetViewTarget( void )
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	// aim the eyes
	Vector tmp = m_viewtarget;

	if (m_iEyeUpdown == -1)
		m_iEyeUpdown = AddGlobalFlexController( "eyes_updown" );

	if (m_iEyeRightleft == -1)
		m_iEyeRightleft = AddGlobalFlexController( "eyes_rightleft" );

	if (m_iEyeAttachment == -1)
	{
		m_iEyeAttachment = -2;
		for (int i = 0; i < hdr->numattachments; i++)
		{
			if (stricmp( hdr->pAttachment( i )->pszName(), "eyes" ) == 0)
			{
				m_iEyeAttachment = i;
				break;
			}
		}
	}

	if (m_iEyeAttachment >= 0)
	{
		Vector local;

		mstudioattachment_t *patt = hdr->pAttachment( m_iEyeAttachment );

		matrix3x4_t attToWorld;

		ConcatTransforms( *modelrender->pBoneToWorld( patt->bone ), patt->local, attToWorld ); 
		VectorITransform( tmp, attToWorld, local );
		
		float flDist = local.Length();

		VectorNormalize( local );

		// calculate animated eye deflection
		Vector eyeDeflect;
		QAngle eyeAng( g_flexweight[m_iEyeUpdown], g_flexweight[m_iEyeRightleft], 0 );

		// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%.3f %.3f", eyeAng.x, eyeAng.y );

		AngleVectors( eyeAng, &eyeDeflect );
		eyeDeflect.x = 0;

		// reduce deflection the more the eye is off center
		// FIXME: this angles make no damn sense
		eyeDeflect = eyeDeflect * (local.x * local.x);
		local = local + eyeDeflect;
		VectorNormalize( local );

		// check to see if the eye is aiming outside a 30 degree cone
		if (local.x < 0.866) // cos(30)
		{
			// if so, clamp it to 30 degrees offset
			// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%.2f %.2f %.2f", local.x, local.y, local.z );
			local.x = 0;
			float d = local.LengthSqr();
			if (d > 0.0)
			{
				d = sqrtf( (1.0 - 0.866 * 0.866) / (local.y*local.y + local.z*local.z) );
				local.x = 0.866;
				local.y = local.y * d;
				local.z = local.z * d;
			}
			else
			{
				local.x = 1.0;
			}
		}
		local = local * flDist;
		VectorTransform( local, attToWorld, tmp );
	}

	modelrender->SetViewTarget( tmp );

	/*
	debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%.2f %.2f %.2f  : %.2f %.2f %.2f", 
		m_viewtarget.x, m_viewtarget.y, m_viewtarget.z, 
		m_prevviewtarget.x, m_prevviewtarget.y, m_prevviewtarget.z );
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void NewMarkovIndex( flexsettinghdr_t *pSettinghdr, flexsetting_t *pSetting )
{
	if ( pSetting->type != FS_MARKOV )
		return;

	int weighttotal = 0;
	int member = 0;
	for (int i = 0; i < pSetting->numsettings; i++)
	{
		flexmarkovgroup_t *group = pSetting->pMarkovGroup( i );
		if ( !group )
			continue;

		weighttotal += group->weight;
		if ( !weighttotal || random->RandomInt(0,weighttotal-1) < group->weight )
		{
			member = i;
		}
	}

	pSetting->currentindex = member;
}

#define STRONG_CROSSFADE_START		0.60f
#define WEAK_CROSSFADE_START		0.40f

//-----------------------------------------------------------------------------
// Purpose: 
// Here's the formula
// 0.5 is neutral 100 % of the default setting
// Crossfade starts at STRONG_CROSSFADE_START and is full at STRONG_CROSSFADE_END
// If there isn't a strong then the intensity of the underlying phoneme is fixed at 2 x STRONG_CROSSFADE_START
//  so we don't get huge numbers
// Input  : *classes - 
//			emphasis_intensity - 
//-----------------------------------------------------------------------------
void C_BaseFlex::ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity )
{
	// See which overrides are available for the current phoneme
	bool has_weak	= classes[ PHONEME_CLASS_WEAK ].valid;
	bool has_strong = classes[ PHONEME_CLASS_STRONG ].valid;

	// Better have phonemes in general
	Assert( classes[ PHONEME_CLASS_NORMAL ].valid );

	if ( emphasis_intensity > STRONG_CROSSFADE_START )
	{
		if ( has_strong )
		{
			// Blend in some of strong
			float dist_remaining = 1.0f - emphasis_intensity;
			float frac = dist_remaining / ( 1.0f - STRONG_CROSSFADE_START );

			classes[ PHONEME_CLASS_NORMAL ].amount = (frac) * 2.0f * STRONG_CROSSFADE_START;
			classes[ PHONEME_CLASS_STRONG ].amount = 1.0f - frac; 
		}
		else
		{
			emphasis_intensity = min( emphasis_intensity, STRONG_CROSSFADE_START );
			classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
		}
	}
	else if ( emphasis_intensity < WEAK_CROSSFADE_START )
	{
		if ( has_weak )
		{
			// Blend in some weak
			float dist_remaining = WEAK_CROSSFADE_START - emphasis_intensity;
			float frac = dist_remaining / ( WEAK_CROSSFADE_START );

			classes[ PHONEME_CLASS_NORMAL ].amount = (1.0f - frac) * 2.0f * WEAK_CROSSFADE_START;
			classes[ PHONEME_CLASS_WEAK ].amount = frac; 
		}
		else
		{
			emphasis_intensity = max( emphasis_intensity, WEAK_CROSSFADE_START );
			classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
		}
	}
	else
	{
		// Assume 0.5 (neutral) becomes a scaling of 1.0f
		classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//			phoneme - 
//			scale - 
//			newexpression - 
//-----------------------------------------------------------------------------
void C_BaseFlex::AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression )
{
	int type;

	// Setup weights for any emphasis blends
	bool skip = SetupEmphasisBlend( classes, phoneme );
	// Uh-oh, missing or unknown phoneme???
	if ( skip )
	{
		return;
	}
		
	// Compute blend weights
	ComputeBlendedSetting( classes, emphasis_intensity );

	for ( type = 0; type < NUM_PHONEME_CLASSES; type++ )
	{
		Emphasized_Phoneme *info = &classes[ type ];
		if ( !info->valid || info->amount == 0.0f )
			continue;

		// Assume that we're not using overrieds
		flexsettinghdr_t *actual_flexsetting_header = info->base;

		const flexsetting_t *pSetting = actual_flexsetting_header->pIndexedSetting( phoneme );
		if (!pSetting)
		{
			continue;
		}

		if ( newexpression )
		{
			if ( pSetting->type == FS_MARKOV )
			{
				NewMarkovIndex( actual_flexsetting_header, ( flexsetting_t * )pSetting );
			}
		}

		// Determine its index
		int i = pSetting - actual_flexsetting_header->pSetting( 0 );
		Assert( i >= 0 );
		Assert( i < actual_flexsetting_header->numflexsettings );

		// Resolve markov chain for the returned setting, probably not an issue for visemes
		pSetting = actual_flexsetting_header->pTranslatedSetting( i );

		// Check for overrides
		if ( info->override )
		{
			// Get name from setting
			const char *resolvedName = pSetting->pszName();
			if ( resolvedName )
			{
				// See if resolvedName exists in the override file
				const flexsetting_t *override = FindNamedSetting( info->override, resolvedName );
				if ( override )
				{
					// If so, point at the override file instead
					actual_flexsetting_header	= info->override;
					pSetting					= override;
				}
			}
		}

		flexweight_t *pWeights = NULL;

		int truecount = pSetting->psetting( (byte *)actual_flexsetting_header, 0, &pWeights );
		if ( pWeights )
		{
			for (i = 0; i < truecount; i++)
			{
				// Translate to global controller number
				int j = actual_flexsetting_header->LocalToGlobal( pWeights->key );
				// Add scaled weighting in
				g_flexweight[j] += info->amount * scale * pWeights->weight;
				// Go to next setting
				pWeights++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A lot of the one time setup and also resets amount to 0.0f default
//  for strong/weak/normal tracks
// Returning true == skip this phoneme
// Input  : *classes - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseFlex::SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme )
{
	int i;

	bool skip = false;

	for ( i = 0; i < NUM_PHONEME_CLASSES; i++ )
	{
		Emphasized_Phoneme *info = &classes[ i ];

		// Assume it's bogus
		info->valid = false;
		info->amount = 0.0f;

		// One time setup
		if ( !info->basechecked )
		{
			info->basechecked = true;
			info->base = (flexsettinghdr_t *)FindSceneFile( info->classname );
		}
		info->override = NULL;

		info->exp = NULL;
		if ( info->base )
		{
			Assert( info->base->id == ('V' << 16) + ('F' << 8) + ('E') );
			info->exp = info->base->pIndexedSetting( phoneme );
		}

		if ( info->required && ( !info->base || !info->exp ) )
		{
			skip = true;
			break;
		}

		if ( info->exp )
		{
			info->valid = true;
		}

		// Find overrides, if any exist
		// Also a one-time setup
		if ( !info->overridechecked )
		{
			char overridefile[ 512 ];
			char shortname[ 128 ];
			char modelname[ 128 ];

			strcpy( modelname, modelinfo->GetModelName( GetModel() ) );

			// Fix up the name
			Extract_FileBase( modelname, shortname );

			Q_snprintf( overridefile, sizeof( overridefile ), "%s/%s", shortname, info->classname );

			info->overridechecked = true;
			info->override = ( flexsettinghdr_t * )FindSceneFile( overridefile );
		}
	}
	
	return skip;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//			*sentence - 
//			t - 
//			dt - 
//			juststarted - 
//-----------------------------------------------------------------------------
void C_BaseFlex::AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted )
{
	for ( int w = 0; w < sentence->m_Words.Size(); w++ )
	{
		CWordTag *word = sentence->m_Words[ w ];
		if ( !word )
			continue;

		for (int k = 0; k < word->m_Phonemes.Size(); k++)
		{
			CPhonemeTag *phoneme = word->m_Phonemes[ k ];

			if (t > phoneme->m_flStartTime && t < phoneme->m_flEndTime)
			{
				if (k < word->m_Phonemes.Size()-1)
				{
					dt = max( dt, min( word->m_Phonemes[ k+1 ]->m_flEndTime - t, phoneme->m_flEndTime - phoneme->m_flStartTime ) );
				}
			}

			float t1 = ( phoneme->m_flStartTime - t) / dt;
			float t2 = ( phoneme->m_flEndTime - t) / dt;

			if (t1 < 1.0 && t2 > 0)
			{
				float scale;

				// clamp
				if (t2 > 1)
					t2 = 1;
				if (t1 < 0)
					t1 = 0;

				// FIXME: simple box filter.  Should use something fancier
				scale = (t2 - t1);

				AddViseme( classes, emphasis_intensity, phoneme->m_nPhonemeCode, scale, juststarted );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : curtime - 
//			sentence - 
//-----------------------------------------------------------------------------
void C_BaseFlex::ProcessCloseCaptionData( float curtime, CSentence* sentence )
{
	CHudCloseCaption *closecaption = GET_HUDELEMENT( CHudCloseCaption );
	if ( closecaption )
	{
		closecaption->Process( this, curtime, sentence );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//-----------------------------------------------------------------------------
void C_BaseFlex::ProcessVisemes( Emphasized_Phoneme *classes )
{
	// Any sounds being played?
	if ( !MouthInfo().IsActive() )
		return;

	// Multiple phoneme tracks can overlap, look across all such tracks.
	for ( int source = 0 ; source < MouthInfo().GetNumVoiceSources(); source++ )
	{
		CVoiceData *vd = MouthInfo().GetVoiceSource( source );
		if ( !vd )
			continue;

		CSentence *sentence = engine->GetSentence( vd->m_pAudioSource );
		if ( !sentence )
			continue;

		float	sentence_length = engine->GetSentenceLength( vd->m_pAudioSource );
		// Give it a bit of extra time to clear out...
		float   sound_end_time = vd->m_flVoiceStartTime + sentence_length + 5.0f;
		
		// This sound should be done...why hasn't it been removed yet???
		if ( gpGlobals->curtime >= sound_end_time )
			continue;

		float timesincestart = gpGlobals->curtime - vd->m_flVoiceStartTime;
		// Adjust actual time
		float t = timesincestart - g_CV_PhonemeDelay.GetFloat();
		// Get box filter duration
		float dt = g_CV_PhonemeFilter.GetFloat();

		// Assume sound has been playing for a while...
		bool juststarted = false;
		/*
		// FIXME:  Do we really want to support markov chains for the phonemes?
		// If so, we'll need to uncomment out these lines.
		if ( timesincestart < 0.001 )
		{
			juststarted = true;
		}
		*/

		// Get intensity setting for this time (from spline)
		float emphasis_intensity = sentence->GetIntensity( t, sentence_length );

		// Blend and add visemes together
		AddVisemesForSentence( classes, emphasis_intensity, sentence, t, dt, juststarted );

		// Push any close captioning info to CC system
		ProcessCloseCaptionData( t, sentence );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseFlex::SetupWeights( )
{
	studiohdr_t *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	memset( g_flexweight, 0, sizeof( g_flexweight ) );

	// FIXME: this should assert then, it's too complex a class for the model
	if (hdr->numflexcontrollers == 0)
		return;

	int i, j;

	// FIXME: shouldn't this happen at runtime?
	// initialize the models local to global flex controller mappings
	if (hdr->pFlexcontroller( 0 )->link == -1)
	{
		for (i = 0; i < hdr->numflexcontrollers; i++)
		{
			j = AddGlobalFlexController( hdr->pFlexcontroller( i )->pszName() );
			hdr->pFlexcontroller( i )->link = j;
		}
	}

	// blend weights from server
	for (i = 0; i < hdr->numflexcontrollers; i++)
	{
		mstudioflexcontroller_t *pflex = hdr->pFlexcontroller( i );

		g_flexweight[pflex->link] = m_flexWeight[i];
		// rescale
		g_flexweight[pflex->link] = g_flexweight[pflex->link] * (pflex->max - pflex->min) + pflex->min;
	}

	// check for blinking
	if (m_blinktoggle != m_prevblinktoggle)
	{
		m_prevblinktoggle = m_blinktoggle;
		m_blinktime = gpGlobals->curtime + BLINKDURATION;
	}

	if (m_iBlink == -1)
		m_iBlink = AddGlobalFlexController( "blink" );
	g_flexweight[m_iBlink] = 0;

	// blink the eyes
	float t = (m_blinktime - gpGlobals->curtime) * M_PI * 0.5 * (1.0/BLINKDURATION);
	if (t > 0)
	{
		// do eyeblink falloff curve
		t = cos(t);
		if (t > 0)
		{
			g_flexweight[m_iBlink] = sqrtf( t ) * 2;
			if (g_flexweight[m_iBlink] > 1)
				g_flexweight[m_iBlink] = 2.0 - g_flexweight[m_iBlink];
		}
	}

	// Drive the mouth from .wav file playback...
	ProcessVisemes( m_PhonemeClasses );

	float destweight[128];
	RunFlexRules( destweight );

	// aim the eyes
	SetViewTarget( );

	/*
	for (i = 0; i < hdr->numflexdesc; i++)
	{
		debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), i-hdr->numflexcontrollers, 0, "%2d:%s %3.2f : %3.2f", i, hdr->pFlexdesc( i )->pszFACS(), destweight[i] );
	}
	*/

	/*
	for (i = 0; i < g_numflexcontrollers; i++)
	{
		int j = hdr->pFlexcontroller( i )->link;
		debugoverlay->AddTextOverlay( origin + Vector( 0, 0, 64 ), -i, 0, "%s %3.2f", g_flexcontroller[i], g_flexweight[j] );
	}
	*/

	modelrender->SetFlexWeights( destweight, hdr->numflexdesc );
}



int C_BaseFlex::g_numflexcontrollers;
char * C_BaseFlex::g_flexcontroller[MAXSTUDIOFLEXCTRL*4];
float C_BaseFlex::g_flexweight[MAXSTUDIOFLEXDESC];

int C_BaseFlex::AddGlobalFlexController( char *szName )
{
	int i;
	for (i = 0; i < g_numflexcontrollers; i++)
	{
		if (stricmp( g_flexcontroller[i], szName ) == 0)
		{
			return i;
		}
	}

	if (g_numflexcontrollers < MAXSTUDIOFLEXCTRL * 4)
	{
		g_flexcontroller[g_numflexcontrollers++] = strdup( szName );
	}
	else
	{
		// FIXME: missing runtime error condition
	}
	return i;
}

const flexsetting_t *C_BaseFlex::FindNamedSetting( const flexsettinghdr_t *pSettinghdr, const char *expr )
{
	int i;
	const flexsetting_t *pSetting = NULL;

	for ( i = 0; i < pSettinghdr->numflexsettings; i++ )
	{
		pSetting = pSettinghdr->pSetting( i );
		if ( !pSetting )
			continue;

		const char *name = pSetting->pszName();

		if ( !stricmp( name, expr ) )
			break;
	}

	if ( i>=pSettinghdr->numflexsettings )
	{
		return NULL;
	}

	return pSetting;
}

#if 0

switch( phoneme )
{
// vowels
case 'i':		// high front unrounded vowel
	break;
case 'y':		// high front rounded vowel
	break;
case 0x0268:	// high central unrounded vowel
	break;
case 0x0289:	// high central rounded vowel
	break;
case 0x026f:	// high back unrounded vowel
case 0x019c:
	break;
case 'u':		// high back rounded vowel
	break;

case 0x026a:	// semi-high front unrounded vowel
case 0x0269:
case 0x0196:
case 0x03b9:
case 0x0197:
	break;
case 0x028f:	// semi-high front rounded vowel
	break;
case 0x028a:	// semi-high back rounded vowel
case 0x0277:
case 0x01b1:
case 0x03c5:
	break;

case 'e':		// upper-mid front unrounded vowel
	break;
case 0x00F8:	// upper-mid front rounded vowel
	break;
case 0x0258:	// upper-mid central unrounded vowel
	break;
case 0x0275:	// rounded mid-central vowel, rounded schwa
case 0x0473:
case 0x04e9:
	break;
case 0x0264:	// upper-mid back unrounded vowel
	break;
case 'o':		// upper-mid back rounded vowel
	break;

case 0x0259:	// mid-central unrounded vowel
case 0x018f:
case 0x01dd:
case 0x04d9:
	break;

case 0x025b:	// lower-mid front unrounded vowel
case 0x0190:
case 0x03b5:
	break;
case 0x029a:	// lower-mid front rounded vowel
case 0x0153:
	break;
case 0x025c:	// lower-mid central unrounded vowel
	break;
case 0x025e:	// lower-mid central rounded vowel
	break;
case 0x028c:	// lower-mid back unrounded vowel
case 0x039b:
case 0x2038:
case 0x2227:
	break;
case 0x0254:	// lower-mid back rounded vowel
case 0x0186:
	break;

case 0x00e6:	// semi-low front unrounded vowel
	break;
case 0x0250:	// semi-low central unrounded vowel
	break;

case 'a':		// low front unrounded vowel
	break;
case 0x0276:	// low front rounded vowel
	break;
case 0x0251:	// low back unrounded vowel
case 0x03B1:
	break;
case 0x0252:	// low back rounded vowel
	break;

// Constonants
// Plosive
case 't':		// voiceless bilabial stop
	break;
case 'd':		// voiced bilabial stop
	break;
case 'p':		// voiceless alveolar stop
	break;
case 'b':		// voiced alveolar stop
	break;
case 0x0288:	// voiceless retroflex stop
case 0x01ae:
	break;
case 0x0256:	// voiced retroflex stop
case 0x0189:
	break;
case 'c':		// voiceless palatal stop
	break;
case 0x025f:	// voiced palatal stop
	break;
case 'k':		// voiceless velar stop
	break;
case 'g':		// voiced velar stop
case 0x0261:
	break;
case 'q':		// voiceless uvular stop
	break;
case 0x0262:	// voiced uvular stop
	break;

// Nasal
case 'm':		// voiced bilabial nasal
	break;
case 0x0271:	// voiced labiodental nasal
	break;
case 'n':		// voiced alveolar nasal
	break;
case 0x0273:	// voiced retroflex nasal
	break;
case 0x0272:	// voiced palatal nasal
case 0x019d:
	break;
case 0x014b:	// voiced velar nasal
	break;
case 0x0274:	// voiced uvular nasal
	break;

// Trill Consonants
case 0x0299:	// bilabial trill
	break;
case 'r':		// voiced alveolar trill
	break;
case 0x0280:	// voiced uvular trill
	break;

// Tap or Flap
case 0x027d:	// voiced retroflex flap
	break;
case 0x027e:	// voiced alveolar flap or tap
	break;

// Fricative			
case 0x0278:	// voiceless bilabial fricative
case 0x03c6:
	break;
case 0x03b2:	// voiced bilabial fricative
	break;
case 'f':		// voiceless labiodental fricative
	break;
case 'v':		// voiced labialdental fricative
	break;
case 0x03b8:	// voiceless dental fricative (upper or lower case theta?)
	break;
case 0x00f0:	// voiced dental fricative
	break;
case 's':		// voiceless alveolar fricative
	break;
case 'z':		// voiced alveolar fricative
	break;
case 0x0283:	// voiceless postalveolar fricative
case 0x01a9:
case 0x222b:
	break;
case 0x0292:	// voiced postalveolar fricative
case 0x021d:
case 0x04e1:
case 0x2125:
	break;
case 0x0282:	// voiceless retroflex fricative
	break;
case 0x0290:	// voiced retroflex fricative
	break;
case 0x00e7:	// voiceless palatal fricative
	break;
case 0x029d:	// voiced palatal fricative
	break;
case 'x':		// voiceless valar fricative
	break;
case 0x0263:	// voiced velar fricative
case 0x0194:
case 0x03b3:
	break;
case 0x03c7:	// voiceless uvular fricative
	break;
case 0x0281:	// voiced uvular fricative or approximant
case 0x02b6:
	break;
case 0x0127:	// voiceless pharygeal fricative
	break;
case 0x0295:	// voiced pharyngeal fricative
case 0x01b9:
case 0x02c1:
	break;
case 'h':		// voiceless glottal fricative
	break;
case 0x0266:	// breathy-voiced glottal fricative
case 0x02b1:
	break;

// Lateral fricative
case 0x026c:	// voiceless alveolar lateral fricative
	break;
case 0x026e:	// voiced lateral fricative
	break;

// Approximant
case 0x028b:	// voiced labiodental approximant
case 0x01b2:
	break;
case 0x0279:	// voiced alveolar approximant
case 0x02b4:
	break;
case 0x027b:	// voiced retroflex approximant
case 0x02b5:
	break;
case 'j':		// voiced palatal approximant
	break;
case 0x0270:	// voiced velar approximant
	break;

// Lateral approximant
case 'l':		// voiced alveolar lateral approximant
	break;
case 0x026d:	// voiced retroflex lateral approximant
	break;
case 0x028e:	// voiced palatal lateral approximant
	break;
case 0x029f:	// voiced velar lateral approximant
	break;

// Clicks
case 0x0298:	// bilabial click
case 0x2299:
	break;
case 0x0287:	// dental click
	break;
case '!':		// (post)alveolar click
	break;

// Voiced implosives
case 0x0253:	// implosive bilabial stop
case 0x0181:
	break;
case 0x0257:	// implosive dental/alveolar stop
case 0x018a:
	break;
case 0x0284:	// implosive palatal stop
	break;
case 0x0260:	// implosive velar stop
	break;
case 0x029b:	// voiced uvular implosive
	break;

// Other
case 0x028d:	// voiceless rounded labiovelar approximant (fricative???)
	break;
case 'W':		// voiced labial-velar approximant
case 'w':
	break;
case 0x0265:	// voiced rounded palatal approximant ??
	break;
case 0x029c:	// voiceless epiglottal fricative
	break;
case 0x02a2:	// voiced epiglottal fricative
	break;
case 0x02a1:	// voiced epiglottal stop (plosive??)
	break;
case 0x0255:	// voiceless alveolo-palatal laminal fricative
	break;
case 0x0291:	// voiced alveolo-palatal laminal fricative
	break;
case 0x027a:	// voiced lateral flap
	break;
case 0x0267:	// voiceless coarticulated velar and palatoalveolar fricative
	break;

// Used but can't find definition
case 0x025a:	// rhotacized schwa
	break;
case '_':	// silence
	break;

// Undefined and hopefully unused
case 0x025d:	// rhotacized lower-mid central vowel
	break;
case 0x026b:	// velarized voiced alveolar lateral approximant
	break;
case 0x027c:	// voiced strident apico-alveolar till
	break;
case 0x027f:	// apical dental vowel
	break;
case 0x0285:	// apical retroflex vowel
	break;
case 0x0286:	// palatalized voiceless postalveolar fricative
	break;
case 0x0293:	// palatilized voiced postalveolar fricative
	break;
case 0x0294:	// glottal stop
case 0x02c0:
	break;
case 0x0296:	// lateral click
case 0x01c1:
	break;
case 0x0297:	// palatal (or alveolar) click
case 0x01c3:
case 0x2201:
	break;
case 0x029e:	// unused
	break;
case 0x02a0:	// voiceless uvular implosive
	break;

case 0x02a3:	// voiced dental affricate
	break;
case 0x02a4:	// voiced postalveolar afficate
	break;
case 0x02a5:	// voiced alveolo-palatal affricate
	break;
case 0x02a6:	// voiceless dental affricate
	break;
case 0x02a7:	// voiceless postalveolar affricate
	break;
case 0x02a8:	// voiceless alveolo-palatal affricate
	break;
case 0x02a9:	// velopharyngeal fricative
	break;
case 0x02aa:	// lateral alveolar fricative (lisp)
	break;
case 0x02ab:	// voiced lateral alveolar fricative
	break;
case 0x02ac:	// audible lip smack
	break;
case 0x02ad:	// audible teeth gnashing
	break;
}
#endif