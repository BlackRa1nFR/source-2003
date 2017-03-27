//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>

#include "GraphPanel.h"


#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>
#include <VGUI_ISurface.h>
#include <VGUI_IVGui.h>
#include <VGUI_KeyValues.h>
#include <VGUI_Label.h>
#include <VGUI_TextEntry.h>
#include <VGUI_Button.h>
#include <VGUI_ToggleButton.h>
#include <VGUI_RadioButton.h>
#include <VGUI_ListPanel.h>
#include <VGUI_ComboBox.h>
#include <VGUI_PHandle.h>
#include <VGUI_PropertySheet.h>
#include <VGUI_CheckButton.h>

#include <VGUI_ILocalize.h>

#define max(a,b)            (((a) > (b)) ? (a) : (b))


using namespace vgui;
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGraphPanel::CGraphPanel(vgui::Panel *parent, const char *name) : PropertyPage(parent, name)
{
	m_pGraphsPanel = new ImagePanel(this,"Graphs");
	m_pGraphs = new CGraphsImage();
	m_pGraphsPanel->SetImage(m_pGraphs);


	m_pClearButton = new Button(this,"Clear","&Reset");
	m_pClearButton->SetVisible(true);
	m_pClearButton->SetEnabled(true);
	m_pClearButton->SetCommand(new KeyValues("clear"));
	m_pClearButton->AddActionSignalTarget(this);

	m_pBandwidthButton = new CheckButton(this,"Bandwidth","Bandwidth");
	m_pBandwidthButton->SetVisible(true);
	m_pBandwidthButton->SetEnabled(true);


	m_pFPSButton = new CheckButton(this,"FPS","FPS");
	m_pFPSButton->SetVisible(true);
	m_pFPSButton->SetEnabled(true);

	m_pCPUButton = new CheckButton(this,"CPU","CPU");
	m_pCPUButton->SetVisible(true);
	m_pCPUButton->SetEnabled(true);

	m_pPINGButton = new CheckButton(this,"Ping","Ping");
	m_pPINGButton->SetVisible(true);
	m_pPINGButton->SetEnabled(true);


	m_pSecButton = new RadioButton(this,"Seconds","Seconds");
	m_pSecButton->SetEnabled(true);
	m_pSecButton->SetVisible(true);
	m_pSecButton->AddActionSignalTarget(this);

	m_pMinButton = new RadioButton(this,"Minutes","Minutes");
	m_pMinButton->SetEnabled(true);
	m_pMinButton->SetVisible(true);
	m_pMinButton->AddActionSignalTarget(this);

	m_pHourButton = new RadioButton(this,"Hours","Hours");
	m_pHourButton->SetEnabled(true);
	m_pHourButton->SetVisible(true);
	m_pHourButton->AddActionSignalTarget(this);

	m_pTimeLabel = new Label(this,"timelabel","Time frame between points");
	m_pGraphsLabel = new Label(this,"graphslabel","Display graph");
	

	// now setup defaults
	m_pCPUButton->SetSelected(false);
	m_pBandwidthButton->SetSelected(false);
	m_pFPSButton->SetSelected(false);
	m_pPINGButton->SetSelected(true);

	m_pSecButton->SetSelected(true);

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGraphPanel::~CGraphPanel()
{

}

//-----------------------------------------------------------------------------
// Purpose: Activates the page
//-----------------------------------------------------------------------------
void CGraphPanel::OnPageShow()
{
}

//-----------------------------------------------------------------------------
// Purpose: Hides the page
//-----------------------------------------------------------------------------
void CGraphPanel::OnPageHide()
{
}

//-----------------------------------------------------------------------------
// Purpose: Ping onlu
//-----------------------------------------------------------------------------
void CGraphPanel::PingOnly(bool state)
{
	m_pGraphs->PingOnly(state);
	
	m_pBandwidthButton->SetVisible(!state);
	m_pFPSButton->SetVisible(!state);
	m_pCPUButton->SetVisible(!state);
	
}


//-----------------------------------------------------------------------------
// Purpose: Relayouts the data
//-----------------------------------------------------------------------------
void CGraphPanel::PerformLayout()
{
	BaseClass::PerformLayout();
	int x,y,w,h;
	GetBounds(x,y,w,h);
	
// you need to set the size of the image panel here...
	m_pGraphsPanel->SetSize(w,h);
	m_pGraphs->SaveSize(w-10,h-35);

	m_pBandwidthButton->SetBounds(x+w-85,y+h-90,80,20);
	m_pFPSButton->SetBounds(x+w-85,y+h-70,75,20);
	m_pCPUButton->SetBounds(x+w-85,y+h-50,75,20);
	m_pPINGButton->SetBounds(x+w-85,y+h-110,75,20);
	m_pGraphsLabel->SetBounds(x+w-85,y+h-130,80,20);

	const int buttonWidth=80,labelWidth=160;

	m_pClearButton->SetBounds(x+2,y+h-50,47,20);

	m_pTimeLabel->SetBounds(x+55,y+h-50,labelWidth,20);
	m_pSecButton->SetBounds(x+55+labelWidth,y+h-50,buttonWidth,20);
	m_pMinButton->SetBounds(x+55+labelWidth+buttonWidth,y+h-50,buttonWidth,20);
	m_pHourButton->SetBounds(x+55+labelWidth+buttonWidth*2,y+h-50,buttonWidth-20,20);
}


//-----------------------------------------------------------------------------
// Purpose: the paint routine for the graph image. Handles the layout and drawing of the graph image
//-----------------------------------------------------------------------------
void CGraphPanel::CGraphsImage::Paint()
{
	int x,y,distPoints;
	int bottom=5; // be 5 pixels above the bottom
	int left=50;

	int offsetNET,offsetFPS,offsetCPU;


	Point *pCpu=NULL,*pIn=NULL, *pOut=NULL,*pFPS=NULL,*pPing=NULL;

	Color g(0,255,0,255); // green
	Color r(255,0,0,255); // red
	Color yel(255,255,0,255); // yellow
	Color lb(0,255,255,255); // light blue
	Color bl(0,0,0,255); // black
	Color purple(255,0,255,255); // purple

	GetSize(x,y);

	y-=4; // borders
	x-=4;


	if(!cpu && !fps && !net && !ping) // no graphs selected
		return; 

	if(points.Count()<2)
		return; // not enough points yet...

	if(x<200 || y<100) 
		return; // to small


	if(cpu)
	{
		if(net && fps)
		{ // all three
			offsetCPU=5;
			offsetNET= -30;
			offsetFPS= 15;
		}
		else if( net )
		{ // net and cpu
			offsetCPU=5;
			offsetNET= -15;

		}
		else
		{ // cpu and fps
			offsetCPU=5;
			offsetFPS=15;
		}
	}
	else
	{
		if(net && fps)
		{ // net and fps
			offsetNET= -15;
			offsetFPS= 5;
		}
		else if( net )
		{ // net
			offsetNET= 0;

		}
		else
		{ //  fps
			offsetFPS=0;
		}

	}


	distPoints= (x-150)/points.Count();
	if(distPoints<=0)
	{
		distPoints=1;
	}

//	DrawOutlinedRect(x-105,5,x,100);

	SetColor(bl);
	DrawLine(10,5,x-105,5);
	DrawLine(10,y/2,x-105,y/2);
	DrawLine(10,y,x-105,y);


	wchar_t tmp[12];
	// draw the legend + axes

	if(cpu)
	{
		SetColor(g); // green
	//	DrawLine(x-25,20,x-5,20);
		localize()->ConvertANSIToUnicode("%CPU", tmp, 12);
		DrawPrintText(x-70,5,tmp,4);

		localize()->ConvertANSIToUnicode("100%", tmp, 12);
		DrawPrintText(5,offsetCPU,tmp,4);
		localize()->ConvertANSIToUnicode("50%", tmp, 12);
		DrawPrintText(5,y/2-offsetCPU,tmp,3);
		localize()->ConvertANSIToUnicode("0%", tmp, 12);
		DrawPrintText(5,y-offsetCPU,tmp,2);
	}
	
	if(net)
	{
		SetColor(r); // red
	//	DrawLine(x-25,40,x-5,40);
		localize()->ConvertANSIToUnicode("In (kB/sec)", tmp, 12);
		DrawPrintText(x-70,20,tmp,11);

		SetColor(yel); //yellow
	//	DrawLine(x-25,60,x-5,60);
		localize()->ConvertANSIToUnicode("Out (kB/sec)", tmp, 12);
		DrawPrintText(x-70,35,tmp,12);
	}


	if(fps)
	{
		SetColor(lb); 
	//	DrawLine(x-25,80,x-5,80);
		localize()->ConvertANSIToUnicode("FPS", tmp, 12);
		DrawPrintText(x-70,50,tmp,3);
	}

	if(ping)
	{
		SetColor(purple); 
		//DrawLine(x-25,100,x-5,80);
		localize()->ConvertANSIToUnicode("Ping", tmp, 12);
		DrawPrintText(x-40,50,tmp,4);

	}


	float Range=0;
	if(net)
	{
	
		// put them on a common scale, base it at zero 
		Range = max(maxIn,maxOut);
			
		Range+=static_cast<float>(Range*0.1); // don't let the top of the range touch the top of the panel

		if(Range<=1) 
		{ // don't let the zero be at the top of the screen
			Range=1.0;
		}

		char numText[20];
		SetColor(yel); //yellow

		_snprintf(numText,20,"%0.2f",Range);
		localize()->ConvertANSIToUnicode(numText, tmp, 12);
		DrawPrintText(5,-offsetNET,tmp,strlen(numText));
		_snprintf(numText,20,"%0.2f",Range/2);
		localize()->ConvertANSIToUnicode(numText, tmp, 12);
		if(cpu && fps && net) 
		{ // when you have all three you have to draw it slightly differently
			DrawPrintText(5,y/2+offsetNET/2,tmp,strlen(numText));
		}
		else
		{
			DrawPrintText(5,y/2+offsetNET,tmp,strlen(numText));
		}
		_snprintf(numText,20,"0");
		localize()->ConvertANSIToUnicode(numText, tmp, 12);
		DrawPrintText(5,y+offsetNET,tmp,1);
		pIn= new Point[points.Count()];
		pOut= new Point[points.Count()];
	}

	float RangeFPS=maxFPS;
	if(fps)
	{
			
		RangeFPS+=static_cast<float>(maxFPS*0.1); // don't let the top of the range touch the top of the panel

		if(RangeFPS<=1) 
		{ // don't let the zero be at the top of the screen
			RangeFPS=1.0;
		}

		char numText[20];
		SetColor(lb); 

		_snprintf(numText,20,"%0.2f",RangeFPS);
		localize()->ConvertANSIToUnicode(numText, tmp, 12);
		DrawPrintText(5,offsetFPS,tmp,strlen(numText));
		_snprintf(numText,20,"%0.2f",RangeFPS/2);
		localize()->ConvertANSIToUnicode(numText, tmp, 12);
		if(cpu && fps && net) 
		{ // when you have all three you have to draw it slightly differently
			DrawPrintText(5,y/2+offsetFPS/2,tmp,strlen(numText));
		}
		else
		{
			DrawPrintText(5,y/2+offsetFPS,tmp,strlen(numText));
		}
		_snprintf(numText,20,"0");
		localize()->ConvertANSIToUnicode(numText, tmp, 12);
		DrawPrintText(5,y-offsetFPS,tmp,1);
		pFPS= new Point[points.Count()];
	}


	if(cpu)
	{
		pCpu= new Point[points.Count()];
	}


	float RangePing=maxPing;
	if(ping)
	{
			
		RangePing+=static_cast<float>(maxPing*0.1); // don't let the top of the range touch the top of the panel

		if(RangePing<=1) 
		{ // don't let the zero be at the top of the screen
			RangePing=1.0;
		}

		char numText[20];
		SetColor(purple); 

		_snprintf(numText,20,"%0.2f",RangePing);
		localize()->ConvertANSIToUnicode(numText, tmp, 12);
		DrawPrintText(x-120,5,tmp,strlen(numText));
		_snprintf(numText,20,"%0.2f",RangePing/2);
		localize()->ConvertANSIToUnicode(numText, tmp, 12);
		DrawPrintText(x-120,y/2-5,tmp,strlen(numText));
		_snprintf(numText,20,"0");
		localize()->ConvertANSIToUnicode(numText, tmp, 12);
		DrawPrintText(x-120,y-5,tmp,1);


		pPing= new Point[points.Count()];

	}

	for(int i=0;i<points.Count();i++)
	// draw the graphs, left to right
	{
	
		if(cpu) 
		{
			pCpu[i].SetPoint(left+i*distPoints,static_cast<int>((1-points[i].cpu)*y));
		}
	
		if(net)
		{
			pIn[i].SetPoint(left+i*distPoints,static_cast<int>(( (Range-points[i].in)/Range)*y-bottom));
	
			pOut[i].SetPoint(left+i*distPoints,static_cast<int>(((Range-points[i].out)/Range)*y-bottom));
		}
		if(fps)
		{
			pFPS[i].SetPoint(left+i*distPoints,static_cast<int>(( (RangeFPS-points[i].fps)/RangeFPS)*y-bottom));

		}

		if(ping)
		{
			pPing[i].SetPoint(left+i*distPoints,static_cast<int>(( (RangePing-points[i].ping)/RangePing)*y-bottom));

		}
	
	}
	// we use DrawPolyLine, its much, much, much more efficient than calling lots of DrawLine()'s

	if(cpu)
	{
		SetColor(g); // green
		DrawPolyLine(pCpu,points.Count());	
	}

	if(net) 
	{
		SetColor(r); // red
		DrawPolyLine(pIn,points.Count());	

		SetColor(yel); //yellow
		DrawPolyLine(pOut,points.Count());	
	}
	if(fps)
	{
		SetColor(lb);
		DrawPolyLine(pFPS,points.Count());	
	}

	if(ping)
	{
		SetColor(purple);
		DrawPolyLine(pPing,points.Count());	
	}
	
	if(cpu && pCpu) free(pCpu);
	if(net &&pIn) free(pIn);
	if(net && pOut) free(pOut);
	if(fps && pFPS) free(pFPS);
	if(ping && pPing) free(pPing);
}  


//-----------------------------------------------------------------------------
// Purpose: constructor for the graphs image
//-----------------------------------------------------------------------------
CGraphPanel::CGraphsImage::CGraphsImage(): vgui::Image(), points()
{
	maxIn=maxOut=minIn=minOut=minFPS=maxFPS=minPing=maxPing=0;
	net=fps=cpu=ping=false;
	ping_only=true; // by default pings only
	numAvgs=0;
	memset(&avgPoint,0x0,sizeof(Points_t));

}

//-----------------------------------------------------------------------------
// Purpose: sets which graph to draw, true means draw it
//-----------------------------------------------------------------------------
void CGraphPanel::CGraphsImage::SetDraw(bool cpu_in,bool fps_in,bool net_in,bool ping_in)
{
	cpu=cpu_in;
	fps=fps_in;
	net=net_in;
	ping=ping_in;
	if(ping_only)
	{
		cpu=false;
		net=false;
		fps=false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: used to average points over a period of time
//-----------------------------------------------------------------------------
void CGraphPanel::CGraphsImage::AvgPoint(Points_t p)
{
	avgPoint.cpu += p.cpu;
	avgPoint.fps += p.fps;
	avgPoint.in += p.in;
	avgPoint.out += p.out;
	avgPoint.ping += p.ping;
	numAvgs++;
}


//-----------------------------------------------------------------------------
// Purpose: updates the current bounds of the points based on this new point
//-----------------------------------------------------------------------------
void CGraphPanel::CGraphsImage::CheckBounds(Points_t p)
{
	if(p.in>maxIn)
	{
		maxIn=avgPoint.in;
	}
	if(p.out>maxOut)
	{
		maxOut=avgPoint.out;
	}

	if(p.in<minIn)
	{
		minIn=avgPoint.in;
	}
	if(p.out<minOut)
	{
		minOut=avgPoint.out;
	}

	if(p.fps>maxFPS)
	{
		maxFPS=avgPoint.fps;
	}
	if(p.fps<minFPS)
	{
		minFPS=avgPoint.fps;
	}

	if(p.ping>maxPing)
	{
		maxPing=avgPoint.ping;
	}
	if(p.ping<minPing)
	{
		minPing=avgPoint.ping;
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: adds a point to the graph image. 
//-----------------------------------------------------------------------------
bool CGraphPanel::CGraphsImage::AddPoint(Points_t p)
{
	int x,y;
	bool recalcBounds=false;

	GetSize(x,y);
	
	if(avgPoint.cpu>1)  // cpu is a percent !
	{
		return false;
	}

	if(timeBetween==HOURS)
	{
		
		if(points.Count()>1 && (p.time - points[points.Count()-1].time ) < 5*60 ) 
			// only add a point every 5 minutes											 
			// which means 12 per hour
		{
			AvgPoint(p);
			return false;
		}

	} 
	else if ( timeBetween==MINUTES)
	{
		
		if(points.Count()>1 && (p.time - points[points.Count()-1].time ) < 30 ) 
			// only add a point every 30 seconds										 
			// which means 120 per hour
		{
			AvgPoint(p);
			return false;
		}

	}

	AvgPoint(p);
	// now work out the average of all the values
	avgPoint.cpu /= numAvgs;
	avgPoint.fps /= numAvgs;
	avgPoint.in  /= numAvgs;
	avgPoint.out /= numAvgs;
	avgPoint.ping /= numAvgs;
	avgPoint.time = p.time;
	numAvgs=0;

	int k= points.Count();

	if(x!=0 && points.Count()> x/2) 
	// there are more points than pixels so drop one.
	{
		while(points.Count()> x/2)
		{
			// check that the bounds don't move
			if(points[0].in==maxIn ||
				points[0].out==maxOut ||
				points[0].fps==maxFPS ||
				points[0].ping==maxPing )
			{
				recalcBounds=true;
			}
			points.Remove(0); // remove the head node
		}
	}

	if(recalcBounds) 
	{
		for(int i=0;i<points.Count();i++)
		{
			CheckBounds(points[i]);
		}
	}


	CheckBounds(avgPoint);

	points.AddToTail(avgPoint);
	
	memset(&avgPoint,0x0,sizeof(Points_t));

	return true;
}

void CGraphPanel::CGraphsImage::SetScale(intervals time)
{
	timeBetween=time;

	// scale is reset so remove all the points
	points.RemoveAll();

	// and reset the maxes
	maxIn=maxOut=minIn=minOut=minFPS=maxFPS=minPing=maxPing=0;

}


//-----------------------------------------------------------------------------
// Purpose: run when the send button is pressed, send a rcon "say" to the server
//-----------------------------------------------------------------------------
void CGraphPanel::OnSendChat()
{

}

//-----------------------------------------------------------------------------
// Purpose: clear button handler, clears the current points
//-----------------------------------------------------------------------------
void CGraphPanel::OnClearButton()
{
	m_pGraphs->RemovePoints();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: passes the state of the check buttons (for graph line display) through to the graph image
//-----------------------------------------------------------------------------
void CGraphPanel::OnCheckButton()
{
	m_pGraphs->SetDraw(m_pCPUButton->IsSelected(),m_pFPSButton->IsSelected(),
		m_pBandwidthButton->IsSelected(),m_pPINGButton->IsSelected());

}

//-----------------------------------------------------------------------------
// Purpose:Handles the scale radio buttons, passes the scale to use through to the graph image
//-----------------------------------------------------------------------------
void CGraphPanel::OnButtonToggled()
{
	if(m_pSecButton->IsSelected())
	{
		m_pGraphs->SetScale(SECONDS);
	}
	else if(m_pMinButton->IsSelected())
	{
		m_pGraphs->SetScale(MINUTES);
	}
	else
	{
		m_pGraphs->SetScale(HOURS);
	}
}
//-----------------------------------------------------------------------------
// Purpose: Message map
//-----------------------------------------------------------------------------
MessageMapItem_t CGraphPanel::m_MessageMap[] =
{
	MAP_MESSAGE( CGraphPanel, "SendChat", OnSendChat ),
	MAP_MESSAGE( CGraphPanel, "CheckButtonChecked", OnCheckButton ),
	MAP_MESSAGE( CGraphPanel, "clear", OnClearButton ),

	MAP_MESSAGE( CGraphPanel, "RadioButtonChecked", OnButtonToggled ),
//	MAP_MESSAGE_PTR_CONSTCHARPTR( CGraphPanel, "TextChanged", OnTextChanged, "panel", "text" ),
};

IMPLEMENT_PANELMAP( CGraphPanel, Frame );