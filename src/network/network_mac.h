/*
 * network_mac.h
 *
 * Classic Mac OS networking abstraction using OpenTransport
 */

#ifndef NETWORK_MAC_H
#define NETWORK_MAC_H

#include <OpenTransport.h>
#include <OpenTptInternet.h>

/* Network initialization and cleanup */
OSStatus Network_Initialize(void);
void Network_Cleanup(void);

/* DNS resolution */
OSStatus ResolveHostname(const char* hostname, InetAddress* addr);

/* TCP connection management */
OSStatus EstablishTCPConnection(InetAddress* addr, EndpointRef* endpoint);
void CloseTCPConnection(EndpointRef endpoint);

#endif /* NETWORK_MAC_H */