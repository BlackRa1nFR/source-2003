//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ISCRATCHPAD3D_H
#define ISCRATCHPAD3D_H
#ifdef _WIN32
#pragma once
#endif


// IScratchPad3D will store drawing commands in a file to be viewed by ScratchPad3DViewer.
// It can be used while stepping through geometry code to visualize what is going on as
// drawing commands will be immediately visible in ScratchPad3DViewer even while you're stuck
// in the debugger

// ScratchPad3DViewer initially orbits 100 inches from the origin, so it can be useful
// to call SetMapping to map what you're drawing input into this cube.


#include "vector.h"
#include "vector2d.h"
#include "utlvector.h"

class IFileSystem;


class CSPVert
{
public:
				CSPVert();
				CSPVert( Vector const &vPos, Vector const &vColor=Vector(1,1,1) );
	void		Init( Vector const &vPos, Vector const &vColor=Vector(1,1,1) );

	Vector		m_vPos;
	Vector		m_vColor;
};

class CSPVertList
{
public:
				CSPVertList( int nVerts = 0 );
				CSPVertList(CSPVert const *pVerts, int nVerts);
				CSPVertList(Vector const *pVerts, int nVerts, Vector vColor=Vector(1,1,1) );
				CSPVertList(Vector const *pVerts, Vector const *pColors, int nVerts);
				CSPVertList(Vector const &vert1, Vector const &color1, 
					Vector const &vert2, Vector const &color2, 
					Vector const &vert3, Vector const &color3);

	CUtlVector<CSPVert>	m_Verts;
};

class SPRGBA
{
public:
	unsigned char r,g,b,a;
};


class IScratchPad3D
{
protected:

	virtual				~IScratchPad3D() {}


// Types.
public:

	enum RenderState
	{
		RS_FillMode=0,	// val = one of the FillMode enums
		RS_ZRead,
		RS_ZBias		// val = 0 - 16 to push Z towards viewer
	};

	enum FillMode
	{
		FillMode_Wireframe=0,
		FillMode_Solid
	};


public:
	
	virtual void		Release() = 0;

	// This sets up a mapping between input coordinates and output coordinates.
	// This can be used to zoom into an area of interest where you'll be drawing things.
	// An alternative is to press Z while in VisLibViewer to have it center and zoom on
	// everything that has been drawn.
	virtual void		SetMapping( 
		Vector const &vInputMin, 
		Vector const &vInputMax,
		Vector const &vOutputMin,
		Vector const &vOutputMax ) = 0;

	// Enable/disable auto flush. When set to true (the default), all drawing commands
	// are immediately written to the file and will show up in VisLibViewer right away.
	// If you want to draw a lot of things, you can set this to false and call Flush() 
	// manually when you want the file written out.
	// When you set auto flush to true, it calls Flush().
	virtual void		SetAutoFlush( bool bAutoFlush ) = 0;

	// Draw a point. Point size is (roughly) in world coordinates, so points
	// get smaller as the viewer moves away.
	virtual void		DrawPoint( CSPVert const &v, float flPointSize ) = 0;

	// Draw a line.
	virtual void		DrawLine( CSPVert const &v1, CSPVert const &v2 ) = 0;

	// Draw a polygon.
	virtual void		DrawPolygon( CSPVertList const &verts ) = 0;

	// Draw 2D rectangles.
	virtual void		DrawRectYZ( float xPos, Vector2D const &vMin, Vector2D const &vMax, Vector const &vColor ) = 0;
	virtual void		DrawRectXZ( float yPos, Vector2D const &vMin, Vector2D const &vMax, Vector const &vColor ) = 0;
	virtual void		DrawRectXY( float zPos, Vector2D const &vMin, Vector2D const &vMax, Vector const &vColor ) = 0;

	// Draw a wireframe box.
	virtual void		DrawWireframeBox( Vector const &vMin, Vector const &vMax, Vector const &vColor ) = 0;

	// Wireframe on/off.	
	virtual void		SetRenderState( RenderState state, unsigned long val ) = 0;

	// Clear all the drawing commands.
	virtual void		Clear() = 0;

	// Calling this writes all the commands to the file. If AutoFlush is true, this is called
	// automatically in all the drawing commands.
	virtual void		Flush() = 0;


// Primitives that build on the atomic primitives.
public:
	
	// Draw a black and white image.
	virtual void		DrawImageBW( unsigned char const *pData, int width, int height, int pitchInBytes ) = 0;
	
	// Draw an RGBA image.
	virtual void		DrawImageRGBA( SPRGBA *pData, int width, int height, int pitchInBytes ) = 0;
};


// Just a helper for functions where you want to have a CScratchPad3D around 
// and release it automatically when the function exits.
class CScratchPadAutoRelease
{
public:
			CScratchPadAutoRelease( IScratchPad3D *pPad )	{ m_pPad = pPad; }
			~CScratchPadAutoRelease()						{ if( m_pPad ) m_pPad->Release(); }

	IScratchPad3D *m_pPad;
};



IScratchPad3D* ScratchPad3D_Create( char const *pFilename = "scratch.pad" );



// ------------------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------------------ //

inline CSPVert::CSPVert()
{
}

inline CSPVert::CSPVert( Vector const &vPos, Vector const &vColor )
{
	Init( vPos, vColor );
}

inline void CSPVert::Init( Vector const &vPos, Vector const &vColor )
{
	m_vPos = vPos;
	m_vColor = vColor;
}


inline CSPVertList::CSPVertList( int nVerts )
{
	if( nVerts )
		m_Verts.AddMultipleToTail( nVerts );
}

inline CSPVertList::CSPVertList(CSPVert const *pVerts, int nVerts )
{
	m_Verts.CopyArray( pVerts, nVerts );
}

inline CSPVertList::CSPVertList(Vector const *pVerts, int nVerts, Vector vColor )
{
	m_Verts.AddMultipleToTail( nVerts );
	for( int i=0; i < nVerts; i++ )
	{
		m_Verts[i].m_vPos = pVerts[i];
		m_Verts[i].m_vColor = vColor;
	}
}

inline CSPVertList::CSPVertList( Vector const *pVerts, Vector const *pColors, int nVerts )
{
	m_Verts.AddMultipleToTail( nVerts );
	for( int i=0; i < nVerts; i++ )
	{
		m_Verts[i].m_vPos = pVerts[i];
		m_Verts[i].m_vColor = pColors[i];
	}
}

inline CSPVertList::CSPVertList( 
	Vector const &vert1, Vector const &color1, 
	Vector const &vert2, Vector const &color2, 
	Vector const &vert3, Vector const &color3 )
{
	m_Verts.AddMultipleToTail( 3 );
	m_Verts[0].Init( vert1, color1 );
	m_Verts[1].Init( vert2, color2 );
	m_Verts[2].Init( vert3, color3 );
}


#endif // ISCRATCHPAD3D_H
