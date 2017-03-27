#include "cbase.h"
#include "actanimating.h"
#include "animation.h"

BEGIN_DATADESC( CActAnimating )
	DEFINE_FIELD( CActAnimating, m_Activity, FIELD_INTEGER ),
END_DATADESC()


void CActAnimating :: SetActivity( Activity act ) 
{ 
	int sequence = SelectWeightedSequence( act ); 
	if ( sequence != ACTIVITY_NOT_AVAILABLE )
	{
		ResetSequence( sequence );
		m_Activity = act; 
		m_flCycle = 0;
	}
}

