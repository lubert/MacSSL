/*
 * network_mac.c
 *
 * Classic Mac OS networking abstraction using OpenTransport
 * Wrapper functions for network operations
 */

#include "network_mac.h"
#include <Types.h>
#include <OpenTransport.h>
#include <OpenTptInternet.h>

/* Global variables */
static Boolean gNetworkInitialized = false;
static InetSvcRef gInetService = NULL;

/* Network initialization and cleanup */
OSStatus Network_Initialize(void) {
    OSStatus err;

    if (gNetworkInitialized) {
        return noErr;
    }

    err = InitOpenTransport();
    if (err != noErr) {
        return err;
    }

    gInetService = OTOpenInternetServices(kDefaultInternetServicesPath, 0, &err);
    if (err != noErr || gInetService == NULL) {
        CloseOpenTransport();
        return err;
    }

    gNetworkInitialized = true;
    return noErr;
}

void Network_Cleanup(void) {
    if (gNetworkInitialized) {
        if (gInetService != NULL) {
            OTCloseProvider(gInetService);
            gInetService = NULL;
        }
        CloseOpenTransport();
        gNetworkInitialized = false;
    }
}

/* DNS resolution */
OSStatus ResolveHostname(const char* hostname, InetAddress* addr) {
    InetHostInfo hostInfo;
    OSStatus err;

    if (!gNetworkInitialized || gInetService == NULL) {
        return paramErr;
    }

    err = OTInetStringToAddress(gInetService, (char*)hostname, &hostInfo);
    if (err != noErr) {
        return err;
    }

    OTInitInetAddress(addr, 0, hostInfo.addrs[0]);
    return noErr;
}

/* TCP connection management */
OSStatus EstablishTCPConnection(InetAddress* addr, EndpointRef* endpoint) {
    OSStatus err;
    OTConfigurationRef config;
    TEndpointInfo info;
    TCall sndCall;
    TBind reqAddr;

    if (!gNetworkInitialized || addr == NULL || endpoint == NULL) {
        return paramErr;
    }

    /* Create TCP configuration */
    config = OTCreateConfiguration(kTCPName);
    if (config == NULL) {
        return kOTConfigurationChangedErr;
    }

    /* Open TCP endpoint */
    *endpoint = OTOpenEndpoint(config, 0, &info, &err);
    if (err != noErr || *endpoint == NULL) {
        return err;
    }

    /* Bind to any local address */
    OTMemzero(&reqAddr, sizeof(reqAddr));
    err = OTBind(*endpoint, &reqAddr, NULL);
    if (err != noErr) {
        OTCloseProvider(*endpoint);
        return err;
    }

    /* Connect to remote address */
    OTMemzero(&sndCall, sizeof(sndCall));
    sndCall.addr.buf = (UInt8*)addr;
    sndCall.addr.len = sizeof(InetAddress);

    err = OTConnect(*endpoint, &sndCall, NULL);
    if (err != noErr) {
        OTUnbind(*endpoint);
        OTCloseProvider(*endpoint);
        return err;
    }

    return noErr;
}

void CloseTCPConnection(EndpointRef endpoint) {
    if (endpoint != NULL) {
        OTUnbind(endpoint);
        OTCloseProvider(endpoint);
    }
}