#include "quakedef.h"
#include "server.h"

extern ConVar rcon_password;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int - 1 == allow the rcom, 0 == decline it
//-----------------------------------------------------------------------------
int SV_Rcom_Validate (void)
{
	// Must have a password set to allow any rconning.
	if ( !strlen(rcon_password.GetString() ) )
		return 0;

	// If the pw does not match, then disallow command
	if (strcmp (Cmd_Argv(1), rcon_password.GetString() ) )
		return 0;

	// Otherwise it's ok.
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: A client issued an rcom command.  Shift down the remaining args and redirect all Con_Printfs
// Input  : *net_from - 
//-----------------------------------------------------------------------------
void SV_Rcom (netadr_t *net_from)
{
	int		i;
	char	remaining[1024];
	
	// Verify this user has access rights.
	i = SV_Rcom_Validate ();

	if ( i == 0 )
	{
		Con_Printf ("Bad rcon from %s:\n%s\n", NET_AdrToString (*net_from), net_message.data + 4 );
	}
	if ( i == 1 )
	{
		Con_Printf ("Rcon from %s:\n%s\n", NET_AdrToString (*net_from), net_message.data + 4 );
	}

	SV_RedirectStart (RD_PACKET, net_from);

	if ( !i )
	{
		Con_Printf ("Bad rcon_password.\n");
	}
	else
	{
		remaining[0] = 0;
		for (i=2 ; i<Cmd_Argc() ; i++)
		{
			strcat (remaining, Cmd_Argv(i) );
			strcat (remaining, " ");
		}

		// Act as if it was typed in at command console.
		Cmd_ExecuteString (remaining, src_command);

	}

	SV_RedirectEnd();

}