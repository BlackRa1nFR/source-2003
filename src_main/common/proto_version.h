#if !defined( PROTO_VERSION_H )
#define PROTO_VERSION_H
#ifdef _WIN32
#pragma once
#endif

// The current network protocol version.  Changing this makes clients and servers incompatible
#define PROTOCOL_VERSION    2

// The client listens for incoming messages from the server and responds on this port
#define PORT_CLIENT "27005"
// The server listens and sends on this port.
#define PORT_SERVER "27015"  

#endif