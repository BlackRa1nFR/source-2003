//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "OptionsSubKeyboard.h"
#include "EngineInterface.h"
#include "VControlsListPanel.h"

#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/QueryBox.h>

#include <vgui/Cursor.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui/KeyCode.h>
#include <vgui/MouseCode.h>
#include <vgui/ISystem.h>
#include <vgui/IInput.h>

#include "FileSystem.h"
#include "UtlBuffer.h"
#include "igameuifuncs.h"
#include <vstdlib/IKeyValuesSystem.h>

#include "keydefs.h"
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define SCRIPTS_DIR "scripts"  // SRC path , don't have a trailing "/"
//#define SCRIPTS_DIR "gfx/shell"   // GoldSrc path

//-----------------------------------------------------------------------------
// Purpose: Converts a vgui key code into an engine key code
//-----------------------------------------------------------------------------
int ConvertVGUIToEngine( vgui::KeyCode code )
{
	switch ( code )
	{
	case vgui::KEY_0:
		return '0';
	case vgui::KEY_1:
		return '1';
	case vgui::KEY_2:
		return '2';
	case vgui::KEY_3:
		return '3';
	case vgui::KEY_4:
		return '4';
	case vgui::KEY_5:
		return '5';
	case vgui::KEY_6:
		return '6';
	case vgui::KEY_7:
		return '7';
	case vgui::KEY_8:
		return '8';
	case vgui::KEY_9:
		return '9';
	case vgui::KEY_A:
		return 'a';
	case vgui::KEY_B:
		return 'b';
	case vgui::KEY_C:
		return 'c';
	case vgui::KEY_D:
		return 'd';
	case vgui::KEY_E:
		return 'e';
	case vgui::KEY_F:
		return 'f';
	case vgui::KEY_G:
		return 'g';
	case vgui::KEY_H:
		return 'h';
	case vgui::KEY_I:
		return 'i';
	case vgui::KEY_J:
		return 'j';
	case vgui::KEY_K:
		return 'k';
	case vgui::KEY_L:
		return 'l';
	case vgui::KEY_M:
		return 'm';
	case vgui::KEY_N:
		return 'n';
	case vgui::KEY_O:
		return 'o';
	case vgui::KEY_P:
		return 'p';
	case vgui::KEY_Q:
		return 'q';
	case vgui::KEY_R:
		return 'r';
	case vgui::KEY_S:
		return 's';
	case vgui::KEY_T:
		return 't';
	case vgui::KEY_U:
		return 'u';
	case vgui::KEY_V:
		return 'v';
	case vgui::KEY_W:
		return 'w';
	case vgui::KEY_X:
		return 'x';
	case vgui::KEY_Y:
		return 'y';
	case vgui::KEY_Z:
		return 'z';
	case vgui::KEY_PAD_0:
		return K_KP_INS;
	case vgui::KEY_PAD_1:
		return K_KP_END;
	case vgui::KEY_PAD_2:
		return K_KP_DOWNARROW;
	case vgui::KEY_PAD_3:
		return K_KP_PGDN;
	case vgui::KEY_PAD_4:
		return K_KP_LEFTARROW;
	case vgui::KEY_PAD_5:
		return K_KP_5;
	case vgui::KEY_PAD_6:
		return K_KP_RIGHTARROW;
	case vgui::KEY_PAD_7:
		return K_KP_HOME;
	case vgui::KEY_PAD_8:
		return K_KP_UPARROW;
	case vgui::KEY_PAD_9:
		return K_KP_PGUP;
	case vgui::KEY_PAD_DIVIDE:
		return K_KP_SLASH;
	case vgui::KEY_PAD_MINUS:
		return K_KP_MINUS;
	case vgui::KEY_PAD_PLUS:
		return K_KP_PLUS;
	case vgui::KEY_PAD_ENTER:
		return K_KP_ENTER;
	case vgui::KEY_PAD_DECIMAL:
		return K_KP_DEL;
	case vgui::KEY_PAD_MULTIPLY:
		return '*';
	case vgui::KEY_LBRACKET:
		return '[';
	case vgui::KEY_RBRACKET:
		return ']';
	case vgui::KEY_SEMICOLON:
		return ';';
	case vgui::KEY_APOSTROPHE:
		return '\'';
	case vgui::KEY_BACKQUOTE:
		return '`';
	case vgui::KEY_COMMA:
		return ',';
	case vgui::KEY_PERIOD:
		return '.';
	case vgui::KEY_SLASH:
		return '/';
	case vgui::KEY_BACKSLASH:
		return '\\';
	case vgui::KEY_MINUS:
		return '-';
	case vgui::KEY_EQUAL:
		return '=';
	case vgui::KEY_ENTER:
		return K_ENTER;
	case vgui::KEY_SPACE:
		return K_SPACE;
	case vgui::KEY_BACKSPACE:
		return K_BACKSPACE;
	case vgui::KEY_TAB:
		return K_TAB;
	case vgui::KEY_CAPSLOCK:
		return K_CAPSLOCK;
	case vgui::KEY_ESCAPE:
		return K_ESCAPE;
	case vgui::KEY_INSERT:
		return K_INS;
	case vgui::KEY_DELETE:
		return K_DEL;
	case vgui::KEY_HOME:
		return K_HOME;
	case vgui::KEY_END:
		return K_END;
	case vgui::KEY_PAGEUP:
		return K_PGUP;
	case vgui::KEY_PAGEDOWN:
		return K_PGDN;
	case vgui::KEY_BREAK:
		return K_PAUSE; 
	case vgui::KEY_LSHIFT:
		return K_SHIFT;
	case vgui::KEY_RSHIFT:
		return K_SHIFT;
	case vgui::KEY_LALT:
		return K_ALT;
	case vgui::KEY_RALT:
		return K_ALT;
	case vgui::KEY_LCONTROL:
		return K_CTRL;
	case vgui::KEY_RCONTROL:
		return K_CTRL;
	case vgui::KEY_UP:
		return K_UPARROW;
	case vgui::KEY_LEFT:
		return K_LEFTARROW;
	case vgui::KEY_DOWN:
		return K_DOWNARROW;
	case vgui::KEY_RIGHT:
		return K_RIGHTARROW;
	case vgui::KEY_F1:
		return K_F1;
	case vgui::KEY_F2:
		return K_F2;
	case vgui::KEY_F3:
		return K_F3;
	case vgui::KEY_F4:
		return K_F4;
	case vgui::KEY_F5:
		return K_F5;
	case vgui::KEY_F6:
		return K_F6;
	case vgui::KEY_F7:
		return K_F7;
	case vgui::KEY_F8:
		return K_F8;
	case vgui::KEY_F9:
		return K_F9;
	case vgui::KEY_F10:
		return K_F10;
	case vgui::KEY_F11:
		return K_F11;
	case vgui::KEY_F12:
		return K_F12;

	// Not defined by HL engine
	case vgui::KEY_NUMLOCK:
	case vgui::KEY_LWIN:
	case vgui::KEY_RWIN:
	case vgui::KEY_APP:
	case vgui::KEY_SCROLLLOCK:
	case vgui::KEY_CAPSLOCKTOGGLE:
	case vgui::KEY_NUMLOCKTOGGLE:
	case vgui::KEY_SCROLLLOCKTOGGLE:
		return 0;
	}

	return 0;
};

