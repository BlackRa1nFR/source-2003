#include "quakedef.h"
#include "server.h"
#include "proto_oob.h"

static redirect_t	sv_redirected;
static netadr_t		sv_redirectto;
static char			sv_redirect_buffer[ 1400 ];

//-----------------------------------------------------------------------------
// Purpose: Clears all remaining data from the redirection buffer.
//-----------------------------------------------------------------------------
void SV_RedirectFlush( void )
{
	if ( sv_redirected == RD_PACKET )   // Print to remote address.
	{
		byte	data[1420];
	
		bf_write buf( "SV_RedirectFlush", data, sizeof(data) );

		buf.WriteLong   ( 0xFFFFFFFF ); // -1 -1 -1 -1 signal
		buf.WriteByte   ( A2C_PRINT );
		buf.WriteString ( sv_redirect_buffer );
		buf.WriteByte   ( 0 );
		NET_SendPacket( NS_SERVER, buf.GetNumBytesWritten(), buf.GetData(), sv_redirectto );
	}
	else if ( sv_redirected == RD_CLIENT )   // Send to client on message stream.
	{
		host_client->netchan.message.WriteByte( svc_print );
		host_client->netchan.message.WriteString( sv_redirect_buffer );
	}
	// clear it
	sv_redirect_buffer[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Sents console printfs to remote client instead of to console
// Input  : rd - 
//			*addr - 
//-----------------------------------------------------------------------------
void SV_RedirectStart (redirect_t rd, netadr_t *addr)
{
	sv_redirected = rd;
	sv_redirectto = *addr;
	sv_redirect_buffer[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Flushes buffers to network, and resets mode to inactive
//-----------------------------------------------------------------------------
void SV_RedirectEnd (void)
{
	SV_RedirectFlush ();
	sv_redirected = RD_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : len - 
//-----------------------------------------------------------------------------
void SV_RedirectCheckFlush( int len )
{
	if ( len + Q_strlen( sv_redirect_buffer ) > sizeof(sv_redirect_buffer) - 1)
	{
		SV_RedirectFlush();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : qboolean
//-----------------------------------------------------------------------------
qboolean SV_RedirectActive( void )
{	
	return ( sv_redirected ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *txt - 
//-----------------------------------------------------------------------------
void SV_RedirectAddText( const char *txt )
{
	SV_RedirectCheckFlush( strlen( txt ) );
	Q_strcat ( sv_redirect_buffer, (char *)txt );
}