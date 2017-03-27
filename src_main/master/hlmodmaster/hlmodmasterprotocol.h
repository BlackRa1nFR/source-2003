#ifndef INC_HLMODMASTERPROTOCOLH
#define INC_HLMODMASTERPROTOCOLH

#include <stdio.h>
#include <string.h>
#include <winsock.h>
#include <time.h>
#include <errno.h>

/*
Master server protocols
The first byte is the command code.
Followed by a \n, then any other parameters, each followed by a \n.
An empty parameter is acceptable.
*/

#define LAUNCHERONLY
//#include "../../engine/Protocol.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

typedef struct sockaddr_in	netadr_t;
typedef unsigned char byte;

typedef struct keypair_s
{
	char *key;
	char *value;

	struct keypair_s *next;
} keypair_t;

typedef struct mod_s
{
	struct	mod_s	*next;
	keypair_t *keys;
} mod_t;

typedef struct banned_sv_s
{
	struct banned_sv_s	*next;

	netadr_t	address;

} banned_sv_t;


#endif // INC_HLMODMASTERPROTOCOLH
