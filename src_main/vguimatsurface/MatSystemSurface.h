//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MATSYSTEMSURFACE_H
#define MATSYSTEMSURFACE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
#include <vgui/IClientPanel.h>
#include <vgui_controls/Panel.h>
#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>
#include <vgui/Point.h>
#include "materialsystem/IMaterialSystem.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/IMesh.h"
#include "materialsystem/IMaterial.h"
#include "UtlVector.h"
#include "UtlSymbol.h"

using namespace vgui;

extern class IMaterialSystem *g_pMaterialSystem;
class HtmlWindow;
//-----------------------------------------------------------------------------
// The default material system embedded panel
//-----------------------------------------------------------------------------
class CMatEmbeddedPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
public:
	CMatEmbeddedPanel();
	virtual void OnThink();

	VPANEL IsWithinTraverse(int x, int y, bool traversePopups);
};

//-----------------------------------------------------------------------------
//
// Implementation of the VGUI surface on top of the material system
//
//-----------------------------------------------------------------------------
class CMatSystemSurface : public IMatSystemSurface
{
public:
	CMatSystemSurface();
	virtual ~CMatSystemSurface();

	// Methods of IAppSystem
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();
	virtual void *QueryInterface( const char *pInterfaceName );
	virtual InitReturnVal_t Init();
	virtual void Shutdown();

	// initialization
	virtual void SetEmbeddedPanel(vgui::VPANEL pEmbeddedPanel);

	// returns true if a panel is minimzed
	bool IsMinimized(vgui::VPANEL panel);

	// Sets the only panel to draw.  Set to NULL to clear.
	void RestrictPaintToSinglePanel(vgui::VPANEL panel);

	// frame
	virtual void RunFrame();

	// implementation of vgui::ISurface
	virtual vgui::VPANEL GetEmbeddedPanel();
	
	// drawing context
	virtual void PushMakeCurrent(vgui::VPANEL panel,bool useInSets);
	virtual void PopMakeCurrent(vgui::VPANEL panel);

	// rendering functions
	virtual void DrawSetColor(int r,int g,int b,int a);
	virtual void DrawSetColor(Color col);
	
	virtual void DrawLine( int x0, int y0, int x1, int y1 );
	virtual void DrawTexturedLine( const vgui::Vertex_t &a, const vgui::Vertex_t &b );
	virtual void DrawPolyLine(int *px, int *py, int numPoints);
	virtual void DrawTexturedPolyLine( const vgui::Vertex_t *p, int n );

	virtual void DrawFilledRect(int x0, int y0, int x1, int y1);
	virtual void DrawOutlinedRect(int x0, int y0, int x1, int y1);
	virtual void DrawOutlinedCircle(int x, int y, int radius, int segments);

	// textured rendering functions
	virtual int CreateNewTextureID( bool procedural = false );
	virtual bool IsTextureIDValid(int id);

