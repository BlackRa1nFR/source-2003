//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_breakableprop.h"

class C_DynamicProp : public C_BreakableProp
{
	DECLARE_CLASS( C_DynamicProp, C_BreakableProp );
public:
	DECLARE_CLIENTCLASS();

	// constructor, destructor
	C_DynamicProp( void );
	~C_DynamicProp( void );

private:
	C_DynamicProp( const C_DynamicProp & );
};

IMPLEMENT_CLIENTCLASS_DT(C_DynamicProp, DT_DynamicProp, CDynamicProp)
END_RECV_TABLE()

C_DynamicProp::C_DynamicProp( void )
{
}

C_DynamicProp::~C_DynamicProp( void )
{
}