//-----------------------------------------------------------------------------
// Purpose: Cracks engine usable name from raw ui button flags
// Input  : buttons - bit field of which buttons are being held down ( lower gets precedence for naming )
//-----------------------------------------------------------------------------
static const char *GetButtonName( int buttons )
{
	if ( buttons & 1 )
	{
		return "MOUSE1";
	}
	else if ( buttons & 2 )
	{
		return "MOUSE2";
	}
	else if ( buttons & 4 )
	{
		return "MOUSE3";
	}
	else if ( buttons & 8 )
	{
		return "MOUSE4";
	}
	else if ( buttons & 16 )
	{
		return "MOUSE5";
	}

	return "MOUSE1";
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
COptionsSubKeyboard::COptionsSubKeyboard(vgui::Panel *parent) : PropertyPage(parent, NULL)
{
	memset( m_Bindings, 0, sizeof( m_Bindings ));

	// create the key bindings list
	CreateKeyBindingList();
	// Store all current key bindings
	SaveCurrentBindings();
	// Parse default descriptions
	ParseActionDescriptions();
	
	m_pSetBindingButton = new Button(this, "ChangeKeyButton", "");
	m_pClearBindingButton = new Button(this, "ClearKeyButton", "");

	LoadControlSettings("Resource/OptionsSubKeyboard.res");

	m_pSetBindingButton->SetEnabled(false);
	m_pClearBindingButton->SetEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
COptionsSubKeyboard::~COptionsSubKeyboard()
{
	DeleteSavedBindings();
}

//-----------------------------------------------------------------------------
// Purpose: reloads current keybinding
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::OnResetData()
{
	// debug timing code, this function takes too long
//	double s4 = system()->GetCurrentTime();
	// Populate list based on current settings
	FillInCurrentBindings();
    m_pKeyBindList->SetSelectedItem(0);
//	double s5 = system()->GetCurrentTime();
//	ivgui()->DPrintf("FillInCurrentBindings(): %.3fms\n", (float)(s5 - s4) * 1000.0f);
}

//-----------------------------------------------------------------------------
// Purpose: saves out keybinding changes
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::OnApplyChanges()
{
	ApplyAllBindings();
}

//-----------------------------------------------------------------------------
// Purpose: Create key bindings list control
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::CreateKeyBindingList()
{
	// Create the control
	m_pKeyBindList = new VControlsListPanel(this, "listpanel_keybindlist");
}

//-----------------------------------------------------------------------------
// Purpose: command handler
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::OnCommand( const char *command )
{
	if ( !stricmp( command, "Defaults" )  )
	{
		// open a box asking if we want to restore defaults
		QueryBox *box = new QueryBox("#GameUI_KeyboardSettings", "#GameUI_KeyboardSettingsText");
		box->AddActionSignalTarget(this);
		box->SetOKCommand(new KeyValues("Command", "command", "DefaultsOK"));
		box->DoModal();
	}
	else if ( !stricmp(command, "DefaultsOK"))
	{
		// Restore defaults from default keybindings file
		FillInDefaultBindings();
		m_pKeyBindList->RequestFocus();
	}
	else if ( !m_pKeyBindList->IsCapturing() && !stricmp( command, "ChangeKey" ) )
	{
		m_pKeyBindList->StartCaptureMode(dc_none);
	}
	else if ( !m_pKeyBindList->IsCapturing() && !stricmp( command, "ClearKey" ) )
	{
		OnKeyCodePressed(KEY_DELETE);
        m_pKeyBindList->RequestFocus();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

static char token[ 1024 ];
static char *UTIL_Parse( char *data )
{
	data = engine->COM_ParseFile( data, token );
	return data;
}
static char *UTIL_GetToken( void )
{
	return token;
}
static char *UTIL_CopyString( char const *in )
{
	char *out = new char[ strlen( in ) + 1 ];
	strcpy( out, in );
	return out;
}

char *UTIL_va(char *format, ...)
{
	va_list         argptr;
	static char             string[4][1024];
	static int curstring = 0;
	
	curstring = ( curstring + 1 ) % 4;

	va_start (argptr, format);
	_vsnprintf( string[curstring], 1024, format, argptr );
	va_end (argptr);

	return string[curstring];  
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::ParseActionDescriptions( void )
{
	char szFileName[_MAX_PATH];
	
	char szBinding[256];
	char szDescription[256];

	char *buffer = NULL;
	char *data;

	KeyValues *item;

	// Load the kb_act.lst
	sprintf( szFileName, "%s/kb_act.lst", SCRIPTS_DIR );

	FileHandle_t fh = vgui::filesystem()->Open( szFileName, "rb" );
	if (fh == FILESYSTEM_INVALID_HANDLE)
		return;

	int size = vgui::filesystem()->Size(fh);
	CUtlBuffer buf( 0, size + 1, true );
	vgui::filesystem()->Read( buf.Base(), size, fh );
	vgui::filesystem()->Close(fh);
	
	data = (char *)buf.Base();
	data[size] = 0;

	int sectionIndex = 0;
	while ( 1 )
	{
		data = UTIL_Parse( data );
		// Done.
		if ( strlen( UTIL_GetToken() ) <= 0 )  
			break;

		strcpy( szBinding, UTIL_GetToken() );

		data = UTIL_Parse( data );
		if ( strlen(UTIL_GetToken()) <= 0 )
		{
			break;
		}

		strcpy(szDescription, UTIL_GetToken());

		// Skip '======' rows
		if ( szDescription[ 0 ] != '=' )
		{
			// Flag as special header row if binding is "blank"
			if (!stricmp(szBinding, "blank"))
			{
				// add header item
				m_pKeyBindList->AddSection(++sectionIndex, szDescription);
				m_pKeyBindList->AddColumnToSection(sectionIndex, "Action", szDescription, SectionedListPanel::COLUMN_BRIGHT, 226);
				m_pKeyBindList->AddColumnToSection(sectionIndex, "Key", "#GameUI_KeyButton", SectionedListPanel::COLUMN_BRIGHT, 128);
				m_pKeyBindList->AddColumnToSection(sectionIndex, "AltKey", "#GameUI_Alternate", SectionedListPanel::COLUMN_BRIGHT, 128);
			}
			else
			{
				// Create a new: blank item
				item = new KeyValues( "Item" );
				
				// fill in data
				item->SetString("Action", szDescription);
				item->SetString("Binding", szBinding);
				item->SetString("Key", "");
				item->SetString("AltKey", "");

				// Add to list
				m_pKeyBindList->AddItem(sectionIndex, item);
				item->deleteThis();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Search current data set for item which has the specified binding string
// Input  : *binding - string to find
// Output : KeyValues or NULL on failure
//-----------------------------------------------------------------------------
KeyValues *COptionsSubKeyboard::GetItemForBinding( const char *binding )
{
	static int bindingSymbol = KeyValuesSystem()->GetSymbolForString("Binding");

	// Loop through all items
	for (int i = 0; i < m_pKeyBindList->GetItemCount(); i++)
	{
		KeyValues *item = m_pKeyBindList->GetItemData(m_pKeyBindList->GetItemIDFromRow(i));
		if ( !item )
			continue;

		KeyValues *bindingItem = item->FindKey(bindingSymbol);
		const char *bindString = bindingItem->GetString();

		// Check the "Binding" key
		if (!stricmp(bindString, binding))
			return item;
	}
	// Didn't find it
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Retrieve the bindable key name for an ascii index
//-----------------------------------------------------------------------------
const char *COptionsSubKeyboard::GetKeyName( int keynum )
{
	return gameuifuncs->Key_NameForKey( keynum );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int COptionsSubKeyboard::FindKeyForName( char const *keyname )
{
	for ( int i = 0; i < 256; i++ )
	{
		char const *name = GetKeyName( i );
		if ( !name || !name[ 0 ] )
			continue;

		// if it's one character - use case sensitive
		if (strlen(keyname) == 1)
		{
			if (!strcmp( keyname, name ) )
				return i;
		}
		else	// more than one character, use case insensitive
		{
			if ( !stricmp( keyname, name ) )
				return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Bind the specified keyname to the specified item
// Input  : *item - Item to which to add the key
//			*keyname - The key to be added
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::AddBinding( KeyValues *item, const char *keyname )
{
	// See if it's already there as a primary or alternate binding
	if ( !stricmp( item->GetString( "Key", "" ), keyname ) )
		return;

	// Make sure it doesn't live anywhere
	RemoveKeyFromBindItems( keyname );
	// Copy Key to AltKey
	item->SetString( "AltKey", item->GetString( "Key", "" ) );
	// Set new Key
	item->SetString( "Key", keyname );
}

//-----------------------------------------------------------------------------
// Purpose: Remove all keys from list
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::ClearBindItems( void )
{
	m_KeysToUnbind.RemoveAll();

	for (int i = 0; i < m_pKeyBindList->GetItemCount(); i++)
	{
		KeyValues *item = m_pKeyBindList->GetItemData(m_pKeyBindList->GetItemIDFromRow(i));
		if ( !item )
			continue;

		// Reset keys
		item->SetString( "Key", "" );
		item->SetString( "AltKey", "" );

		m_pKeyBindList->InvalidateItem(i);
	}

	m_pKeyBindList->InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Remove all instances of the specified key from bindings
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::RemoveKeyFromBindItems( const char *key )
{
	assert( key && key[ 0 ] );
	if ( !key || !key[ 0 ] )
		return;

	for (int i = 0; i < m_pKeyBindList->GetItemCount(); i++)
	{
		KeyValues *item = m_pKeyBindList->GetItemData(m_pKeyBindList->GetItemIDFromRow(i));
		if ( !item )
			continue;

		// If it's bound to the alternate: just unbind it and
		//  remove the alternate key
		if ( !stricmp( key, item->GetString( "AltKey", "" ) ) )
		{
			// Tell the item
			item->SetString( "AltKey", "" );
			m_pKeyBindList->InvalidateItem(i);
		}

		// If it's bound to the primary: then remove it and copy
		//  any alternate over to the primary to take its place
		if ( !stricmp( key, item->GetString( "Key", "" ) ) )
		{
			// Clear it out
			item->SetString( "Key", "" );
			m_pKeyBindList->InvalidateItem(i);

			// Is there still an alternate?
			const char *alt = item->GetString( "AltKey", "" );
			// If so: shift it over
			if ( alt && alt[0] )
			{
				// Set the key tot he alternate
				item->SetString( "Key", alt );
				// Reset the alternate
				item->SetString( "AltKey", "" );
			}
		}
	}

	// Make sure the display is up to date
	m_pKeyBindList->InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Ask the engine for all bindings and set up the list
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::FillInCurrentBindings( void )
{
	// Clear any current settings
	ClearBindItems();

	for (int i = 0; i < 256; i++)
	{
		// Look up binding
		const char *binding = gameuifuncs->Key_BindingForKey( i );
		if ( !binding )
			continue;

		// See if there is an item for this one?
		KeyValues *item = GetItemForBinding( binding );
		if ( item )
		{
			// Bind it by name
			const char *keyName = GetKeyName(i);
			AddBinding( item, keyName );

			// remember to apply unbinding of this key when we apply
			m_KeysToUnbind.AddToTail( keyName );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clean up memory used by saved bindings
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::DeleteSavedBindings( void )
{
	for (int i = 0; i < 256; i++)
	{
		if (m_Bindings[ i ].binding)
			delete[] m_Bindings[ i ].binding;
		m_Bindings[ i ].binding = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copy all bindings into save array
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::SaveCurrentBindings( void )
{
    DeleteSavedBindings();
	for (int i = 0; i < 256; i++)
	{
		const char *binding = gameuifuncs->Key_BindingForKey( i );
		if ( !binding || !binding[0])
			continue;

		// Copy the binding string
		m_Bindings[ i ].binding = UTIL_CopyString( binding );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tells the engine to bind a key
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::BindKey( const char *key, const char *binding )
{
	engine->ClientCmd( UTIL_va( "bind \"%s\" \"%s\"\n", key, binding ) );
}

//-----------------------------------------------------------------------------
// Purpose: Tells the engine to unbind a key
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::UnbindKey( const char *key )
{
	engine->ClientCmd( UTIL_va( "unbind \"%s\"\n", key ) );
}

//-----------------------------------------------------------------------------
// Purpose: Go through list and bind specified keys to actions
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::ApplyAllBindings( void )
{
	// unbind everything that the user unbound
	{for (int i = 0; i < m_KeysToUnbind.Count(); i++)
	{
		UnbindKey(m_KeysToUnbind[i].String());
	}}
	m_KeysToUnbind.RemoveAll();

	// free binding memory
    DeleteSavedBindings();

	for (int i = 0; i < m_pKeyBindList->GetItemCount(); i++)
	{
		KeyValues *item = m_pKeyBindList->GetItemData(m_pKeyBindList->GetItemIDFromRow(i));
		if ( !item )
			continue;

		// See if it has a binding
		const char *binding = item->GetString( "Binding", "" );
		if ( !binding || !binding[ 0 ] )
			continue;

		const char *keyname;
		
		// Check main binding
		keyname = item->GetString( "Key", "" );
		if ( keyname && keyname[ 0 ] )
		{
			// Tell the engine
			BindKey( keyname, binding );
            int bindIndex = FindKeyForName( keyname );
            if (bindIndex != -1)
				m_Bindings[ bindIndex ].binding = UTIL_CopyString( binding );
		}

		// Check alternate binding
		keyname = item->GetString( "AltKey", "" );
		if ( keyname && keyname[ 0 ] )
		{
			// Tell the engine
			BindKey( keyname, binding );
            int bindIndex = FindKeyForName( keyname );
			if (bindIndex != -1)
                m_Bindings[ bindIndex ].binding = UTIL_CopyString( binding );
		}
	}

	// Now exec their custom bindings
	engine->ClientCmd( "exec userconfig.cfg\nhost_writeconfig\n" );
}

//-----------------------------------------------------------------------------
// Purpose: Read in defaults from game's kb_def.lst file and populate list 
//			using those defaults
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::FillInDefaultBindings( void )
{
	char szFileName[_MAX_PATH];
	char *buffer = NULL;
	char *data;
	char szKeyName[256];
	char szBinding[256];
	
	// Read in the defaults list.
	sprintf( szFileName, "%s/kb_def.lst", SCRIPTS_DIR );

	FileHandle_t fh = vgui::filesystem()->Open( szFileName, "rb" );
	if (fh == FILESYSTEM_INVALID_HANDLE)
		return;

	int size = vgui::filesystem()->Size(fh);
	CUtlBuffer buf( 0, size, true );
	vgui::filesystem()->Read( buf.Base(), size, fh );
	vgui::filesystem()->Close(fh);

	// Clear out all current bindings
	ClearBindItems();

	data = (char*)buf.Base();

	KeyValues *item;
	while ( 1 )
	{
		data = UTIL_Parse( data );
		if ( strlen( UTIL_GetToken() ) <= 0 )
			break;

		// Key name
		strcpy ( szKeyName, UTIL_GetToken() );

		data = UTIL_Parse( data );
		if ( strlen( UTIL_GetToken() ) <= 0 )  // Error
			break;
			
		// Key binding
		strcpy( szBinding, UTIL_GetToken() );

		// Find item
		item = GetItemForBinding( szBinding );
		if ( item )
		{
			// Bind it
			AddBinding( item, szKeyName );
		}
	}
	
	PostActionSignal(new KeyValues("ApplyButtonEnable"));

	// Make sure console and escape key are always valid
    item = GetItemForBinding( "toggleconsole" );
    if (item)
    {
        // Bind it
        AddBinding( item, "`" );
    }
    item = GetItemForBinding( "cancelselect" );
    if (item)
    {
        // Bind it
        AddBinding( item, "ESCAPE" );
    }
}

//-----------------------------------------------------------------------------
// Purpose: User clicked on item: remember where last active row/column was
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::ItemSelected(int itemID)
{
	m_pKeyBindList->SetItemOfInterest(itemID);

	if (m_pKeyBindList->IsItemIDValid(itemID))
	{
		// find the details, see if we should be enabled/clear/whatever
		m_pSetBindingButton->SetEnabled(true);

		KeyValues *kv = m_pKeyBindList->GetItemData(itemID);
		if (kv)
		{
			const char *key = kv->GetString("Key", NULL);
			if (key && *key)
			{
				m_pClearBindingButton->SetEnabled(true);
			}
			else
			{
				m_pClearBindingButton->SetEnabled(false);
			}

			if (kv->GetInt("Header"))
			{
				m_pSetBindingButton->SetEnabled(false);
			}
		}
	}
	else
	{
		m_pSetBindingButton->SetEnabled(false);
		m_pClearBindingButton->SetEnabled(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: called when the capture has finished
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::Finish(int key, int button)
{
	int r = m_pKeyBindList->GetItemOfInterest();

	// Retrieve clicked row and column
	m_pKeyBindList->EndCaptureMode(dc_arrow);

	// Find item for this row
	KeyValues *item = m_pKeyBindList->GetItemData(r);
	if ( item )
	{
		if ( button != 0 )
		{
			AddBinding( item, GetButtonName( button ) );
			PostActionSignal(new KeyValues("ApplyButtonEnable"));	
		}
		
		// Handle keys: but never rebind the escape key
		// Esc just exits bind mode silently
		else if ( key != 0 && key != 27 )
		{
			// Bind the named key
			AddBinding( item, GetKeyName( key ) );
			PostActionSignal(new KeyValues("ApplyButtonEnable"));	
		}

		m_pKeyBindList->InvalidateItem(r);
	}

	m_pSetBindingButton->SetEnabled(true);
	m_pClearBindingButton->SetEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: key input handler
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( m_pKeyBindList->IsCapturing() )
	{
		int key = ConvertVGUIToEngine( code );
		Finish( key, 0 );
		return;
	}

	BaseClass::OnKeyCodeTyped( code );
}

//-----------------------------------------------------------------------------
// Purpose: key input handler
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::OnKeyTyped(wchar_t unichar)
{
	BaseClass::OnKeyTyped( unichar );
}

//-----------------------------------------------------------------------------
// Purpose: mouse input handler
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::OnMousePressed(vgui::MouseCode code)
{
	if ( m_pKeyBindList->IsCapturing() )
	{
		switch ( code )
		{
		default:
		case vgui::MOUSE_LEFT:
			Finish( 0, 0x01 );
			break;
		case vgui::MOUSE_RIGHT:
			Finish( 0, 0x02 );
			break;
		case vgui::MOUSE_MIDDLE:
			Finish( 0, 0x04 );
			break;
		case vgui::MOUSE_4:
			Finish( 0, 0x08 );
			break;
		case vgui::MOUSE_5:
			Finish( 0, 0x10 );
			break;
		}
		return;
	}

	BaseClass::OnMousePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: sets up mouse capture mode
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::OnMouseDoublePressed(vgui::MouseCode code)
{
	if ( m_pKeyBindList->IsCapturing() )
	{
		switch ( code )
		{
		default:
		case vgui::MOUSE_LEFT:
			Finish( 0, 0x01 );
			break;
		case vgui::MOUSE_RIGHT:
			Finish( 0, 0x02 );
			break;
		case vgui::MOUSE_MIDDLE:
			Finish( 0, 0x04 );
			break;
		case vgui::MOUSE_4:
			Finish( 0, 0x08 );
			break;
		case vgui::MOUSE_5:
			Finish( 0, 0x10 );
			break;
		}
		return;
	}

	BaseClass::OnMouseDoublePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: captures mouse input
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::OnMouseWheeled(int delta)
{
	if (m_pKeyBindList->IsCapturing())
	{
		if ( delta > 0 )
		{
			Finish(FindKeyForName("MWHEELUP"),  0);
		}
		else
		{
			Finish(FindKeyForName("MWHEELDOWN"), 0);
		}
		return;
	}

	BaseClass::OnMouseWheeled(delta);
}

//-----------------------------------------------------------------------------
// Purpose: Check for enter key and go into keybinding mode if it was pressed
//-----------------------------------------------------------------------------
void COptionsSubKeyboard::OnKeyCodePressed(vgui::KeyCode code)
{
	// Enter key pressed and not already trapping next key/button press
	if (!m_pKeyBindList->IsCapturing())
	{
		// Grab which item was set as interesting
		int r = m_pKeyBindList->GetItemOfInterest();

		// Check that it's visible
		int x, y, w, h;
		bool visible = m_pKeyBindList->GetCellBounds(r, 1, x, y, w, h);
		if (visible)
		{
			/*
			if (vgui::KEY_ENTER == code)
			{
				// start editing
				PostMessage(this, new KeyValues("Command", "command", "ChangeKey"));
				// message handled, don't pass on
				return;
			}
			else */if (vgui::KEY_DELETE == code)
			{
				// find the current binding and remove it
				KeyValues *kv = m_pKeyBindList->GetItemData(r);

				const char *altkey = kv->GetString("AltKey", NULL);
				if (altkey && *altkey)
				{
					RemoveKeyFromBindItems(altkey);
				}

				const char *key = kv->GetString("Key", NULL);
				if (key && *key)
				{
					RemoveKeyFromBindItems(key);
				}

				m_pClearBindingButton->SetEnabled(false);
				m_pKeyBindList->InvalidateItem(r);

				// message handled, don't pass on
				return;
			}
		}
	}

	// Allow base class to process message instead
	BaseClass::OnKeyCodePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
vgui::MessageMapItem_t COptionsSubKeyboard::m_MessageMap[] =
{
	MAP_MESSAGE_INT( COptionsSubKeyboard, "ItemSelected", ItemSelected, "itemID" ),
};
IMPLEMENT_PANELMAP(COptionsSubKeyboard,BaseClass);