	virtual bool DrawGetTextureFile(int id, char *filename, int maxlen );
	virtual int	 DrawGetTextureId( char const *filename );
	virtual void DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload);
	virtual void DrawSetTexture(int id);
	virtual void DrawGetTextureSize(int id, int &wide, int &tall);

	virtual void DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload);

	virtual void DrawTexturedRect(int x0, int y0, int x1, int y1);
	virtual void DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 );

	virtual void DrawTexturedPolygon(int n, vgui::Vertex_t *pVertices);

	virtual void DrawPrintText(const wchar_t *text, int textLen, FontDrawType_t drawType = FONT_DRAW_DEFAULT);
	virtual void DrawUnicodeChar(wchar_t wch, FontDrawType_t drawType = FONT_DRAW_DEFAULT );
	virtual void DrawSetTextFont(vgui::HFont font);
	virtual void DrawFlushText();

	virtual void DrawSetTextColor(int r, int g, int b, int a);
	virtual void DrawSetTextColor(Color col);
	virtual void DrawSetTextPos(int x, int y);
	virtual void DrawGetTextPos(int& x,int& y);

	virtual vgui::IHTML *CreateHTMLWindow(vgui::IHTMLEvents *events,vgui::VPANEL context);
	virtual void PaintHTMLWindow(vgui::IHTML *htmlwin);
	virtual void DeleteHTMLWindow(vgui::IHTML *htmlwin);


	virtual void GetScreenSize(int &wide, int &tall);
	virtual void SetAsTopMost(vgui::VPANEL panel, bool state);
	virtual void BringToFront(vgui::VPANEL panel);
	virtual void SetForegroundWindow (vgui::VPANEL panel);
	virtual void SetPanelVisible(vgui::VPANEL panel, bool state);
	virtual void SetMinimized(vgui::VPANEL panel, bool state);
	virtual void FlashWindow(vgui::VPANEL panel, bool state);
	virtual void SetTitle(vgui::VPANEL panel, const wchar_t *title);
	virtual const wchar_t *GetTitle( vgui::VPANEL panel );

	virtual void SetAsToolBar(vgui::VPANEL panel, bool state);		// removes the window's task bar entry (for context menu's, etc.)

	// windows stuff
	virtual void CreatePopup(VPANEL panel, bool minimized, bool showTaskbarIcon = true, bool disabled = false, bool mouseInput = true , bool kbInput = true);

	virtual void SwapBuffers(vgui::VPANEL panel);
	virtual void Invalidate(vgui::VPANEL panel);

	virtual void SetCursor(vgui::HCursor cursor);
	virtual bool IsCursorVisible();

	virtual void ApplyChanges();
	virtual bool IsWithin(int x, int y);
	virtual bool HasFocus();

	virtual bool SupportsFeature(SurfaceFeature_e feature);

	// engine-only focus handling (replacing WM_FOCUS windows handling)
	virtual void SetTopLevelFocus(vgui::VPANEL panel);

	// fonts
	virtual vgui::HFont CreateFont();
	virtual bool AddGlyphSetToFont(vgui::HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int lowRange, int highRange);
	virtual int GetFontTall(HFont font);
	virtual bool IsFontAdditive(HFont font);
	virtual void GetCharABCwide(HFont font, int ch, int &a, int &b, int &c);
	virtual void GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall);
	virtual int GetCharacterWidth(vgui::HFont font, int ch);
	virtual bool AddCustomFontFile(const char *fontFileName);

	// GameUI-only accessed functions
	// uploads a part of a texture, used for font rendering
	void DrawSetSubTextureRGBA(int textureID, int drawX, int drawY, unsigned const char *rgba, int subTextureWide, int subTextureTall);

	// helpers for web browser painting
	int GetHTMLWindowCount() { return _htmlWindows.Count(); }
	HtmlWindow *GetHTMLWindow(int i) { return _htmlWindows[i]; }


	// notify icons?!?
	virtual vgui::VPANEL GetNotifyPanel();
	virtual void SetNotifyIcon(vgui::VPANEL context, vgui::HTexture icon, vgui::VPANEL panelToReceiveMessages, const char *text);

	// plays a sound
	virtual void PlaySound(const char *fileName);

	//!! these functions Should not be accessed directly, but only through other vgui items
	//!! need to move these to seperate interface
	virtual int GetPopupCount();
	virtual vgui::VPANEL GetPopup( int index );
	virtual bool ShouldPaintChildPanel(vgui::VPANEL childPanel);
	virtual bool RecreateContext(vgui::VPANEL panel);
	virtual void AddPanel(vgui::VPANEL panel);
	virtual void ReleasePanel(vgui::VPANEL panel);
	virtual void MovePopupToFront(vgui::VPANEL panel);

	virtual void SolveTraverse(vgui::VPANEL panel, bool forceApplySchemeSettings);
	virtual void PaintTraverse(vgui::VPANEL panel);

	virtual void EnableMouseCapture(vgui::VPANEL panel, bool state);

	virtual void SetWorkspaceInsets( int left, int top, int right, int bottom );
	virtual void GetWorkspaceBounds(int &x, int &y, int &wide, int &tall);

	// Hook needed to Get input to work
	virtual void AttachToWindow( void *hwnd, bool bLetAppDriveInput );
	virtual void HandleWindowMessage( void *hwnd, unsigned int uMsg, unsigned int wParam, long lParam );

	// Begins, ends 3D painting
	virtual void Begin3DPaint( int iLeft, int iTop, int iRight, int iBottom );
	virtual void End3DPaint();

	// Disable clipping during rendering
	virtual void DisableClipping( bool bDisable );

	// Prevents vgui from changing the cursor
	virtual bool IsCursorLocked() const;

	// Sets the mouse Get + Set callbacks
	virtual void SetMouseCallbacks( GetMouseCallback_t GetFunc, SetMouseCallback_t SetFunc );

	// Tells the surface to ignore windows messages
	virtual void EnableWindowsMessages( bool bEnable );

	// Installs a function to play sounds
	virtual void InstallPlaySoundFunc( PlaySoundFunc_t soundFunc );

	// Some drawing methods that cannot be accomplished under Win32
	virtual void DrawColoredCircle( int centerx, int centery, float radius, int r, int g, int b, int a );
	virtual int	DrawColoredText( vgui::HFont font, int x, int y, int r, int g, int b, int a, char *fmt, ... );
	virtual void DrawColoredTextRect( vgui::HFont font, int x, int y, int w, int h, int r, int g, int b, int a, char *fmt, ... );
	virtual void DrawTextHeight( vgui::HFont font, int w, int& h, char *fmt, ... );

	// Returns the length in pixels of the text
	virtual int	DrawTextLen( vgui::HFont font, char *fmt, ... );

	// Draws a panel in 3D space. 
	virtual void DrawPanelIn3DSpace( vgui::VPANEL pRootPanel, const VMatrix &panelCenterToWorld, int pw, int ph, float sw, float sh ); 

	// Only visible within vguimatsurface
	void DrawSetTextureMaterial(int id, IMaterial *pMaterial);
	void ReferenceProceduralMaterial( int id, int referenceId, IMaterial *pMaterial );

	// new stuff for Alfreds VGUI2 port!!
	virtual bool InEngine() { return true; }
	void GetProportionalBase( int &width, int &height ) { width = BASE_WIDTH; height = BASE_HEIGHT; }
	virtual bool HasCursorPosFunctions() { return true; }

	virtual void SetModalPanel(VPANEL );
	virtual VPANEL GetModalPanel();
	virtual void UnlockCursor();
	virtual void LockCursor();
	virtual void SetTranslateExtendedKeys(bool state);
	virtual VPANEL GetTopmostPopup();
	virtual void GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall);
	virtual void CalculateMouseVisible();
	virtual bool NeedKBInput();
	virtual void SurfaceGetCursorPos(int &x, int &y);
	virtual void SurfaceSetCursorPos(int x, int y);
	virtual void MovePopupToBack(VPANEL panel);

	virtual bool IsInThink( VPANEL panel); 

	virtual bool DrawGetUnicodeCharRenderInfo( wchar_t ch, CharRenderInfo& info );
	virtual void DrawRenderCharFromInfo( const CharRenderInfo& info );

