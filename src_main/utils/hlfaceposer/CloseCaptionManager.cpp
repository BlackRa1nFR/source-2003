//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "choreowidgetdrawhelper.h"
#include "sentence.h"
#include "hlfaceposer.h"
#include "iclosecaptionmanager.h"
#include "vstdlib/strtools.h"

extern double realtime;


#define STREAM_FONT			"Arial Unicode MS"
#define STREAM_POINTSIZE	11
#define STREAM_WEIGHT		FW_NORMAL

#define CAPTION_LINGER_TIME	2.0f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCloseCaptionItem
{
public:
	CCloseCaptionItem( 
		StudioModel *actor, 
		CSentence *sentence, 
		CCloseCaptionPhrase *phrase,
		float curtime, 
		int frame, 
		double updatetime )
	{
		SetActor( actor );
		m_pPhrase = NULL;
		SetPhrase( phrase );
		SetSentence( sentence );
		SetSentenceTime( curtime );
		SetLastUpdateFrame( frame );
		SetLastUpdateTime( updatetime );
	}

	CCloseCaptionItem( const CCloseCaptionItem& src )
	{
		m_pActor = src.m_pActor;
		m_pSentence = src.m_pSentence;

		SetPhrase( src.GetPhrase() );

		m_nLastUpdateFrame = src.m_nLastUpdateFrame;
		m_flUpdateTime = src.m_flUpdateTime;
		m_flSentenceTime = src.m_flSentenceTime;
		m_flLingerTime = src.m_flLingerTime;
	}

	~CCloseCaptionItem( void )
	{
		delete m_pPhrase;
	}

	void SetPhrase( CCloseCaptionPhrase const *phrase )
	{
		m_flLingerTime = CAPTION_LINGER_TIME;
		delete m_pPhrase;
		if ( !phrase )
		{
			m_pPhrase = NULL;
		}
		else
		{
			// Copy it
			m_pPhrase = new CCloseCaptionPhrase( *phrase );

			// Check for lingering
			int tokenCount = m_pPhrase->CountTokens();
			for ( int i = 0; i < tokenCount; i++ )
			{
				wchar_t const *token = m_pPhrase->GetToken( i );

				if ( !token )
					continue;

				wchar_t cmd[ 256 ];
				wchar_t args[ 256 ];

				if ( !m_pPhrase->SplitCommand( token, cmd, args ) )
					continue;

				if ( wcscmp( cmd, L"linger" ) )
					continue;

				char converted[ 128 ];
				ConvertUnicodeToANSI( args, converted, sizeof( converted ) );

				float linger_time = (float)Q_atof( converted );

				if ( linger_time > m_flLingerTime )
				{
					m_flLingerTime = linger_time;
				}
			}
		}
	}

	const CCloseCaptionPhrase *GetPhrase( void ) const
	{
		return m_pPhrase;
	}

	const StudioModel *GetActor( void ) const
	{
		return m_pActor;
	}
	void SetActor( const StudioModel *actor )
	{
		m_pActor = actor;
	}

	void SetSentence( const CSentence *sentence )
	{
		m_pSentence = sentence;
	}

	CSentence const *GetSentence( void ) const
	{
		return m_pSentence;
	}

	void SetLastUpdateFrame( int frame )
	{
		m_nLastUpdateFrame = frame;
	}

	int GetLastUpdateFrame( void ) const
	{
		return m_nLastUpdateFrame;
	}

	void SetLastUpdateTime( double t )
	{
		m_flUpdateTime = t;
	}

	double GetLastUpdateTime( void ) const
	{
		return m_flUpdateTime;
	}

	void SetSentenceTime( float t )
	{
		m_flSentenceTime = t;
	}

	float GetSentenceTime( void ) const
	{
		return m_flSentenceTime;
	}

	float GetLingerTime( void ) const
	{
		return m_flLingerTime;
	}

	void SetLingerTime( float t )
	{
		m_flLingerTime = t;
	}

private:
	int					m_nLastUpdateFrame;
	double				m_flUpdateTime;
	float				m_flSentenceTime;
	float				m_flLingerTime;

	CCloseCaptionPhrase	*m_pPhrase;	

	CSentence const		*m_pSentence;
	StudioModel	const	*m_pActor;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCloseCaptionManager : public ICloseCaptionManager
{
public:
	CCloseCaptionManager() 
	{
		fntNormal = CreateFont(
			-STREAM_POINTSIZE, 
			0,
			0,
			0,
			STREAM_WEIGHT,
			FALSE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY,
			DEFAULT_PITCH,
			STREAM_FONT );

		fntItalic = CreateFont(
			-STREAM_POINTSIZE, 
			0,
			0,
			0,
			STREAM_WEIGHT,
			TRUE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY,
			DEFAULT_PITCH,
			STREAM_FONT );
		
		fntBold = CreateFont(
			-STREAM_POINTSIZE, 
			0,
			0,
			0,
			900,
			FALSE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY,
			DEFAULT_PITCH,
			STREAM_FONT );

		fntBoldItalic = CreateFont(
			-STREAM_POINTSIZE, 
			0,
			0,
			0,
			700,
			TRUE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY,
			DEFAULT_PITCH,
			STREAM_FONT );
	}

	~CCloseCaptionManager()
	{
		DeleteObject( fntItalic );
		DeleteObject( fntBold );
		DeleteObject( fntBoldItalic );
		DeleteObject( fntNormal );
	}

	virtual void Reset( void );

	virtual void PreProcess( int framecount );

	virtual void Process( int framecount, StudioModel *model, float curtime, CSentence* sentence );
	
	virtual void PostProcess( int framecount, float frametime );
	
	virtual void Draw( CChoreoWidgetDrawHelper &helper, RECT &rcOutput );

private:
	void	DrawStream( CChoreoWidgetDrawHelper &helper, RECT &rcText, CCloseCaptionPhrase const *phrase ); 
	int		ComputeStreamHeight( CChoreoWidgetDrawHelper &helper, int available_width, CCloseCaptionPhrase const *phrase );

	CUtlVector< CCloseCaptionItem * > m_Items;

	HFONT fntItalic;
	HFONT fntBold;
	HFONT fntBoldItalic;
	HFONT fntNormal;
};

static CCloseCaptionManager g_CloseCaptionManager;
ICloseCaptionManager *closecaptionmanager = &g_CloseCaptionManager;

void CCloseCaptionManager::Reset( void )
{
	while ( m_Items.Count() > 0 )
	{
		CCloseCaptionItem *i = m_Items[ 0 ];
		delete i;
		m_Items.Remove( 0 );
	}
}

void CCloseCaptionManager::PreProcess( int framecount )
{
}

void CCloseCaptionManager::Process( int framecount, StudioModel *model, float curtime, CSentence* sentence )
{
	int i;
	int c = m_Items.Count();
	bool found = false;
	CCloseCaptionItem *item = NULL;

	for ( i = 0; i < c; i++ )
	{
		item = m_Items[ i ];
		Assert( item );
		if ( item->GetSentence() != sentence )
			continue;

		found = true;
		item->SetSentenceTime( curtime );
		item->SetLastUpdateTime( realtime );
		item->SetLastUpdateFrame( framecount );
	}

	if ( found )
		return;

	// Create new entries
	c = sentence->GetCloseCaptionPhraseCount( CC_ENGLISH );
	for ( i = 0; i < c; i++ )
	{	
		CCloseCaptionPhrase *phrase = sentence->GetCloseCaptionPhrase( CC_ENGLISH, i );
		Assert( phrase );
		item = new CCloseCaptionItem( 
			model, 
			sentence, 
			phrase, 
			curtime, 
			framecount, 
			realtime );
		m_Items.AddToTail( item );
	}
}

void CCloseCaptionManager::PostProcess( int framecount, float frametime )
{
	int c = m_Items.Count();
	int i;
	for  ( i = c - 1; i >= 0; i-- )
	{
		CCloseCaptionItem *item = m_Items[ i ];
		if ( item->GetLastUpdateFrame() == framecount )
			continue;

		// See if it's a lingerer
		float lt = item->GetLingerTime();
		
		lt -= frametime;

		if ( lt > 0.0f )
		{
			item->SetLingerTime( lt );
			continue;
		}

		delete item;
		m_Items.Remove( i );
	}
}

struct VisibleStreamItem
{
	int					height;
	CCloseCaptionItem	*item;
};

void CCloseCaptionManager::Draw( CChoreoWidgetDrawHelper &helper, RECT &rcOutput )
{
	RECT rcText = rcOutput;
	helper.DrawFilledRect( RGB( 0, 0, 0 ), rcText );
	helper.DrawOutlinedRect( RGB( 200, 245, 150 ), PS_SOLID, 2, rcText );
	InflateRect( &rcText, -4, 0 );

	int avail_width = rcText.right - rcText.left;

	int totalheight = 0;
	int i;
	CUtlVector< VisibleStreamItem > visibleitems;
	int c = m_Items.Count();
	for  ( i = 0; i < c; i++ )
	{
		CCloseCaptionItem *item = m_Items[ i ];
		float curtime = item->GetSentenceTime();
		float start = item->GetPhrase()->GetStartTime();

		if ( curtime < start )
			continue;

		float end = item->GetPhrase()->GetEndTime();

		if ( curtime > end )
		{
			// Check lingering
			if ( curtime > end + item->GetLingerTime() )
				continue;
		}

		int itemheight = ComputeStreamHeight( helper, avail_width, item->GetPhrase() );

		totalheight += itemheight;

		VisibleStreamItem si;
		si.height = itemheight;
		si.item = item;

		visibleitems.AddToTail( si );
	}



	rcText.bottom -= 2;
	rcText.top = rcText.bottom - totalheight;

	// Now draw them
	c = visibleitems.Count();
	for ( i = 0; i < c; i++ )
	{
		VisibleStreamItem *si = &visibleitems[ i ];

		int height = si->height;
		CCloseCaptionItem *item = si->item;

		rcText.bottom = rcText.top + height;

		DrawStream( helper, rcText, item->GetPhrase() );

		OffsetRect( &rcText, 0, height );

		if ( rcText.top >= rcOutput.bottom )
			break;
	}
}

int	CCloseCaptionManager::ComputeStreamHeight( CChoreoWidgetDrawHelper &helper, int available_width, CCloseCaptionPhrase const *phrase )
{
	int c = phrase->CountTokens();
	int i;

	int height = STREAM_POINTSIZE + 1;
	int usedw = 0;

	int italic = 0;
	int bold = 0;

	for ( i = 0; i < c; i++ )
	{
		wchar_t const *token = phrase->GetToken( i );
		if ( !token )
			continue;

		wchar_t cmd[ 256 ];
		wchar_t args[ 256 ];

		if ( phrase->SplitCommand( token, cmd, args ) )
		{
			if ( !wcscmp( cmd, L"cr" ) )
			{
				usedw = 0;
				height += ( STREAM_POINTSIZE + 1 );
			}

			if ( !wcscmp( cmd, L"I" ) )
			{
				italic = !italic;
			}

			if ( !wcscmp( cmd, L"B" ) )
			{
				bold = !bold;
			}

			continue;
		}

		int w;
	
		HFONT useF = fntNormal;
		
		if ( bold || italic )
		{
			if ( bold && italic )
			{ 
				useF = fntBoldItalic;
			}
			else if ( bold )
			{
				useF = fntBold;
			}
			else if ( italic )
			{
				useF = fntItalic;
			}
			else
			{
				Assert( 0 );
			}
		}

		w = helper.CalcTextWidthW( useF, L"%s ", token );

		if ( usedw + w > available_width )
		{
			usedw = 0;
			height += ( STREAM_POINTSIZE + 1 );
		}
		usedw += w;
	}

	return height;
}

void CCloseCaptionManager::DrawStream( CChoreoWidgetDrawHelper &helper, RECT &rcText, CCloseCaptionPhrase const *phrase )
{
	int c = phrase->CountTokens();
	int i;

	int height = STREAM_POINTSIZE + 1;
	int x = 0;
	int y = 0;

	int available_width = rcText.right - rcText.left;

	COLORREF clr = RGB( 255, 255, 255 );

	CUtlVector< COLORREF > colorStack;
	colorStack.AddToTail( clr );

	int italic = 0;
	int bold = 0;

	for ( i = 0; i < c; i++ )
	{
		wchar_t const *token = phrase->GetToken( i );
		if ( !token )
			continue;

		wchar_t cmd[ 256 ];
		wchar_t args[ 256 ];

		if ( phrase->SplitCommand( token, cmd, args ) )
		{
			if ( !wcscmp( cmd, L"cr" ) )
			{
				x = 0;
				y += ( STREAM_POINTSIZE + 1 );
			}

			if ( !wcscmp( cmd, L"clr" ) )
			{
				if ( args[0] == 0 && colorStack.Count()>= 2)
				{
					colorStack.Remove( colorStack.Count() - 1 );
				}
				else
				{
					int r, g, b;
					COLORREF newcolor;
					if ( 3 == swscanf( args, L"%i,%i,%i", &r, &g, &b ) )
					{
						newcolor = RGB( r, g, b );
						colorStack.AddToTail( newcolor );
					}
				}
			}

			if ( !wcscmp( cmd, L"I" ) )
			{
				italic = !italic;
			}

			if ( !wcscmp( cmd, L"B" ) )
			{
				bold = !bold;
			}

			continue;
		}

		int w;

		HFONT useF = fntNormal;

		if ( bold || italic )
		{
			if ( bold && italic )
			{ 
				useF = fntBoldItalic;
			}
			else if ( bold )
			{
				useF = fntBold;
			}
			else if ( italic )
			{
				useF = fntItalic;
			}
			else
			{
				Assert( 0 );
			}
		}

		w = helper.CalcTextWidthW( useF, L"%s ", token );

		if ( x + w > available_width )
		{
			x = 0;
			y += ( STREAM_POINTSIZE + 1 );
		}

		RECT rcOut;
		rcOut.left = rcText.left + x;
		rcOut.right = rcOut.left + w;
		rcOut.top = rcText.top + y;
		rcOut.bottom = rcOut.top + STREAM_POINTSIZE + 1;

		COLORREF useColor = colorStack[ colorStack.Count() - 1 ];

		helper.DrawColoredTextW( useF, useColor,
				rcOut, L"%s ", token );

		x += w;
	}
}
