/*
 * open_transport_stubs.c
 *
 * Stub implementations of Open Transport functions for Retro68 testing
 * These allow the application to build and show the interface
 * TODO: Replace with real Open Transport library linking
 */

#include <Types.h>
#include <OpenTransport.h>
#include <OpenTptInternet.h>

/* Stub implementations */
OSStatus InitOpenTransport(void) {
    return noErr; /* Stub - always succeeds */
}

void CloseOpenTransport(void) {
    /* Stub - do nothing */
}

InetSvcRef OTOpenInternetServices(const char* configName, OTOpenFlags flags, OSStatus* err) {
    if (err) *err = noErr;
    return (InetSvcRef)1; /* Return non-null pointer */
}

OSStatus OTCloseProvider(ProviderRef ref) {
    return noErr; /* Stub - always succeeds */
}

OSStatus OTInetStringToAddress(InetSvcRef ref, char* name, InetHostInfo* hinfo) {
    /* Stub - fill in fake host info */
    if (hinfo) {
        hinfo->addrs[0] = 0x08080808; /* 8.8.8.8 as example */
    }
    return noErr;
}

void OTInitInetAddress(InetAddress* addr, InetPort port, InetHost host) {
    /* Stub - fill in the address structure */
    if (addr) {
        addr->fAddressType = AF_INET;
        addr->fPort = port;
        addr->fHost = host;
    }
}

EndpointRef OTOpenEndpoint(void* config, OTOpenFlags flags, TEndpointInfo* info, OSStatus* err) {
    if (err) *err = noErr;
    return (EndpointRef)2; /* Return non-null pointer */
}

void* OTCreateConfiguration(const char* name) {
    return (void*)3; /* Return non-null pointer */
}

OSStatus OTBind(EndpointRef ref, TBind* reqAddr, TBind* retAddr) {
    return noErr; /* Stub - always succeeds */
}

OSStatus OTConnect(EndpointRef ref, TCall* sndCall, TCall* rcvCall) {
    return noErr; /* Stub - always succeeds */
}

OTResult OTLook(EndpointRef ref) {
    return T_CONNECT; /* Stub - pretend connection is ready */
}

OSStatus OTRcvConnect(EndpointRef ref, TCall* call) {
    return noErr; /* Stub - always succeeds */
}

OTResult OTSnd(EndpointRef ref, void* buf, OTByteCount nbytes, OTFlags flags) {
    return (OTResult)nbytes; /* Stub - pretend all bytes were sent */
}

OTResult OTRcv(EndpointRef ref, void* buf, OTByteCount nbytes, OTFlags* flags) {
    return kOTNoDataErr; /* Stub - no data available */
}

OTResult OTGetEndpointState(EndpointRef ref) {
    return T_DATAXFER; /* Stub - pretend we're in data transfer state */
}

