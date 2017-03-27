// MessageBuffer.cpp: implementation of the CMessageBuffer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Status.h"
#include "MessageBuffer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// For handling endian issues:
BOOL  bigendien;

short   (*BigShort) (short l);
short   (*LittleShort) (short l);
int     (*BigLong) (int l);
int     (*LittleLong) (int l);
float   (*BigFloat) (float l);
float   (*LittleFloat) (float l);

/*
============================================================================

					unsigned char ORDER FUNCTIONS

============================================================================
*/

short   ShortSwap (short l)
{
	unsigned char    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   ShortNoSwap (short l)
{
	return l;
}

int    LongSwap (int l)
{
	unsigned char    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int     LongNoSwap (int l)
{
	return l;
}

float FloatSwap (float f)
{
	union
	{
		float   f;
		unsigned char    b[4];
	} dat1, dat2;
	
	
	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//===================================================================
// CMessageBuffer()
//
// CMessageBuffer:  Allocate message buffer, no default size yet.
//===================================================================
CMessageBuffer::CMessageBuffer()
{
	unsigned char    swaptest[2] = {1,0};

	// set the unsigned char swapping variables in a portable manner 
	if ( *(short *)swaptest == 1)
	{
		bigendien = FALSE;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien = TRUE;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	};

	m_bAllowOverflow = FALSE;	// if FALSE, Error
	m_bOverFlowed    = FALSE;	// set to TRUE if the buffer size failed
	
	m_pData		  = NULL;
	m_nMaxSize       = 0;
	m_nCurSize       = 0;

	//SZ_Alloc(NET_MAXMESSAGE);
}

//===================================================================
// CMessageBuffer()
//
// CMessageBuffer:  Allocate buffer and set up default size.
//===================================================================
CMessageBuffer::CMessageBuffer(int sz)
{
	m_bAllowOverflow = TRUE;	// if FALSE, Error
	m_bOverFlowed    = FALSE;	// set to TRUE if the buffer size failed
	
	m_pData		      = NULL;
	m_nMaxSize       = 0;
	m_nCurSize       = 0;

	SZ_Alloc(sz);
}

//===================================================================
// ~CMessageBuffer()
//
// ~CMessageBuffer:  Destructor, deallocate any memory used.
//===================================================================
CMessageBuffer::~CMessageBuffer()
{
	// Free any data pointed to by m_pData
	SZ_Free();
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles unsigned char ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//
void CMessageBuffer::MSG_WriteChar (int c)
{
	unsigned char    *buf;
	
	buf = (unsigned char *)SZ_GetSpace (1);
	buf[0] = c;
}

void CMessageBuffer::MSG_WriteByte (int c)
{
	unsigned char    *buf;
	
	buf = (unsigned char *)SZ_GetSpace (1);
	buf[0] = c;
}


void CMessageBuffer::MSG_WriteShort (int c)
{
	unsigned char    *buf;
	
	buf = (unsigned char *)SZ_GetSpace (2);
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

void CMessageBuffer::MSG_WriteWord (int c)
{
	unsigned char    *buf;
	
	buf = (unsigned char *)SZ_GetSpace (2);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
}

void CMessageBuffer::MSG_WriteLong (int c)
{
	unsigned char    *buf;
	
	buf = (unsigned char *)SZ_GetSpace (4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}

void CMessageBuffer::MSG_WriteFloat (float f)
{
	union
	{
		float   f;
		int     l;
	} dat;
	
	
	dat.f = f;
	dat.l = LittleLong (dat.l);
	
	SZ_Write (&dat.l, 4);
}

void CMessageBuffer::MSG_WriteString (char *s)
{
	if (!s)
		SZ_Write ("", 1);
	else
		SZ_Write (s, strlen(s)+1);
}

void CMessageBuffer::MSG_WriteBuf (int iSize, void *buf)
{
	if (!buf)
		return;

	SZ_Write (buf, iSize);
}

void CMessageBuffer::MSG_WriteCoord (float f)
{
	MSG_WriteShort ((int)(f*8));
}

void CMessageBuffer::MSG_WriteAngle (float f)
{
	CMessageBuffer::MSG_WriteByte (((int)(f*256/360)) & 255);
}

void CMessageBuffer::MSG_WriteHiresAngle (float f)
{
	MSG_WriteShort (((int)(f*65536/360)) & 0xFFFF);
}

//
// reading functions
//
void CMessageBuffer::MSG_BeginReading (void)
{
	m_nMsgReadCount = 0;
	m_bMsgBadRead = FALSE;
}

// returns -1 and sets m_bMsgBadRead if no more characters are available
int CMessageBuffer::MSG_ReadChar (void)
{
	int     c;
	
	if (m_nMsgReadCount+1 > m_nCurSize)
	{
		m_bMsgBadRead = TRUE;
		return -1;
	}
		
	c = (signed char)m_pData[m_nMsgReadCount];
	m_nMsgReadCount++;
	
	return c;
}

int CMessageBuffer::MSG_ReadByte (void)
{
	int     c;
	
	if (m_nMsgReadCount+1 > m_nCurSize)
	{
		m_bMsgBadRead = TRUE;
		return -1;
	}
		
	c = (unsigned char)m_pData[m_nMsgReadCount];
	m_nMsgReadCount++;
	
	return c;
}

int CMessageBuffer::MSG_ReadShort (void)
{
	int     c;
	
	if (m_nMsgReadCount+2 > m_nCurSize)
	{
		m_bMsgBadRead = TRUE;
		return -1;
	}
		
	c = (short)(m_pData[m_nMsgReadCount]
	+ (m_pData[m_nMsgReadCount+1]<<8));
	
	m_nMsgReadCount += 2;
	
	return c;
}

int CMessageBuffer::MSG_ReadWord (void)
{
	int     c;
	
	if (m_nMsgReadCount+2 > m_nCurSize)
	{
		m_bMsgBadRead = TRUE;
		return -1;
	}
		
	c = m_pData[m_nMsgReadCount]
	+ (m_pData[m_nMsgReadCount+1]<<8);
	
	m_nMsgReadCount += 2;
	
	return c;
}


int CMessageBuffer::MSG_ReadLong (void)
{
	int     c;
	
	if (m_nMsgReadCount+4 > m_nCurSize)
	{
		m_bMsgBadRead = TRUE;
		return -1;
	}
		
	c = m_pData[m_nMsgReadCount]
	+ (m_pData[m_nMsgReadCount+1]<<8)
	+ (m_pData[m_nMsgReadCount+2]<<16)
	+ (m_pData[m_nMsgReadCount+3]<<24);
	
	m_nMsgReadCount += 4;
	
	return c;
}

float CMessageBuffer::MSG_ReadFloat (void)
{
	union
	{
		unsigned char    b[4];
		float   f;
		int     l;
	} dat;
	
	dat.b[0] =      m_pData[m_nMsgReadCount];
	dat.b[1] =      m_pData[m_nMsgReadCount+1];
	dat.b[2] =      m_pData[m_nMsgReadCount+2];
	dat.b[3] =      m_pData[m_nMsgReadCount+3];
	m_nMsgReadCount += 4;
	
	dat.l = LittleLong (dat.l);

	return dat.f;   
}

int CMessageBuffer::MSG_ReadBuf(int iSize, void *pbuf)
{
	if (m_nMsgReadCount + iSize > m_nCurSize)
	{
		m_bMsgBadRead = TRUE;
		return -1;
	}
		
	memcpy(pbuf, &m_pData[m_nMsgReadCount], iSize);

	m_nMsgReadCount += iSize;
	
	return 1;
}

char *CMessageBuffer::MSG_ReadString (void)
{
	static char     string[2048];
	int             l,c;
	
	l = 0;
	do
	{
		c = MSG_ReadChar ();
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);
	
	string[l] = 0;
	
	return string;
}

float CMessageBuffer::MSG_ReadCoord (void)
{
	return MSG_ReadShort() * (1.0f/8.0f);
}

float CMessageBuffer::MSG_ReadAngle (void)
{
	return MSG_ReadChar() * (360.0f/256.0f);
}

float CMessageBuffer::MSG_ReadHiresAngle (void)
{
	return MSG_ReadShort() * (360.0f/65536.0f);
}

//===========================================================================

void CMessageBuffer::SZ_Alloc (int startsize)
{
	if (startsize < 256)
		startsize = 256;
	m_pData = (unsigned char *)malloc(startsize);
	m_nMaxSize = startsize;
	m_nCurSize = 0;
}


void CMessageBuffer::SZ_Free ()
{
	if (m_pData)
		free(m_pData);
    m_pData = NULL;
	m_nCurSize = 0;
}

void CMessageBuffer::SZ_Clear ()
{
	m_nCurSize = 0;
	memset(m_pData, 0, m_nMaxSize);
}

void *CMessageBuffer::SZ_GetSpace (int length)
{
	void    *d;
	
	if (m_nCurSize + length > m_nMaxSize)
	{
		if (!m_bAllowOverflow)
		{
			exit(1);
		};
		
		if (length > m_nMaxSize)
		{
			exit(1);
		};
			
		m_bOverFlowed = TRUE;
		SZ_Clear (); 
	}

	d = m_pData + m_nCurSize;
	m_nCurSize += length;
	return d;
}

void CMessageBuffer::SZ_Write (void *m_pData, int length)
{
	memcpy (SZ_GetSpace(length), m_pData, length);         
}

void CMessageBuffer::SZ_Print (char *m_pData)
{
	int             len;
	
	len = strlen(m_pData)+1;

	// unsigned char * cast to keep VC++ happy
	if (m_pData[m_nCurSize-1])
		memcpy ((unsigned char *)SZ_GetSpace(len),m_pData,len); // no trailing 0
	else
		memcpy ((unsigned char *)SZ_GetSpace(len-1)-1,m_pData,len); // write over trailing 0
}

//===================================================================
// GetMaxSize()
//
// GetMaxSize:  Returns current maximum size.
//===================================================================
int CMessageBuffer::GetMaxSize()
{
	return m_nMaxSize;
}

//===================================================================
// GetData()
//
// GetData:  Returns pointer to data.
//===================================================================
void * CMessageBuffer::GetData()
{
	return m_pData;
}

//===================================================================
// SetCurSize()
//
// SetCurSize:  Sets current size.
//===================================================================
void CMessageBuffer::SetCurSize(int nSize)
{
	m_nCurSize = nSize;
}

//===================================================================
// GetCurSize()
//
// GetCurSize:  Retrieves current size of buffer.
//===================================================================
int CMessageBuffer::GetCurSize()
{
	return m_nCurSize;
}

int CMessageBuffer::GetReadCount()
{
	return m_nMsgReadCount;
}
