// Client-side CBasePlayer

#ifndef C_XX_H
#define C_XX_H
#pragma once

class C_BaseAnimatingOverlay : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_BaseAnimatingOverlay, C_BaseAnimating );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();


	C_BaseAnimatingOverlay();

	// model specific
	virtual	void	StandardBlendingRules( Vector pos[], Quaternion q[], float currentTime, int boneMask );

	enum
	{
		MAX_OVERLAYS = 8,
	};

	AnimationLayer_t	m_Layer[ MAX_OVERLAYS ];
	CInterpolatedVarArray< AnimationLayer_t, MAX_OVERLAYS >	m_iv_Layer;

private:
	C_BaseAnimatingOverlay( const C_BaseAnimatingOverlay & ); // not defined, not accessible
};


EXTERN_RECV_TABLE(DT_BaseAnimatingOverlay);


#endif // C_XX_H




