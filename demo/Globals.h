/* Globals.h - only declarations, no initialization */
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

/* Constants */
#define kControlButtonPart 10  // Part code for a button control
#define kFontIDGeneva 3
#define kAppleMenuID 128
#define kFileMenuID 129
#define kEditMenuID 130
#define kEditSelectAll 1
#define kEditCopy 3

/* Protocol Type defined in ProtocolTypes.h */

/* Global variables declarations */
extern char requestBuffer[1024];
extern WindowPtr gMainWindow;
extern ControlHandle gConnectButton;
extern MenuHandle gAppleMenu;
extern MenuHandle gFileMenu;
extern MenuHandle gEditMenu;
extern InetSvcRef gInetService;
extern char gResponseBuffer[RESPONSE_BUFFER_SIZE];
extern TEHandle gResponseText;
extern SSLState gSSLState;
extern ControlHandle gVertScrollBar;

/* Macintosh Toolbox stuff */
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