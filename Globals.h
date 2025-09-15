#ifndef GLOBALS_H
#define GLOBALS_H

#include "api.h"
#include "SSLWrapper.h"  /* Needed for SSLState type */

/* Edit menu constants */
#define kEditMenuID 129
#define kEditSelectAll 1
#define kEditCopy 3

/* Global variable declarations */
extern MenuHandle gFileMenu;
extern MenuHandle gEditMenu;
extern WindowPtr gMainWindow;
extern ControlHandle gConnectButton;
extern EndpointRef gTCPEndpoint;
extern InetSvcRef gInetService;
extern char gResponseBuffer[MAX_RESPONSE_SIZE];
extern char gRequestBuffer[1024];
extern TEHandle gResponseText;
extern ControlHandle gProtocolRadio[2];
extern SSLState gSSLState;
extern ProtocolType gProtocolType;
extern ControlHandle gVertScrollBar;
extern short gLogFileRefNum;

#endif