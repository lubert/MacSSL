#ifndef GLOBALS_H
#define GLOBALS_H

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#include "../src/ssl/SSLWrapper.h"
#include "../src/common/ProtocolTypes.h"

#define kControlButtonPart 10
#define kFontIDGeneva 3
#define kAppleMenuID 128
#define kFileMenuID 129
#define kEditMenuID 130
#define kEditSelectAll 1
#define kEditCopy 3

#define kHTTPMethodGET 0
#define kHTTPMethodPOST 1
#define kHTTPMethodPUT 2
#define kHTTPMethodDELETE 3

#define kHTTPMethodMenuID 4000

extern char requestBuffer[1024];
extern WindowPtr gMainWindow;
extern ControlHandle gSendButton;
extern ControlHandle gMethodPopup;
extern MenuHandle gAppleMenu;
extern MenuHandle gFileMenu;
extern MenuHandle gEditMenu;
extern MenuHandle gHTTPMethodMenu;
extern short gSelectedHTTPMethod;
extern InetSvcRef gInetService;
extern char gResponseBuffer[RESPONSE_BUFFER_SIZE];
extern TEHandle gResponseText;
extern SSLState gSSLState;
extern ControlHandle gVertScrollBar;

#ifndef inDesk
#define inDesk 0
#define inMenuBar 1
#define inSysWindow 2
#define inContext 3
#define inDrag 4
#define inGrow 5
#define inGoAway 6
#define inZoomIn 7
#define inZoomOut 8
#define inControl 10
#define inProxyIcon 11
#endif


#endif