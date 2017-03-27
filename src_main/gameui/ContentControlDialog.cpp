//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include <memory.h>
#include <windows.h>

#include "ContentControlDialog.h"
#include "checksum_md5.h"
#include "EngineInterface.h"

#include <vgui/IInput.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/TextEntry.h>
#include <tier0/vcrmode.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CContentControlDialog::CContentControlDialog(vgui::Panel *parent) : CTaskFrame(parent, "ContentControlDialog")
{
	SetBounds(0, 0, 372, 160);
	SetSizeable( false );

	g_pTaskbar->AddTask(GetVPanel());

	SetTitle( "#GameUI_ContentLock", true );

	m_pStatus = new vgui::Label( this, "ContentStatus", "" );

	m_pPasswordLabel = new vgui::Label( this, "PasswordPrompt", "#GameUI_PasswordPrompt" );
	m_pPassword2Label = new vgui::Label( this, "PasswordReentryPrompt", "#GameUI_PasswordReentryPrompt" );

	m_pExplain = new vgui::Label( this, "ContentControlExplain", "" );

	m_pPassword = new vgui::TextEntry( this, "Password" );
	m_pPassword2 = new vgui::TextEntry( this, "Password2" );

	m_pOK = new vgui::Button( this, "Ok", "#GameUI_OK" );
	m_pOK->SetCommand( "Ok" );

	vgui::Button *cancel = new vgui::Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Cancel" );

	m_szGorePW[ 0 ] = 0;
    ResetPassword();

	LoadControlSettings("Resource\\ContentControlDialog.res");

//	Explain("");
//	UpdateContentControlStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CContentControlDialog::~CContentControlDialog()
{
}

void CContentControlDialog::Activate()
{
    BaseClass::Activate();

    m_pPassword->SetText("");
    m_pPassword->RequestFocus();
    m_pPassword2->SetText("");
	Explain("");
	UpdateContentControlStatus();

	input()->SetAppModalSurface(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CContentControlDialog::ResetPassword()
{
	// Set initial value
	HKEY key;
	if ( ERROR_SUCCESS == g_pVCR->Hook_RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Valve\\Half-Life\\Settings", 0, KEY_READ, &key))
	{
		DWORD type;
		DWORD bufSize = sizeof(m_szGorePW);

		g_pVCR->Hook_RegQueryValueEx(key, "User Token 2", NULL, &type, (unsigned char *)m_szGorePW, &bufSize );
		g_pVCR->Hook_RegCloseKey( key );
	}
    else
    {
        m_szGorePW[ 0 ] = 0;
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CContentControlDialog::ApplyPassword()
{
    WriteToken( m_szGorePW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CContentControlDialog::Explain( char const *fmt, ... )
{
	if ( !m_pExplain )
		return;

	va_list		argptr;
	char		text[1024];

	va_start (argptr,fmt);
	_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	m_pExplain->SetText( text );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *command - 
//-----------------------------------------------------------------------------
void CContentControlDialog::OnCommand( const char *command )
{
	if ( !stricmp( command, "Ok" ) )
	{
		bool canclose = false;

		char pw1[ 256 ];
		char pw2[ 256 ];

		m_pPassword->GetText( pw1, 256 );
		m_pPassword2->GetText( pw2, 256 );

        // Get text and check
//        bool enabled = PasswordEnabled(); //( m_szGorePW[0]!=0 ) ? true : false;
//		bool pwMatch = stricmp( pw1, pw2 ) == 0 ? true : false;

        if (IsPasswordEnabledInDialog())
        {
            canclose = DisablePassword(pw1);
//            canclose = CheckPassword( m_szGorePW, pw1, false );
        }
        else if (!strcmp(pw1, pw2))
        {
            canclose = EnablePassword(pw1);
//            canclose = CheckPassword( NULL, pw1, true );
        }
		else
		{
			Explain( "#GameUI_PasswordsDontMatch" );
		}

		if ( canclose )
		{
			OnClose();
		}
	}
	else if ( !stricmp( command, "Cancel" ) )
	{
		OnClose();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CContentControlDialog::OnClose()
{
	BaseClass::OnClose();
    PostActionSignal(new KeyValues("ContentControlClose"));
//	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char *CContentControlDialog::BinPrintf( unsigned char *buf, int nLen )
{
	static char szReturn[1024];
	unsigned char c;
	char szChunk[10];
	int i;

	memset(szReturn, 0, sizeof( szReturn ) );

	for (i = 0; i < nLen; i++)
	{
		c = (unsigned char)buf[i];
		_snprintf(szChunk, sizeof(szChunk), "%02x", c);
		strncat(szReturn, szChunk, sizeof(szReturn) - strlen(szReturn) - 1);
	}

	return szReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CContentControlDialog::WriteToken( const char *str )
{
	// Set initial value
	HKEY key;
	if ( ERROR_SUCCESS == g_pVCR->Hook_RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Valve\\Half-Life\\Settings", 0, KEY_WRITE, &key))
	{
		DWORD type = REG_SZ;
		DWORD bufSize = strlen( str ) + 1;

		g_pVCR->Hook_RegSetValueEx(key, "User Token 2", 0, type, (const unsigned char *)str, bufSize );

		g_pVCR->Hook_RegCloseKey( key );
	}

	strcpy( m_szGorePW, str );

	UpdateContentControlStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CContentControlDialog::HashPassword(const char *newPW, char *hashBuffer)
{
	// Compute the md5 hash and save it.
	unsigned char md5_hash[16];
	MD5Context_t ctx;

	MD5Init( &ctx );
	MD5Update( &ctx, (unsigned char const *)(LPCSTR)newPW, strlen( newPW ) );
	MD5Final( md5_hash, &ctx );

//	char digestedPW[ 128 ];
	strcpy( hashBuffer, BinPrintf( md5_hash, 16 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/*
bool CContentControlDialog::CheckPassword( char const *oldPW, char const *newPW, bool enableContentControl )
{
	char digestedPW[ 128 ];
    HashPassword(newPW, digestedPW);
	
    // Compute the md5 hash and save it.
	unsigned char md5_hash[16];
	MD5Context_t ctx;

	MD5Init( &ctx );
	MD5Update( &ctx, (unsigned char const *)(LPCSTR)newPW, strlen( newPW ) );
	MD5Final( md5_hash, &ctx );

	strcpy( digestedPW, BinPrintf( md5_hash, 16 ) );
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CContentControlDialog::EnablePassword(const char *newPW)
{
    if ( !newPW[ 0 ] )
    {
        Explain( "#GameUI_MustEnterPassword" );
        return false;
    }

	char digestedPW[ 128 ];
    HashPassword(newPW, digestedPW);

	// disable violence
/*	engine->Cvar_SetValue("violence_hblood", 0.0 );
	engine->Cvar_SetValue("violence_hgibs" , 0.0 );
	engine->Cvar_SetValue("violence_ablood", 0.0 );
	engine->Cvar_SetValue("violence_agibs" , 0.0 );
	*/

	ConVar *var = (ConVar *)cvar->FindVar( "violence_hblood" );
	if ( var )
	{
		var->SetValue(false);
	}
	var = (ConVar *)cvar->FindVar( "violence_hgibs" );
	if ( var )
	{
		var->SetValue(false);
	}
	var = (ConVar *)cvar->FindVar( "violence_ablood" );
	if ( var )
	{
		var->SetValue(false);
	}
	var = (ConVar *)cvar->FindVar( "violence_agibs" );
	if ( var )
	{
		var->SetValue(false);
	}
	

    // Store digest to registry
//    WriteToken( digestedPW );
    strcpy(m_szGorePW, digestedPW);
    /*
		}
		else
		{
			if ( stricmp( oldPW, digestedPW ) )
			{
				// Warn that password is invalid
				Explain( "#GameUI_IncorrectPassword" );
				return false;
			}
		}
	}*/
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CContentControlDialog::DisablePassword(const char *oldPW)
{
    if ( !oldPW[ 0 ] )
    {
        Explain( "#GameUI_MustEnterPassword" );
        return false;
    }

	char digestedPW[ 128 ];
    HashPassword(oldPW, digestedPW);

    if( stricmp( m_szGorePW, digestedPW ) )
    {
        Explain( "#GameUI_IncorrectPassword" );
        return false;
    }

    m_szGorePW[0] = 0;

	// set the violence cvars
/*	engine->Cvar_SetValue("violence_hblood", 1.0 );
	engine->Cvar_SetValue("violence_hgibs" , 1.0 );
	engine->Cvar_SetValue("violence_ablood", 1.0 );
	engine->Cvar_SetValue("violence_agibs" , 1.0 );
	*/
	ConVar *var = (ConVar *)cvar->FindVar( "violence_hblood" );
	if ( var )
	{
		var->SetValue(true);
	}
	var = (ConVar *)cvar->FindVar( "violence_hgibs" );
	if ( var )
	{
		var->SetValue(true);
	}
	var = (ConVar *)cvar->FindVar( "violence_ablood" );
	if ( var )
	{
		var->SetValue(true);
	}
	var = (ConVar *)cvar->FindVar( "violence_agibs" );
	if ( var )
	{
		var->SetValue(true);
	}


//		// Remove digest value
//		WriteToken( "" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CContentControlDialog::IsPasswordEnabledInDialog()
{
    return m_szGorePW[0] != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CContentControlDialog::UpdateContentControlStatus( void )
{
	bool enabled = IsPasswordEnabledInDialog(); //( m_szGorePW[0]!=0 ) ? true : false;
	m_pStatus->SetText( enabled ? "#GameUI_ContentStatusEnabled" : "#GameUI_ContentStatusDisabled" );

    if (enabled)
    {
        m_pPasswordLabel->SetText("#GameUI_PasswordDisablePrompt");
    }
    else
    {
        m_pPasswordLabel->SetText("#GameUI_PasswordPrompt");
    }

    // hide the re-entry
    m_pPassword2Label->SetVisible(!enabled);
    m_pPassword2->SetVisible(!enabled);
//	m_pOK->SetText( enabled ? "#GameUI_Disable" : "#GameUI_Enable" );
}
