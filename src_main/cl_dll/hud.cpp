/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// hud.cpp
//
// implementation of CHud class
//
#include "cbase.h"
#include "hud_macros.h"
#include "history_resource.h"
#include "parsemsg.h"
#include "iinput.h"
#include "clientmode.h"
#include "in_buttons.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include "itextmessage.h"
#include "mempool.h"
#include <KeyValues.h>
#include "filesystem.h"
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void __MsgFunc_ClearDecals(const char *pszName, int iSize, void *pbuf);

static 	CClassMemoryPool< CHudTexture >	 g_HudTextureMemoryPool( 128 );

ConVar hud_autoreloadscript("hud_autoreloadscript", "0", FCVAR_ARCHIVE, "Automatically reloads the animation script each time one is ran");

//-----------------------------------------------------------------------------
// Purpose: Parses the weapon txt files to get the sprites needed.
//-----------------------------------------------------------------------------
void LoadHudTextures( CUtlDict< CHudTexture *, int >& list, char *psz )
{
	KeyValues *pTemp, *pTextureSection;

	KeyValues *pKeyValuesData = new KeyValues( "WeaponDatafile" );
	if ( pKeyValuesData->LoadFromFile( filesystem, psz ) )
	{
		pTextureSection = pKeyValuesData->FindKey( "TextureData" );
		if ( pTextureSection  )
		{
			// Read the sprite data
			pTemp = pTextureSection->GetFirstSubKey();
			while ( pTemp )
			{
				CHudTexture *tex = new CHudTexture();

				// Key Name is the sprite name
				strcpy( tex->szShortName, pTemp->GetName() );
				strcpy( tex->szTextureFile, pTemp->GetString( "file" ) );
				tex->rc.left	= pTemp->GetInt( "x", 0 );
				tex->rc.top		= pTemp->GetInt( "y", 0 );
				tex->rc.right	= pTemp->GetInt( "width", 0 )	+ tex->rc.left;
				tex->rc.bottom	= pTemp->GetInt( "height", 0 )	+ tex->rc.top;

				list.Insert( tex->szShortName, tex );

				pTemp = pTemp->GetNextKey();
			}
		}
	}

	// Failed for some reason. Delete the Key data and abort.
	pKeyValuesData->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : * - 
//			list - 
//-----------------------------------------------------------------------------
void FreeHudTextureList( CUtlDict< CHudTexture *, int >& list )
{
	int c = list.Count();
	for ( int i = 0; i < c; i++ )
	{
		CHudTexture *tex = list[ i ];
		delete tex;
	}
	list.RemoveAll();
}

// Globally-used fonts
vgui::HFont g_hFontTrebuchet24 = vgui::INVALID_FONT;
vgui::HFont g_hFontTrebuchet40 = vgui::INVALID_FONT;

//=======================================================================================================================
// Hud Element Visibility handling
//=======================================================================================================================
typedef struct hudelement_hidden_s
{
	char	*sElementName;
	int		iHiddenBits;	// Bits in which this hud element is hidden
} hudelement_hidden_t;

hudelement_hidden_t sHudHiddenArray[] =
{
	{ "CHudHealth", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT },
	{ "CHudWeaponSelection", HIDEHUD_WEAPONS | HIDEHUD_NEEDSUIT | HIDEHUD_PLAYERDEAD },
	{ "CHudHistoryResource", HIDEHUD_MISCSTATUS },
	{ "CHudTrain", HIDEHUD_MISCSTATUS },
	{ "ScorePanel", 0 },
	{ "CVProfPanel", 0 },
	{ "CBudgetPanel", 0 },
	{ "CPDumpPanel", 0 },
	{ "CHudMOTD", 0 },
	{ "CHudMessage", 0 },
	{ "CHudMenu", HIDEHUD_MISCSTATUS },
	{ "CHudChat", HIDEHUD_CHAT },
	{ "CHudDeathNotice", HIDEHUD_MISCSTATUS },
	{ "CHudCloseCaption", 0 },  // Never hidden if needed
	{ "CHudAnimationInfo", 0 }, // For debugging, never hidden if visible
	{ "CHudDamageIndicator", HIDEHUD_HEALTH },

	// HL2 hud elements
	{ "CHudBattery", HIDEHUD_HEALTH | HIDEHUD_NEEDSUIT },
	{ "CHudSuitPower", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT },
	{ "CHudFlashlight", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT },
	{ "CHudGeiger", HIDEHUD_HEALTH },
	{ "CHudAmmo", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT },
	{ "CHudSecondaryAmmo", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT },
	{ "CHudZoom", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT },
	{ "CHUDQuickInfo", 0 },

	// TF2 hud elements
	{ "CHudResources", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD },
	{ "CHudResourcesPickup", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD },
	{ "CTargetID", HIDEHUD_MISCSTATUS },
	{ "CHudOrderList", HIDEHUD_MISCSTATUS | HIDEHUD_PLAYERDEAD },
	{ "CHudObjectStatuses", HIDEHUD_MISCSTATUS | HIDEHUD_PLAYERDEAD },
	{ "CHudEMP", HIDEHUD_HEALTH },
	{ "CHudMortar", HIDEHUD_PLAYERDEAD },
	{ "CHudWeaponFlashHelper", HIDEHUD_MISCSTATUS | HIDEHUD_PLAYERDEAD },
	{ "CVehicleRoleHudElement", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD },
	{ "CHudTimer", HIDEHUD_MISCSTATUS },
	{ "CHudVehicleHealth", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD },
	{ "CMinimapPanel", 0 },

	{ "CHudAmmoPrimary", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD },
	{ "CHudAmmoPrimaryClip", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD },
	{ "CHudAmmoSecondary", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD },

	{ "CHudVehicle", HIDEHUD_PLAYERDEAD },
	{ "CHudCrosshair", HIDEHUD_PLAYERDEAD },
	{ "CHudWeapon", HIDEHUD_PLAYERDEAD | HIDEHUD_WEAPONS },
	{ "CHudProgressBar", HIDEHUD_PLAYERDEAD | HIDEHUD_WEAPONS },
	
	// Counter-Strike hud elements.
	{ "CHudRoundTimer", 0 },
	{ "CHudAccount", HIDEHUD_PLAYERDEAD },
	{ "CHudShoppingCart", HIDEHUD_PLAYERDEAD },
	{ "CHudC4", HIDEHUD_PLAYERDEAD },
	{ "CHudArmor", HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD },

	{ NULL, 0 },
};

ConVar hidehud( "hidehud", "0", 0 );

//=======================================================================================================================
//  CHudElement
//	All hud elements are derived from this class.
//=======================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: Registers the hud element in a global list, in CHud
//-----------------------------------------------------------------------------
CHudElement::CHudElement( const char *pElementName )
{
	m_bActive = false;
	m_iHiddenBits = 0;
	m_pElementName = pElementName;
	SetNeedsRemove( false );
}

//-----------------------------------------------------------------------------
// Purpose: Remove this hud element from the global list in CHUD
//-----------------------------------------------------------------------------
CHudElement::~CHudElement()
{
	if ( m_bNeedsRemove )
	{
		gHUD.RemoveHudElement( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudElement::SetActive( bool bActive )
{
	m_bActive = bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : needsremove - 
//-----------------------------------------------------------------------------
void CHudElement::SetNeedsRemove( bool needsremove )
{
	m_bNeedsRemove = needsremove;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudElement::SetHiddenBits( int iBits )
{
	m_iHiddenBits = iBits;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudElement::ShouldDraw( void )
{
	return !gHUD.IsHidden( m_iHiddenBits );
}

/*============================================
  CHud class definition
============================================*/
CHud gHUD;  // global HUD object

DECLARE_MESSAGE(gHUD, ResetHUD);
DECLARE_MESSAGE(gHUD, InitHUD);
DECLARE_MESSAGE(gHUD, GameMode);
DECLARE_MESSAGE(gHUD, TeamChange);

CHud::CHud()
{
}

//-----------------------------------------------------------------------------
// Purpose: This is called every time the DLL is loaded
//-----------------------------------------------------------------------------
void CHud::Init( void )
{
	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( GameMode );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( TeamChange );
	HOOK_MESSAGE( ClearDecals );

	InitFonts();

	// Create all the Hud elements and then tell them to initialise
	CHudElementHelper::CreateAllElements();
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->Init();
	}

	MsgFunc_ResetHUD( 0, 0, NULL );

	m_bHudTexturesLoaded = false;

	KeyValues *kv = new KeyValues( "layout" );
	if ( kv )
	{
		if ( kv->LoadFromFile( filesystem, "scripts/HudLayout.res" ) )
		{
			for ( int i = 0; i < m_HudList.Size(); i++ )
			{
				vgui::Panel *pPanel = dynamic_cast<vgui::Panel*>(m_HudList[i]);
				if ( !pPanel )
				{
					Msg( "Non-vgui hud element %s\n", m_HudList[i]->GetName() );
					continue;
				}

				KeyValues *key = kv->FindKey( pPanel->GetName(), false );
				if ( !key )
				{
					Msg( "Hud element '%s' doesn't have an entry '%s' in scripts/HudLayout.res\n", m_HudList[i]->GetName(), pPanel->GetName() );
				}

				if ( !pPanel->GetParent() )
				{
					Msg( "Hud element '%s'/'%s' doesn't have a parent\n", m_HudList[i]->GetName(), pPanel->GetName() );
				}
			}
		}

		kv->deleteThis();
	}

	if ( m_bHudTexturesLoaded )
		return;

	m_bHudTexturesLoaded = true;
	CUtlDict< CHudTexture *, int >	textureList;

	// check to see if we have sprites for this res; if not, step down
	LoadHudTextures( textureList, VarArgs("scripts/hud_textures.txt" ) );
	LoadHudTextures( textureList, VarArgs("scripts/mod_textures.txt" ) );

	int c = textureList.Count();
	for ( int index = 0; index < c; index++ )
	{
		CHudTexture* tex = textureList[ index ];
		AddSearchableHudIconToList( *tex );
	}

	FreeHudTextureList( textureList );
}

//-----------------------------------------------------------------------------
// Purpose: Init Hud global colors
// Input  : *scheme - 
//-----------------------------------------------------------------------------
void CHud::InitColors( vgui::IScheme *scheme )
{
	m_clrNormal = scheme->GetColor( "Normal", Color( 255, 208, 64 ,255 ) );
	m_clrCaution = scheme->GetColor( "Caution", Color( 255, 48, 0, 255 ) );
	m_clrYellowish = scheme->GetColor( "Yellowish", Color( 255, 160, 0, 255 ) );
}

//-----------------------------------------------------------------------------
// Initializes fonts
//-----------------------------------------------------------------------------
void CHud::InitFonts()
{
	g_hFontTrebuchet24 = vgui::surface()->CreateFont();
	vgui::surface()->AddGlyphSetToFont( g_hFontTrebuchet24, "Trebuchet MS", 24, 900, 0, 0, 0, 0x0, 0x7F );
	g_hFontTrebuchet40 = vgui::surface()->CreateFont();
	vgui::surface()->AddGlyphSetToFont( g_hFontTrebuchet40, "Trebuchet MS", 40, 900, 0, 0, 0, 0x0, 0x7F );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHud::Shutdown( void )
{
	// Delete all the Hud elements
	int iMax = m_HudList.Size();
	for ( int i = iMax-1; i >= 0; i-- )
	{
		delete m_HudList[i];
	}
	m_HudList.Purge();
	m_bHudTexturesLoaded = false;
}


//-----------------------------------------------------------------------------
// Purpose: LevelInit's called whenever a new level's starting
//-----------------------------------------------------------------------------
void CHud::LevelInit( void )
{
	// Tell all the registered hud elements to LevelInit
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->SetGameRestored( false );
		m_HudList[i]->LevelInit();
	}
}

//-----------------------------------------------------------------------------
// Purpose: LevelShutdown's called whenever a level's finishing
//-----------------------------------------------------------------------------
void CHud::LevelShutdown( void )
{
	// Tell all the registered hud elements to LevelInit
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->LevelShutdown();
		m_HudList[i]->SetGameRestored( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: cleans up memory allocated for m_rg* arrays
//-----------------------------------------------------------------------------
CHud::~CHud()
{
	int c = m_Icons.Count();
	for ( int i = c - 1; i >= 0; i-- )
	{
		CHudTexture *tex = m_Icons[ i ];
		g_HudTextureMemoryPool.Free( tex );
	}
	m_Icons.Purge();
}

void CHudTexture::DrawSelf( int x, int y, Color& clr ) const
{
	DrawSelf( x, y, Width(), Height(), clr );
}

void CHudTexture::DrawSelf( int x, int y, int w, int h, Color& clr ) const
{
	if ( textureId == -1 )
		return;

	vgui::surface()->DrawSetTexture( textureId );
	vgui::surface()->DrawSetColor( clr );
	vgui::surface()->DrawTexturedSubRect( x, y, x + w, y + h, 
		texCoords[ 0 ], texCoords[ 1 ], texCoords[ 2 ], texCoords[ 3 ] );
}

void CHudTexture::DrawSelfCropped( int x, int y, int cropx, int cropy, int cropw, int croph, Color& clr ) const
{
	if ( textureId == -1 )
		return;

	float fw = (float)Width();
	float fh = (float)Height();

	float twidth	= texCoords[ 2 ] - texCoords[ 0 ];
	float theight	= texCoords[ 3 ] - texCoords[ 1 ];

	// Interpolate coords
	float tCoords[ 4 ];
	tCoords[ 0 ] = texCoords[ 0 ] + ( (float)cropx / fw ) * twidth;
	tCoords[ 1 ] = texCoords[ 1 ] + ( (float)cropy / fh ) * theight;
	tCoords[ 2 ] = texCoords[ 0 ] + ( (float)(cropx + cropw ) / fw ) * twidth;
	tCoords[ 3 ] = texCoords[ 1 ] + ( (float)(cropy + croph ) / fh ) * theight;

	vgui::surface()->DrawSetTexture( textureId );
	vgui::surface()->DrawSetColor( clr );
	vgui::surface()->DrawTexturedSubRect( 
		x, y, 
		x + cropw, y + croph, 
		tCoords[ 0 ], tCoords[ 1 ], 
		tCoords[ 2 ], tCoords[ 3 ] );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *t - 
//-----------------------------------------------------------------------------
void CHud::SetupNewHudTexture( CHudTexture *t )
{
	// Set up texture id and texture coordinates
	t->textureId = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile( t->textureId, t->szTextureFile, false, false );

	int wide, tall;
	vgui::surface()->DrawGetTextureSize( t->textureId, wide, tall );

	t->texCoords[ 0 ] = (float)(t->rc.left + 0.5f) / (float)wide;
	t->texCoords[ 1 ] = (float)(t->rc.top + 0.5f) / (float)tall;
	t->texCoords[ 2 ] = (float)(t->rc.right - 0.5f) / (float)wide;
	t->texCoords[ 3 ] = (float)(t->rc.bottom - 0.5f) / (float)tall;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture *CHud::AddUnsearchableHudIconToList( CHudTexture& texture )
{
	// These names are composed based on the texture file name
	char composedName[ 512 ];
	Q_snprintf( composedName, sizeof( composedName ), "%s_%i_%i_%i_%i",
		texture.szTextureFile, texture.rc.left, texture.rc.top, texture.rc.right, texture.rc.bottom );

	CHudTexture *icon = GetIcon( composedName );
	if ( icon )
	{
		return icon;
	}

	CHudTexture *newTexture = ( CHudTexture * )g_HudTextureMemoryPool.Alloc();
	*newTexture = texture;

	SetupNewHudTexture( newTexture );

	int idx = m_Icons.Insert( composedName, newTexture );
	return m_Icons[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : texture - 
// Output : CHudTexture
//-----------------------------------------------------------------------------
CHudTexture *CHud::AddSearchableHudIconToList( CHudTexture& texture )
{
	CHudTexture *icon = GetIcon( texture.szShortName );
	if ( icon )
	{
		return icon;
	}

	CHudTexture *newTexture = ( CHudTexture * )g_HudTextureMemoryPool.Alloc();
	*newTexture = texture;

	SetupNewHudTexture( newTexture );

	int idx = m_Icons.Insert( texture.szShortName, newTexture );
	return m_Icons[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to an icon in the list
// Input  : *szIcon - 
// Output : CHudTexture
//-----------------------------------------------------------------------------
CHudTexture *CHud::GetIcon( const char *szIcon )
{
	int i = m_Icons.Find( szIcon );
	if ( i == m_Icons.InvalidIndex() )
		return NULL;

	return m_Icons[ i ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHud::RefreshHudTextures()
{
	if ( !m_bHudTexturesLoaded )
	{
		Assert( 0 );
		return;
	}

	CUtlDict< CHudTexture *, int >	textureList;

	// check to see if we have sprites for this res; if not, step down
	LoadHudTextures( textureList, VarArgs("scripts/hud_textures.txt" ) );
	LoadHudTextures( textureList, VarArgs("scripts/mod_textures.txt" ) );

	int c = textureList.Count();
	for ( int index = 0; index < c; index++ )
	{
		CHudTexture *tex = textureList[ index ];
		Assert( tex );

		CHudTexture *icon = GetIcon( tex->szShortName );
		if ( !icon )
			continue;

		// Update file
		strcpy( icon->szTextureFile, tex->szTextureFile );
		
		// Update subrect
		icon->rc = tex->rc;

		// Keep existing texture id, but now update texture file and texture coordinates
		vgui::surface()->DrawSetTextureFile( icon->textureId, icon->szTextureFile, false, false );

		// Get new texture dimensions in case it changed
		int wide, tall;
		vgui::surface()->DrawGetTextureSize( icon->textureId, wide, tall );

		// Assign coords
		icon->texCoords[ 0 ] = (float)(icon->rc.left + 0.5f) / (float)wide;
		icon->texCoords[ 1 ] = (float)(icon->rc.top + 0.5f) / (float)tall;
		icon->texCoords[ 2 ] = (float)(icon->rc.right - 0.5f) / (float)wide;
		icon->texCoords[ 3 ] = (float)(icon->rc.bottom - 0.5f) / (float)tall;
	}

	FreeHudTextureList( textureList );
}

void CHud::OnRestore()
{
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->SetGameRestored( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHud::VidInit( void )
{
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->VidInit();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudElement *CHud::FindElement( const char *pName )
{
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		if ( stricmp( m_HudList[i]->GetName(), pName ) == 0 )
			return m_HudList[i];
	}

	DevWarning(1, "Could not find Hud Element: %s\n", pName );
	assert(0);
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a member to the HUD
//-----------------------------------------------------------------------------
void CHud::AddHudElement( CHudElement *pHudElement ) 
{
	// Add the hud element to the end of the array
	m_HudList.AddToTail( pHudElement );

	pHudElement->SetNeedsRemove( true );

	// Set the hud element's visibility
	int i = 0;
	while ( sHudHiddenArray[i].sElementName )
	{
		if ( !strcmp( sHudHiddenArray[i].sElementName, pHudElement->GetName() ) )
		{
			pHudElement->SetHiddenBits( sHudHiddenArray[i].iHiddenBits );
			return;
		}

		i++;
	}

	// Couldn't find the hud element's entry in the hidden array
	// Add an entry for your new hud element to the sHudHiddenArray array at the top of this file.
	Assert( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Remove an element from the HUD
//-----------------------------------------------------------------------------
void CHud::RemoveHudElement( CHudElement *pHudElement ) 
{
	m_HudList.FindAndRemove( pHudElement );
}

//-----------------------------------------------------------------------------
// Purpose: Returns current mouse sensitivity setting
// Output : float - the return value
//-----------------------------------------------------------------------------
float CHud::GetSensitivity( void )
{
	return m_flMouseSensitivity;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the passed in sections of the HUD shouldn't be drawn
//-----------------------------------------------------------------------------
bool CHud::IsHidden( int iHudFlags )
{
	// Not in game?
	if ( !engine->IsInGame() )
		return true;

	// No local player yet?
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return true;

	// Get current hidden flags
	int iHideHud = pPlayer->m_Local.m_iHideHUD;
	if ( hidehud.GetInt() )
	{
		iHideHud = hidehud.GetInt();
	}

	// Everything hidden?
	if ( iHideHud & HIDEHUD_ALL )
		return true;

	// Local player dead?
	if ( ( iHudFlags & HIDEHUD_PLAYERDEAD ) && ( pPlayer->GetHealth() <= 0 ) )
		return true;

	// Need the HEV suit ( HL2 )
	if ( ( iHudFlags & HIDEHUD_NEEDSUIT ) && ( !pPlayer->IsSuitEquipped() ) )
		return true;

	return ( ( iHudFlags & iHideHud ) != 0);
}

//-----------------------------------------------------------------------------
// Purpose: Allows HUD to modify input data
//-----------------------------------------------------------------------------
void CHud::ProcessInput( bool bActive )
{
	if ( bActive )
	{
		m_iKeyBits = input->GetButtonBits( 0 );

		// Weaponbits need to be sent down as a UserMsg now.
		gHUD.Think();

		if ( !C_BasePlayer::GetLocalPlayer() )
		{
			extern ConVar default_fov;
			engine->SetFieldOfView( default_fov.GetFloat() ); 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows HUD to Think and modify input data
// Input  : *cdata - 
//			time - 
// Output : int - 1 if there were changes, 0 otherwise
//-----------------------------------------------------------------------------
void CHud::UpdateHud( bool bActive )
{
	// clear the weapon bits.
	gHUD.m_iKeyBits &= (~(IN_WEAPON1|IN_WEAPON2));

	g_pClientMode->Update();
}

//-----------------------------------------------------------------------------
// Purpose: Force a Hud UI anim to play
//-----------------------------------------------------------------------------
void TestHudAnim_f( void )
{
	if (engine->Cmd_Argc() != 2)
	{
		Msg("Usage:\n   testhudanim <anim name>\n");
		return;
	}

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( engine->Cmd_Argv(1) );
}

static ConCommand testhudanim( "testhudanim", TestHudAnim_f, "Test a hud element animation.\n\tArguments: <anim name>\n", FCVAR_CHEAT );