private:
	void DrawRenderCharInternal( const CharRenderInfo& info );

private:
	enum { BASE_HEIGHT = 480, BASE_WIDTH = 640 };

	struct PaintState_t
	{
		vgui::VPANEL m_pPanel;
		int	m_iTranslateX;
		int m_iTranslateY;
		int	m_iScissorLeft;
		int	m_iScissorRight;
		int	m_iScissorTop;
		int	m_iScissorBottom;
	};

	// material Setting method 
	void InternalSetMaterial( IMaterial *material = NULL );

	// Helper method to initialize vertices (transforms them into screen space too)
	void InitVertex( vgui::Vertex_t &vertex, int x, int y, float u, float v );

	// Draws a quad + quad array 
	void DrawQuad( const vgui::Vertex_t &ul, const vgui::Vertex_t &lr, unsigned char *pColor );
	void DrawQuadArray( int numQuads, vgui::Vertex_t *pVerts, unsigned char *pColor );

	// Necessary to wrap the rendering
	void StartDrawing( void );
	void StartDrawingIn3DSpace( const VMatrix &screenToWorld, int pw, int ph, float sw, float sh );
	void FinishDrawing( void );

	// Sets up a particular painting state...
	void SetupPaintState( const PaintState_t &paintState );

	void ResetPopupList();
	void AddPopup( vgui::VPANEL panel );
	void RemovePopup( vgui::VPANEL panel );
	void AddPopupsToList( vgui::VPANEL panel );

	// Helper for drawing colored text
	int DrawColoredText( vgui::HFont font, int x, int y, int r, int g, int b, int a, char *fmt, va_list argptr );
	void SearchForWordBreak( vgui::HFont font, char *text, int& chars, int& pixels );

	void InternalThinkTraverse(VPANEL panel);
	void InternalSolveTraverse(VPANEL panel);
	void InternalSchemeSettingsTraverse(VPANEL panel, bool forceApplySchemeSettings);

	// handles mouse movement
	void SetCursorPos(int x, int y);
	void GetCursorPos(int &x, int &y);

	void DrawTexturedLineInternal( const Vertex_t &a, const Vertex_t &b );

	// Point Translation for current panel
	int				m_nTranslateX;
	int				m_nTranslateY;

	// The size of the window to draw into
	int				m_pSurfaceExtents[4];

	// Color for drawing all non-text things
	unsigned char	m_DrawColor[4];

	// Color for drawing text
	unsigned char	m_DrawTextColor[4];

	// Location of text rendering
	int				m_pDrawTextPos[2];

	// Meshbuilder used for drawing
	IMesh* m_pMesh;
	CMeshBuilder meshBuilder;

	// White material used for drawing non-textured things
	IMaterial *m_pWhite;

	// Root panel
	vgui::VPANEL m_pEmbeddedPanel;
	vgui::Panel *m_pDefaultEmbeddedPanel;
	vgui::VPANEL m_pRestrictedPanel;

	// List of pop-up panels based on the type enum above (draw order vs last clicked)
	CUtlVector<vgui::HPanel>	m_PopupList;

	// Stack of paint state...
	CUtlVector<	PaintState_t > m_PaintStateStack;

	CUtlVector<HtmlWindow *> _htmlWindows;
	vgui::HFont m_hCurrentFont;
	vgui::HCursor _currentCursor;

	// The currently bound texture
	int m_iBoundTexture;

	// Are we painting in 3D? (namely drawing 3D objects *inside* the vgui panel)
	bool m_bIn3DPaintMode;

	// Are we drawing the vgui panel in the 3D world somewhere?
	bool m_bDrawingIn3DWorld;

	// Are we currently in the think() loop
	bool m_bInThink;
	VPANEL m_CurrentThinkPanel;

	// The attached HWND
	void *m_HWnd;

	// Installed function to play sounds
	PlaySoundFunc_t m_PlaySoundFunc;

	int		m_WorkSpaceInsets[ 4 ];

	class TitleEntry
	{
	public:
		TitleEntry()
		{
			panel = NULL;
			title[ 0 ] = 0;
		}

		vgui::VPANEL panel;
		wchar_t		title[ 128 ];
	};

	CUtlVector< TitleEntry > m_Titles;
	CUtlVector< CUtlSymbol > m_CustomFontFileNames;

	bool _needKB;
	bool _needMouse;

	int		GetTitleEntry( vgui::VPANEL panel );
};
#endif // MATSYSTEMSURFACE_H
