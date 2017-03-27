//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: HL2 specific input handling
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "kbutton.h"
#include "input.h"

//-----------------------------------------------------------------------------
// Purpose: HL Input interface
//-----------------------------------------------------------------------------
class CHLInput : public CInput
{
public:
};

static CHLInput g_Input;

// Expose this interface
IInput *input = ( IInput * )&g_Input;
