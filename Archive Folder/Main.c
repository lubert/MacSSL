/* 
 * 640by480 Classic Mac Client - Basic Networking Version
 *  
 * A simple client for the 640by480 photo sharing service
 * Built for CodeWarrior Pro 4, targeting Mac OS 7.1-9.2
 *
 * "Insufficiently advanced technology
 * is indistinguishable from autism"
 */

Boolean gDone = false;
Boolean gNetworkInitialized = false;

/* Include necessary headers in proper order */
#include "Globals.h"
#include <Types.h>
#include "api.h"
#include "base64.h"

/* Standard Mac Headers */
#include <Files.h>
#include <ansi_prefix.mac.h>
#include <stdarg.h>
#include <stdlib.h>
#include <Quickdraw.h>
#include <Fonts.h>
#include <Events.h>
#include <Windows.h>
#include <Controls.h>
#include <Dialogs.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Memory.h>
#include <OSUtils.h>
#include <Strings.h>
#include <stdio.h>
#include <string.h>
#include "mac_stdint.h"
#include "mac_entropy.h"


#include "ssl.h"
#include "md5.h"
#include "sha1.h"

/* Networking Headers */
#include <OpenTransport.h>
#include <OpenTptInternet.h>
#include "SSLWrapper.h"
#include "Logging.h"

/* Global variable definitions */
MenuHandle gFileMenu;
MenuHandle gEditMenu;
WindowPtr gMainWindow = NULL;
ControlHandle gConnectButton = NULL;
EndpointRef gTCPEndpoint = kOTInvalidEndpointRef;
InetSvcRef gInetService = kOTInvalidProviderRef;
char gResponseBuffer[MAX_RESPONSE_SIZE];
char gRequestBuffer[1024];   /* Request buffer for HTTP requests */
TEHandle gResponseText = NULL;
ControlHandle gProtocolRadio[2];  /* Radio buttons for HTTP/HTTPS selection */
SSLState gSSLState;
ProtocolType gProtocolType = kProtocolHTTPS;
ControlHandle gVertScrollBar = NULL;
short gLogFileRefNum = 0;

/* Function prototypes */
void InitializeToolbox(void);
void SetupMenus(void);
void HandleRadioClick(ControlHandle control);
void HandleMenuChoice(long menuChoice);
void HandleEvent(EventRecord *event);
void HandleMouseDown(EventRecord *event);
void SetupWindow(void);
void DoUpdate(WindowPtr window);
OSStatus InitializeNetwork(void);
OSStatus CheckSSLLibrary(LoggingCallback logFunc);
void CleanupNetwork(void);
OSStatus ConnectToServer(void);
void DisplayResponse(char* response, long responseLength);
void dummy_function(void);
void AppendLogText(const char* message);
void ClearLogText(void);
void LogTextf(const char* format, ...);
void LogMessage(const char* message);
void ClearLog(void);
void LogMessagef(const char* format, ...);
void LogHTTPRequest(const char* requestBuffer, LoggingCallback logFunc);
void LogHTTPResponse(const char* responseBuffer, long responseLength, LoggingCallback logFunc);
void CopyTextToClipboard(TEHandle textH);
OSStatus WriteResponseToFile(char* buffer, long bufferLength);


/* Main event loop */
int main(void)
{
    EventRecord event;
    OSStatus err;
    
    /* Initialize the application */
    InitializeToolbox();
    SetupMenus();
    SetupWindow();
    
    /* Initialize networking */
    err = InitializeNetwork();
    if (err != noErr) {
        /* Show error dialog */
        SysBeep(2);
    }
    
    /* Enter main event loop */
    while (!gDone) {
        if (WaitNextEvent(everyEvent, &event, 0, NULL)) {
            HandleEvent(&event);
        }
    }
    
    /* Clean up */
    CleanupNetwork();
    
    /* Clean up text handle if it exists */
    if (gResponseText != NULL) {
        TEDispose(gResponseText);
    }
    
    return 0;
}

/* Initialize Mac Toolbox managers */
void InitializeToolbox(void)
{
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();
}

/* Setup application menus */
void SetupMenus(void)
{
    /* File menu */
    gFileMenu = NewMenu(128, "\pFile");
    AppendMenu(gFileMenu, "\pQuit/Q");
    InsertMenu(gFileMenu, 0);
    
    /* Edit menu */
    gEditMenu = NewMenu(kEditMenuID, "\pEdit");
    AppendMenu(gEditMenu, "\pSelect All/A");
    AppendMenu(gEditMenu, "\p-");
    AppendMenu(gEditMenu, "\pCopy/C");
    InsertMenu(gEditMenu, 0);
    
    /* Draw menu bar */
    DrawMenuBar();
}


/* Create and setup main window with properly configured radio buttons */
void SetupWindow(void)
{
    Rect windowRect;
    Rect buttonRect;
    Rect radioRect1, radioRect2;
    Rect textRect;
    Rect visibleTextRect;
    Rect scrollBarRect;
    
    /* Create main window with specified dimensions */
    SetRect(&windowRect, 50, 50, 500, 300);
    gMainWindow = NewWindow(NULL, &windowRect, "\p640by480 Client", true, documentProc,
                            (WindowPtr)-1, true, 0);
    
    if (gMainWindow != NULL) {
        /* Set as active window */
        SetPort(gMainWindow);
        
        /* Create protocol radio buttons */
        /* Position first radio button (HTTP) */
        SetRect(&radioRect1, 10, 10, 80, 30);
        
        /* Position second radio button (HTTPS) with clear separation */
        SetRect(&radioRect2, 100, 10, 180, 30);
        
        /* Create HTTP radio button with correct initial state */
        gProtocolRadio[kProtocolHTTP] = NewControl(
            gMainWindow, 
            &radioRect1, 
            "\pHTTP",
            true, 
            (gProtocolType == kProtocolHTTP) ? 1 : 0,
            0, 1, radioButProc, 0
        );
        
        /* Create HTTPS radio button with correct initial state */
        gProtocolRadio[kProtocolHTTPS] = NewControl(
            gMainWindow, 
            &radioRect2, 
            "\pHTTPS",
            true, 
            (gProtocolType == kProtocolHTTPS) ? 1 : 0,
            0, 1, radioButProc, 0
        );
        
        /* Create connect button - position it to the right of the radio buttons */
        SetRect(&buttonRect, 200, 10, 340, 30);
        gConnectButton = NewControl(gMainWindow, &buttonRect, "\pConnect To Server",
                              true, 0, 0, 0, pushButProc, kControlButtonPart);
                                           
        /* Create text edit field for response - position it below the controls */
        SetRect(&textRect, 10, 40, 420, 250);
            
        visibleTextRect = textRect;
        InsetRect(&visibleTextRect, 5, 5);
            
        gResponseText = TENew(&textRect, &visibleTextRect);
            
        if (gResponseText != NULL) {
        	
        	/* Set up scrolling */
        	TEAutoView(true, gResponseText);
        	
        	/* Initialize scrollbar controls */
        	SetRect(&scrollBarRect, textRect.right +1, textRect.top,
        		textRect.right + 16, textRect.bottom);
        	gVertScrollBar = NewControl(gMainWindow, &scrollBarRect, "\p",
        						true, 0,0,0, scrollBarProc, 0);
        
            /* Set initial text */
            AppendLogText("Press 'Connect to Server' to fetch photos from 640by480.com");
            
            /* Make it look better - set font and add a border */
            TextFont(kFontIDGeneva);
            TextSize(10);
            
            /* Draw border around text area */
            PenSize(1, 1);
            FrameRect(&textRect);
        }
    }
}


/* Handle radio button clicks with callback logging */
void HandleRadioClick(ControlHandle control)
{
    OSStatus err;
    
    /* First, check which control was clicked */
    if (control == gProtocolRadio[kProtocolHTTP]) {
        /* Only switch if we're not already in HTTP mode */
        if (gProtocolType != kProtocolHTTP) {
            ClearLogText();
            AppendLogText("Switching to HTTP protocol");
            
            /* Switch to HTTP */
            gProtocolType = kProtocolHTTP;
            
            /* Update control values for both radio buttons */
            SetControlValue(gProtocolRadio[kProtocolHTTP], 1);
            SetControlValue(gProtocolRadio[kProtocolHTTPS], 0);
            
            /* Clean up SSL if initialized */
            if (gSSLState.initialized) {
                AppendLogText("Closing existing SSL connection");
                SSL_Close(&gSSLState);
            }
            
            AppendLogText("Now using HTTP protocol");
        }
    }
    else if (control == gProtocolRadio[kProtocolHTTPS]) {
        /* Only switch if we're not already in HTTPS mode */
        if (gProtocolType != kProtocolHTTPS) {
            ClearLogText();
            AppendLogText("Switching to HTTPS protocol");
            
            /* Check if SSL library is available */
            AppendLogText("Checking SSL library availability...");
            if (CheckSSLLibrary(AppendLogText) != noErr) {
                AppendLogText("SSL library check failed. Cannot use HTTPS.");
                
                /* Revert to HTTP */
                gProtocolType = kProtocolHTTP;
                SetControlValue(gProtocolRadio[kProtocolHTTP], 1);
                SetControlValue(gProtocolRadio[kProtocolHTTPS], 0);
                return;
            }
            
            /* Update control values for both radio buttons */
            SetControlValue(gProtocolRadio[kProtocolHTTP], 0);
            SetControlValue(gProtocolRadio[kProtocolHTTPS], 1);
            
            /* Initialize SSL - this will show detailed logs via callback */
            AppendLogText("Starting SSL initialization");
            err = SSL_Initialize(&gSSLState, AppendLogText);
            
            if (err != noErr) {
                /* Failed to initialize - switch back to HTTP with descriptive error */
                char errorMsg[100];
                sprintf(errorMsg, "SSL initialization failed (error %d)", (int)err);
                AppendLogText(errorMsg);
                AppendLogText("Reverting to HTTP protocol");
                
                gProtocolType = kProtocolHTTP;
                SetControlValue(gProtocolRadio[kProtocolHTTP], 1);
                SetControlValue(gProtocolRadio[kProtocolHTTPS], 0);
            } else {
                /* Successfully initialized */
                gProtocolType = kProtocolHTTPS;
                AppendLogText("HTTPS protocol successfully activated");
            }
        }
    }
}

/* Handle menu selections */
void HandleMenuChoice(long menuChoice)
{
    short menu = HiWord(menuChoice);
    short item = LoWord(menuChoice);
    
    TEInit();
	ZeroScrap();
    
    switch (menu) {
        case 128: /* File menu */
            switch (item) {
                case 1: /* Quit */
                    gDone = true;
                    break;
            }
            break;
            
        case kEditMenuID:
        	switch (item) {
        		case kEditSelectAll:
        			if(gResponseText != NULL) {
        				TESetSelect(0, (*gResponseText)->teLength, gResponseText);
        				TEUpdate(&(*gResponseText)->viewRect, gResponseText);
        			}
        			break;
        			
        		case kEditCopy:
        			if(gResponseText != NULL &&
        				(*gResponseText)->selEnd > (*gResponseText)->selStart) {
        				CopyTextToClipboard(gResponseText);
        			}
        			break;
        	}
        	break;
    }
    
    HiliteMenu(0);
}

/* Handle Mac OS events */
void HandleEvent(EventRecord *event)
{
    WindowPtr window;
    short part;
    ControlHandle control;
    Point mousePoint;
    char key;
    
    switch (event->what) {
        case mouseDown:
            HandleMouseDown(event);
            break;
            
        case keyDown:
        case autoKey:
        	key = (char)(event->message & charCodeMask);
        
            /* Handle keyboard shortcuts */
            if (event->modifiers & cmdKey) {
            	if(key == 'a' || key == 'A') {
            		if(gResponseText != NULL) {
            			TESetSelect(0, (*gResponseText)->teLength, gResponseText);
            			TEUpdate(&(*gResponseText)->viewRect, gResponseText);
            		}
            	}
            	else if(key == 'c' || key == 'C') {
            		if(gResponseText != NULL &&
            			(*gResponseText)->selEnd > (*gResponseText)->selStart) {
            			
            			TECopy(gResponseText);
            		}
            	}
            	else {
            		HandleMenuChoice(MenuKey(key));
            	}
            }
            break;
            
        case updateEvt:
            window = (WindowPtr)event->message;
            BeginUpdate(window);
            DoUpdate(window);
            EndUpdate(window);
            break;
            
        case activateEvt:
            window = (WindowPtr)event->message;
            if (event->modifiers & activeFlag) {
                /* Window being activated */
                if (window == gMainWindow && gResponseText != NULL) {
                    TEActivate(gResponseText);
                }
            } else {
                /* Window being deactivated */
                if (window == gMainWindow && gResponseText != NULL) {
                    TEDeactivate(gResponseText);
                }
            }
            break;
    }
}

/* Handle mouse down events with better radio button handling */
void HandleMouseDown(EventRecord *event)
{
    WindowPtr window;
    short part;
    long menuChoice;
    ControlHandle control;
    short controlPart;
    Point mousePoint;
    
    /* Find which window was clicked */
    part = FindWindow(event->where, &window);
    
    switch (part) {
        case inMenuBar:
            menuChoice = MenuSelect(event->where);
            if (menuChoice != 0) {
                HandleMenuChoice(menuChoice);
            }
            break;
            
        case inDrag:
            DragWindow(window, event->where, &qd.screenBits.bounds);
            break;
            
        case inGoAway:
            if (TrackGoAway(window, event->where)) {
                if (window == gMainWindow) {
                    gDone = true;
                } else {
                    DisposeWindow(window);
                }
            }
            break;
            
        case inContent:
            if (window != FrontWindow()) {
                SelectWindow(window);
            } else {
                /* Convert global coordinates to local */
                mousePoint = event->where;
                GlobalToLocal(&mousePoint);
                
                /* Find which control was clicked (if any) */
                controlPart = FindControl(mousePoint, window, &control);
                
                if (controlPart) {
                    /* Track the control click */
                    controlPart = TrackControl(control, mousePoint, NULL);
                    
                    /* Handle the click result if the control was actually clicked */
                    if (controlPart) {
                        /* Connect button */
                        if (control == gConnectButton) {
                            ConnectToServer();
                        }
                        /* HTTP radio button */
                        else if (control == gProtocolRadio[kProtocolHTTP]) {
                            HandleRadioClick(gProtocolRadio[kProtocolHTTP]);
                        }
                        /* HTTPS radio button */
                        else if (control == gProtocolRadio[kProtocolHTTPS]) {
                            HandleRadioClick(gProtocolRadio[kProtocolHTTPS]);
                        }
                    }
                }
                
                /* Handle clicks in text field */
                if (gResponseText != NULL &&
                    PtInRect(mousePoint, &(*gResponseText)->viewRect)) {
                    TEClick(mousePoint, (event->modifiers & shiftKey) != 0, gResponseText);
                }
            }
            break;
        
        /* handle the scrollbar */    
        case inControl:
        	if(control == gVertScrollBar){
        		switch(controlPart) {
        			case kControlUpButtonPart:
        				TEScroll(0, -16, gResponseText);
        				break;
        			
        			case kControlDownButtonPart:
        				TEScroll(0, 16, gResponseText);
        				break;
        			
        			case kControlPageUpPart:
        				TEScroll(0, -(((*gResponseText)->viewRect.bottom -
        							(*gResponseText)->viewRect.top) - 16),
        							gResponseText);
        				break;
        				
        			case kControlIndicatorPart:
        				TEScroll(0, (*gResponseText)->destRect.top +
        							GetControlValue(gVertScrollBar) -
        							(*gResponseText)->viewRect.top,
        							gResponseText);
        				break;		
        		}	
        				
        		/* Update scrollbar update after scrolling */
        		SetControlValue(gVertScrollBar,
        						(*gResponseText)->viewRect.top -
        						(*gResponseText)->destRect.top);
        						
        	}
        	break;
        
    }
}
/* Update the window contents */
void DoUpdate(WindowPtr window)
{
    Rect textBorderRect;
    
    if (window == gMainWindow) {
        /* Redraw our controls */
        UpdateControls(window, window->visRgn);
        
        /* Redraw the text */
        if (gResponseText != NULL) {
            TEUpdate(&(*gResponseText)->viewRect, gResponseText);
            
            /* Redraw border around text area */
            textBorderRect = (*gResponseText)->viewRect;
            InsetRect(&textBorderRect, -5,-5);
            PenNormal();
            FrameRect(&textBorderRect);
            
            /* Add 3D effect */
            PenPat(&qd.gray);
            FrameRect(&textBorderRect);
            PenNormal();
        }
        
        if(gVertScrollBar != NULL) {
        	DrawControls(window);
        }
    }
}


/* Initialize Open Transport and TCP/IP */
OSStatus InitializeNetwork(void){
    OSStatus err = noErr;
    
    if (gNetworkInitialized) {
        return noErr;
    }
    
    /* Initialize Open Transport */
    err = InitOpenTransport();
    if (err != noErr) {
        if (gResponseText != NULL) {
            char errMsg[100];
            sprintf(errMsg, "Failed to initialize Open Transport. Error: %d", (int)err);
            AppendLogText(errMsg);
            
        }
        return err;
    }
    
    /* Create and open Internet Services provider */
    gInetService = OTOpenInternetServices(kDefaultInternetServicesPath, 0, &err);
    if (err != noErr) {
        if (gResponseText != NULL) {
            char errMsg[100];
            sprintf(errMsg, "Failed to open Internet Services. Error: %d", (int)err);
            AppendLogText(errMsg);
        }
        return err;
    }
    
    /* Check if SSL library is properly linked */
    if (gResponseText != NULL) {
        ClearLogText();
        AppendLogText("Checking SSL library availability...");
    }
    
    /* Call CheckSSLLibrary with AppendLogText as the callback */
    err = CheckSSLLibrary(AppendLogText);
    if (err != noErr) {
        AppendLogText("SSL library check failed. HTTPS will not be available.");
        /* Continue anyway - HTTPS might not work */
    }
    
    /* Initialize SSL if using HTTPS */
    if (gProtocolType == kProtocolHTTPS) {
        /* Show status message */
        if (gResponseText != NULL) {
            AppendLogText("Initializing SSL...");
        }
        
        /* Initialize SSL with AppendLogText as the callback */
        err = SSL_Initialize(&gSSLState, AppendLogText);
        if (err != noErr) {
            if (gResponseText != NULL) {
                char errMsg[100];
                sprintf(errMsg, "SSL initialization failed. Error: %d", (int)err);
                AppendLogText(errMsg);
            }
            
            /* We'll continue without SSL and let user switch to HTTP */
            if (gResponseText != NULL) {
                AppendLogText("SSL failed to initialize. Please use HTTP mode instead.");
            }
            
            /* Force protocol to HTTP */
            gProtocolType = kProtocolHTTP;
            if (gProtocolRadio[kProtocolHTTP] != NULL) {
                SetControlValue(gProtocolRadio[kProtocolHTTP], 1);
            }
            if (gProtocolRadio[kProtocolHTTPS] != NULL) {
                SetControlValue(gProtocolRadio[kProtocolHTTPS], 0);
            }
        }
        else if (gResponseText != NULL) {
            AppendLogText("SSL initialized successfully.");
        }
    }
    
    gNetworkInitialized = true;
    
    /* Show success message */
    if (gResponseText != NULL) {
        AppendLogText("Network initialized successfully.");
    }
    
    return noErr;
}

/* Clean up Open Transport */
void CleanupNetwork(void)
{
	CloseLogFile();

    /* Close SSL connection if active */
    if (gProtocolType == kProtocolHTTPS) {
        SSL_Close(&gSSLState);
    }
    
    /* Close regular TCP endpoint if active */
    if (gTCPEndpoint != kOTInvalidEndpointRef) {
        OTCloseProvider(gTCPEndpoint);
        gTCPEndpoint = kOTInvalidEndpointRef;
    }
    
    /* Close internet service */
    if (gInetService != kOTInvalidProviderRef) {
        OTCloseProvider(gInetService);
        gInetService = kOTInvalidProviderRef;
    }
    
    CloseOpenTransport();
    gNetworkInitialized = false;
}

/* Connect to the server and fetch data */
OSStatus ConnectToServer(void)
{
    OSStatus err = noErr;
    InetHostInfo hostInfo;
    InetAddress inAddr;
    unsigned long responseLength = 0;
    char *bodyStart;
    unsigned long bodyLength;
    size_t bytesSent = 0;
    size_t bytesReceived = 0;
    char connectMsg[50];
    OTResult sendResult;
    OTResult recvResult;
    int readAttempts;
	const int maxReadAttempts = 10;
	const char *simple_request = "GET / HTTP/1.0\r\nHost: httpbin.org\r\n\r\n";
    
    /* Show wait cursor */
    SetCursor(*GetCursor(watchCursor));
    
    /* Reset response text */
    if (gResponseText != NULL) {
        AppendLogText("Connecting to server");
    }
    
    /* Clear existing connections if any */
    if (gProtocolType == kProtocolHTTPS) {
        SSL_Close(&gSSLState);
        err = SSL_Initialize(&gSSLState, AppendLogText);
        if (err != noErr) {
            if (gResponseText != NULL) {
                AppendLogText("SSL Init failed");
            }
            SetCursor(&qd.arrow);
            return err;
        }
    } else {
        if (gTCPEndpoint != kOTInvalidEndpointRef) {
            OTCloseProvider(gTCPEndpoint);
            gTCPEndpoint = kOTInvalidEndpointRef;
        }
    }
    
    /* Look up the host address */
    err = OTInetStringToAddress(gInetService, (char*)API_HOST, &hostInfo);
    if (err != noErr) {
        if (gResponseText != NULL) {
            AppendLogText("Could not resolve host address");
        }
        SetCursor(&qd.arrow);
        return err;
    }
    
    /* Set up the address for the remote host with correct port based on protocol */
    if (gProtocolType == kProtocolHTTPS) {
        OTInitInetAddress(&inAddr, API_PORT, hostInfo.addrs[0]);
        strcpy(connectMsg, "Connecting using HTTPS...");
    } else {
        OTInitInetAddress(&inAddr, API_PORT_HTTP, hostInfo.addrs[0]);
        strcpy(connectMsg, "Connecting using HTTP...");
    }
    
    /* Update status */
    if (gResponseText != NULL) {
        AppendLogText("Connecting...");
    }
    
    /* Connect based on protocol type */
    if (gProtocolType == kProtocolHTTPS) {
        /* Use SSL for HTTPS connection */
        err = SSL_Connect(&gSSLState, &inAddr, gResponseText, AppendLogText);
        if (err != noErr) {
            if (gResponseText != NULL) {
                char errorMsg[80];
                sprintf(errorMsg, "Error: SSL connection failed (code %d)", (int)err);
                AppendLogText(errorMsg);
            }
            SSL_Close(&gSSLState);
            SetCursor(&qd.arrow);
            return err;
        }
    } else {
        /* Use standard TCP for HTTP connection */
        TBind bindReq;
        TCall sndCall;
        
        /* Create the endpoint */
        gTCPEndpoint = OTOpenEndpoint(OTCreateConfiguration(kTCPName), 0, NULL, &err);
        if (err != noErr) {
            if (gResponseText != NULL) {
                AppendLogText("Error: Could not create network endpoint");
            }
            SetCursor(&qd.arrow);
            return err;
        }
        
        /* Bind the endpoint */
        bindReq.addr.maxlen = 0;
        bindReq.addr.len = 0;
        bindReq.addr.buf = NULL;
        bindReq.qlen = 0;
        
        err = OTBind(gTCPEndpoint, &bindReq, NULL);
        if (err != noErr) {
            if (gResponseText != NULL) {
                AppendLogText("Error: Could not bind network endpoint");
            }
            OTCloseProvider(gTCPEndpoint);
            gTCPEndpoint = kOTInvalidEndpointRef;
            SetCursor(&qd.arrow);
            return err;
        }
        
        /* Set up the connection call structure */
        sndCall.addr.maxlen = sizeof(InetAddress);
        sndCall.addr.len = sizeof(InetAddress);
        sndCall.addr.buf = (UInt8*)&inAddr;
        
        sndCall.opt.maxlen = 0;
        sndCall.opt.len = 0;
        sndCall.opt.buf = NULL;
        
        sndCall.udata.maxlen = 0;
        sndCall.udata.len = 0;
        sndCall.udata.buf = NULL;
        
        /* Connect to the server */
        err = OTConnect(gTCPEndpoint, &sndCall, NULL);
        if (err != kOTNoDataErr && err != noErr) {
            if (gResponseText != NULL) {
                AppendLogText("Error: Connection failed");
            }
            OTCloseProvider(gTCPEndpoint);
            gTCPEndpoint = kOTInvalidEndpointRef;
            SetCursor(&qd.arrow);
            return err;
        }
        
        /* Check if we need to finish the connection asynchronously */
        if (err == kOTNoDataErr) {
            OTResult result;
            result = OTLook(gTCPEndpoint);
            
            if (result == T_CONNECT) {
                /* Accept the connection and finish connecting */
                err = OTRcvConnect(gTCPEndpoint, NULL);
                if (err != noErr) {
                    if (gResponseText != NULL) {
                        AppendLogText("Error: Connection failed");
                    }
                    OTCloseProvider(gTCPEndpoint);
                    gTCPEndpoint = kOTInvalidEndpointRef;
                    SetCursor(&qd.arrow);
                    return err;
                }
            } else {
                if (gResponseText != NULL) {
                    AppendLogText("Error: Connection state error");
                }
                OTCloseProvider(gTCPEndpoint);
                gTCPEndpoint = kOTInvalidEndpointRef;
                SetCursor(&qd.arrow);
                return kOTStateChangeErr;
            }
        }
    }
    
    /* Update status */
    if (gResponseText != NULL) {
        AppendLogText("Connected. Sending request...");
    }
    
	/* Format the HTTP request */
	AppendLogText("Preparing HTTP request...");

	/* Clear the request buffer */
	memset(gRequestBuffer, 0, sizeof(gRequestBuffer));

	/* Basic request line */
	sprintf(gRequestBuffer, "GET %s HTTP/1.0\r\n", API_PATH);

	/* Add Host header - required for virtual hosting */
	sprintf(gRequestBuffer + strlen(gRequestBuffer), "Host: %s\r\n", API_HOST);

	/* Add User-Agent */
	sprintf(gRequestBuffer + strlen(gRequestBuffer), "User-Agent: 640by480-ClassicMacClient/1.0\r\n");

	/* Content type we're willing to accept */
	sprintf(gRequestBuffer + strlen(gRequestBuffer), "Accept: */*\r\n");

	/* Disable keep-alive to ensure connection closes after response */
	sprintf(gRequestBuffer + strlen(gRequestBuffer), "Connection: close\r\n");

	/* End of headers */
	sprintf(gRequestBuffer + strlen(gRequestBuffer), "\r\n");

	/* Log the request for debugging */
	LogHTTPRequest(gRequestBuffer, AppendLogText);

	/* Send the request based on protocol type */
	AppendLogText("Sending HTTP request...");
    
    /* Send the request based on protocol type */
    if (gProtocolType == kProtocolHTTPS) {
        // THIS IS WHERE WE'RE SIMPLIFYING
        
        //err = SSL_Send(&gSSLState, simple_request, strlen(simple_request), &bytesSent, AppendLogText);
        err = SSL_Send(&gSSLState, gRequestBuffer, strlen(gRequestBuffer), &bytesSent, AppendLogText);
        if (err != noErr) {
        	char errMsg[100];
        	sprintf(errMsg, "Error: Failed to send request (code %d, sent %lu of %lu bytes)", 
        	        (int)err, (unsigned long)bytesSent, (unsigned long)strlen(gRequestBuffer));
        	AppendLogText(errMsg);
        
        	/* Show what was actually sent, if anything */
        	if (bytesSent > 0) {
         	   sprintf(errMsg, "Partial request sent (%lu bytes)", (unsigned long)bytesSent);
        	    AppendLogText(errMsg);
        	}
        
        	SSL_Close(&gSSLState);
        	SetCursor(&qd.arrow);
        	return err;
    	}
    } else {
        sendResult = OTSnd(gTCPEndpoint, gRequestBuffer, strlen(gRequestBuffer), 0);
        if (sendResult < 0) {
            err = sendResult;
        }
    }
    
    if (err != noErr) {
        if (gResponseText != NULL) {
            AppendLogText("Error: Failed to send request");
        }
        if (gProtocolType == kProtocolHTTPS) {
            SSL_Close(&gSSLState);
        } else {
            OTCloseProvider(gTCPEndpoint);
            gTCPEndpoint = kOTInvalidEndpointRef;
        }
        SetCursor(&qd.arrow);
        return err;
    }
    
    /* Update status */
    if (gResponseText != NULL) {
        AppendLogText("Request sent. Waiting for response...");
    }
    
    /* Receive the response */
responseLength = 0;
memset(gResponseBuffer, 0, MAX_RESPONSE_SIZE);  /* Clear the response buffer */

AppendLogText("Request sent. Waiting for response...");

/* Number of read attempts */
readAttempts = 0;


while (responseLength < MAX_RESPONSE_SIZE) {
    if (gProtocolType == kProtocolHTTPS) {
        /* Use SSL for HTTPS connection */
        err = SSL_Receive(&gSSLState, 
                         gResponseBuffer + responseLength,
                         MAX_RESPONSE_SIZE - responseLength, 
                         &bytesReceived, 
                         AppendLogText);
        
        if (err != noErr && err != kOTNoDataErr) {
            char errorMsg[100];
            sprintf(errorMsg, "Error receiving SSL data (code %d)", (int)err);
            AppendLogText(errorMsg);
            break;
        }
    } else {
        /* Use standard TCP for HTTP connection */
        recvResult = OTRcv(gTCPEndpoint, 
                          gResponseBuffer + responseLength,
                          MAX_RESPONSE_SIZE - responseLength, 
                          0);
        
        if (recvResult > 0) {
            bytesReceived = recvResult;
        } else if (recvResult == 0) {
            /* Connection closed */
            AppendLogText("Connection closed by server");
            bytesReceived = 0;
        } else {
            if (recvResult == kOTNoDataErr) {
                /* No data available yet, try again */
                readAttempts++;
                if (readAttempts % 3 == 0) {
                    char msg[100];
                    sprintf(msg, "Waiting for data... (attempt %d)", readAttempts);
                    AppendLogText(msg);
                }
                
                if (readAttempts >= maxReadAttempts) {
                    AppendLogText("Too many retries, giving up");
                    break;
                }
                
                continue;
            } else {
                /* Error */
                char errorMsg[100];
                sprintf(errorMsg, "Error receiving data (code %d)", (int)recvResult);
                AppendLogText(errorMsg);
                err = recvResult;
                break;
            }
        }
    }
    
    /* Log progress */
    if (bytesReceived > 0) {
        char msg[100];
        responseLength += bytesReceived;
        sprintf(msg, "Received %d bytes (total: %ld bytes)", (int)bytesReceived, (long)responseLength);
        AppendLogText(msg);
        
        /* Reset the retry counter since we got data */
        readAttempts = 0;
    } else if (bytesReceived == 0) {
        /* End of data */
        AppendLogText("End of data reached");
        break;
    } else {
        /* No data received this time */
        readAttempts++;
        if (readAttempts % 3 == 0) {
            char msg[100];
            sprintf(msg, "Waiting for more data... (attempt %d)", readAttempts);
            AppendLogText(msg);
        }
        
        if (readAttempts >= maxReadAttempts) {
            AppendLogText("Too many retries, ending receive operation");
            break;
        }
    }
}
    
    /* Close the connection */
    if (gProtocolType == kProtocolHTTPS) {
        SSL_Close(&gSSLState);
    } else {
        OTCloseProvider(gTCPEndpoint);
        gTCPEndpoint = kOTInvalidEndpointRef;
    }
    
    /* Process the response */
    if (responseLength > 0) {
        /* Find the start of the HTTP body (after the headers) */
        bodyStart = strstr(gResponseBuffer, "\r\n\r\n");
        if (bodyStart != NULL) {
            bodyStart += 4; /* Skip the header-body separator */
            
            /* Calculate body length */
            bodyLength = responseLength - (bodyStart - gResponseBuffer);
            
            /* Display the response body */
            DisplayResponse(bodyStart, bodyLength);
        } else {
            /* No body separator found - display the whole response */
            DisplayResponse(gResponseBuffer, responseLength);
        }
    } else {
        /* No data received */
        if (gResponseText != NULL) {
            AppendLogText("Error: No response received from server");
        }
    }
    
    /* Restore normal cursor */
    SetCursor(&qd.arrow);
    
    return err;
}

/* Modified DisplayResponse function to also write to a file */
void DisplayResponse(char* response, long responseLength) {
    char* displayText;
    long maxLength = 30000;
    long startPos = 0;
    
    /* First, try to write the complete response to a file 
    OSStatus fileErr = WriteResponseToFile(response, responseLength);*/
    
    /* Process text for display */
    if (gResponseText != NULL && response != NULL) {
        /* If response is longer than our limit, take only the last 30K */
        if (responseLength > maxLength) {
            startPos = responseLength - maxLength;
            responseLength = maxLength;
        }
        
        /* Make a null-terminated copy */
        displayText = NewPtr(responseLength + 1);
        if (displayText != NULL) {
            /* Copy from the calculated start position */
            BlockMoveData(response + startPos, displayText, responseLength);
            displayText[responseLength] = '\0';
            
            /* Clear existing text and set new text */
            TESetText(displayText, responseLength, gResponseText);
            
            /* Clean up */
            DisposePtr(displayText);
        }
        
        /* Update scrollbar */
        if (gVertScrollBar != NULL) {
            SetControlMaximum(gVertScrollBar, 
                TEGetHeight(0, (*gResponseText)->nLines, gResponseText) -
                ((*gResponseText)->viewRect.bottom - (*gResponseText)->viewRect.top));
            SetControlValue(gVertScrollBar, 0);  // Reset to top
        }
    }
}

/* Check if the SSL library is properly linked and available */
OSStatus CheckSSLLibrary(LoggingCallback logFunc)
{
    /* Variables used in this function */
    int functions_ok = 0;
    int initialized = 0;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    int ret;
    char msgBuf[100];
    unsigned char enhanced_test_seed[32];
    
    if (logFunc) logFunc("Testing SSL Library...");
    
    /* Step 1: Test simple function calls */
    if (logFunc) logFunc("Step 1: Testing basic function availability");
    
    /* Try to initialize entropy - if this crashes, we have linking issues */
    mbedtls_entropy_init(&entropy);
    initialized |= 1;
    if (logFunc) logFunc("  mbedtls_entropy_init: OK");
    
    /* Try to initialize RNG - if this crashes, we have linking issues */
    mbedtls_ctr_drbg_init(&ctr_drbg);
    initialized |= 2;
    if (logFunc) logFunc("  mbedtls_ctr_drbg_init: OK");
    
    /* Basic functions seem available */
    functions_ok = 1;
    
    /* Step 2: Test RNG seeding */
    if (functions_ok) {
        if (logFunc) logFunc("Step 2: Testing RNG seeding");
       
		/* Fill with entropy from our custom gatherer */
		mac_entropy_gather(enhanced_test_seed, sizeof(enhanced_test_seed));
		
		/* show a sample of the entropy */
		if (logFunc)
		{
			sprintf(msgBuf, " Test seed preview: %02X %02X %02X %02X %02X %02X %02X %02X",
				enhanced_test_seed[0], enhanced_test_seed[1], enhanced_test_seed[2], enhanced_test_seed[3], 
				enhanced_test_seed[4], enhanced_test_seed[5], enhanced_test_seed[6], enhanced_test_seed[7]);
				logFunc(msgBuf);
		}
		
		/* Attempt to seed the RNG with our entropy */
		ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
			enhanced_test_seed, sizeof(enhanced_test_seed));
			
		if(ret == 0)
		{
			if(logFunc) logFunc(" RNG seeding OKAY");
		}
		else {
			sprintf(msgBuf, "ERROR: RNG seeding test failed: %d", ret);
			if(logFunc) logFunc(msgBuf);
			goto cleanup;
		}
    }
    
    /* Step 3: Test random number generation */
    if (functions_ok) {
        unsigned char random_buf[16];
        
        if (logFunc) logFunc("Step 3: Testing random number generation");
        
        /* Try to generate random data */
        ret = mbedtls_ctr_drbg_random(&ctr_drbg, random_buf, sizeof(random_buf));
        
        if (ret == 0) {
            if (logFunc) logFunc("  Random generation: OK");
            
            /* Display a few bytes of random data */
            sprintf(msgBuf, "  Random sample: %02X %02X %02X %02X",
                   random_buf[0], random_buf[1], random_buf[2], random_buf[3]);
            if (logFunc) logFunc(msgBuf);
        } else {
            sprintf(msgBuf, "  ERROR: Random generation failed: %d", ret);
            if (logFunc) logFunc(msgBuf);
            goto cleanup;
        }
    }
    
    /* Success! */
    if (logFunc) logFunc("SSL library appears to be working correctly!");
    
cleanup:
    /* Clean up resources we allocated */
    if (initialized & 2) {
        mbedtls_ctr_drbg_free(&ctr_drbg);
    }
    
    if (initialized & 1) {
        mbedtls_entropy_free(&entropy);
    }
    
    return functions_ok ? noErr : -1;
}

/* Dummy function to force linker to include necessary symbols */
void dummy_function(void){

	size_t olen;
    unsigned char buf[1];

    if(0)
    {
        /* mbedTLS core symbols */
        //mbedtls_sha256_init(NULL);
        mbedtls_ssl_init(NULL);
        mbedtls_ssl_config_init(NULL);
        mbedtls_entropy_init(NULL);
        mbedtls_ctr_drbg_init(NULL);
        
        /* Configuration functions */
        mbedtls_ssl_conf_rng(NULL, NULL, NULL);
        //mbedtls_ssl_conf_authmode(NULL, 0);
        //mbedtls_ssl_conf_min_version(NULL, 0, 0);
        //mbedtls_ssl_conf_max_version(NULL, 0, 0);
        mbedtls_ssl_conf_read_timeout(NULL, 0);
        
        /* SSL operation functions */
        mbedtls_ssl_setup(NULL, NULL);
        mbedtls_ssl_read(NULL, NULL, 0);
        mbedtls_ssl_write(NULL, NULL, 0);
        mbedtls_ssl_close_notify(NULL);
        mbedtls_ssl_free(NULL);
        mbedtls_ssl_config_free(NULL);
        mbedtls_ctr_drbg_free(NULL);
        mbedtls_entropy_free(NULL);
        mbedtls_ssl_set_hostname(NULL, NULL);
        mbedtls_ssl_set_bio(NULL, NULL, NULL, NULL, NULL);
        mbedtls_ssl_handshake(NULL);
        
        /* Random generation functions */
        mbedtls_ctr_drbg_random(NULL, NULL, 0);
        mbedtls_entropy_func(NULL, NULL, 0);
        
        /* SSL default configuration functions */
        mbedtls_ssl_config_defaults(NULL, 0, 0, 0);
        
        mbedtls_ecp_group_load(NULL, 0);
        
        /* Cipher suite info functions */
        mbedtls_ssl_get_ciphersuite(NULL);
        
        /* custom entropy function */
        mbedtls_hardware_poll(NULL, buf, 1, &olen);
        
        mbedtls_base64_decode(NULL, 0, &olen, NULL, 0);
    }
}

/* Append a log message to the text field instead of replacing content */
void AppendLogText(const char* message)
{
	short textLen;

    if (gResponseText == NULL)
        return;
    
    /* Get current text length */
    textLen = (*gResponseText)->teLength;
    
    /* Append newline if there's already text */
    if (textLen > 0) {
        TEInsert("\n", 1, gResponseText);
        textLen += 1;
    }
    
    /* Append the new message */
    TEInsert(message, strlen(message), gResponseText);
    
    /* Auto-scroll to see the latest entry */
    TESetSelect(textLen + strlen(message), textLen + strlen(message), gResponseText);
    TESelView(gResponseText);
    
    /* Update the scrollbar */
    
    if(gVertScrollBar != NULL)
    {
    	SetControlMaximum(gVertScrollBar, TEGetHeight(0, (*gResponseText)->nLines, gResponseText) -
    						((*gResponseText)->viewRect.bottom - (*gResponseText)->viewRect.top));
    	SetControlValue(gVertScrollBar, (*gResponseText)->viewRect.top - (*gResponseText)->destRect.top);
    }
    
    /* Update the display */
    TEUpdate(&(*gResponseText)->viewRect, gResponseText);
}

/* Clear the log text */
void ClearLogText(void)
{
    if (gResponseText == NULL)
        return;
    
    /* Reset text */
    AppendLogText("\n");

}

/* Log a formatted message (like printf) */
void LogTextf(const char* format, ...)
{
    char buffer[256];
    va_list args;
    
    if (gResponseText == NULL)
        return;
    
    /* Format the message */
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    /* Append it to the log */
    AppendLogText(buffer);
}

void LogMessage(const char* message)
{
	AppendLogText(message);
}

void ClearLog(void)
{
	ClearLogText();
}

void LogMessagef(const char* format, ...)
{
	char buffer[256];
	va_list args;
	
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	
	LogMessage(buffer);
	
}

/* Log HTTP request headers for debugging */
void LogHTTPRequest(const char* requestBuffer, LoggingCallback logFunc)
{
	char* headerEnd;

    if (logFunc == NULL) return;
    
    logFunc("--- HTTP Request Headers ---");
    
    /* Find the end of headers */
    headerEnd = strstr(requestBuffer, "\r\n\r\n");
    if (headerEnd == NULL) {
        /* Could not find end of headers, log the whole buffer */
        logFunc(requestBuffer);
    } else {
        /* Create a copy of just the headers */
        size_t headerLength = headerEnd - requestBuffer + 2;  /* Include the first \r\n */
        char* headerCopy = (char*)NewPtr(headerLength + 1);
        
        if (headerCopy != NULL) {
            /* Copy headers and null-terminate */
            BlockMoveData(requestBuffer, headerCopy, headerLength);
            headerCopy[headerLength] = '\0';
            
            /* Log the headers */
            logFunc(headerCopy);
            
            /* Clean up */
            DisposePtr(headerCopy);
        } else {
            /* Memory allocation failed, log what we can */
            logFunc("(Memory allocation failed, showing beginning of request)");
            logFunc(requestBuffer);
        }
    }
    
    logFunc("---------------------------");
}

/* Log HTTP response headers for debugging */
void LogHTTPResponse(const char* responseBuffer, long responseLength, LoggingCallback logFunc)
{
	char* headerEnd;
	char* tempBuf;
	

    if (logFunc == NULL) return;
    
    logFunc("--- HTTP Response Headers ---");
    
    /* Find the end of headers */
    headerEnd = strstr(responseBuffer, "\r\n\r\n");
    if (headerEnd == NULL) {
        /* Could not find end of headers, log the beginning of the response */
        char* tempBuf = (char*)NewPtr(256);
        if (tempBuf != NULL) {
            size_t len = responseLength < 255 ? responseLength : 255;
            BlockMoveData(responseBuffer, tempBuf, len);
            tempBuf[len] = '\0';
            logFunc(tempBuf);
            DisposePtr(tempBuf);
        } else {
            logFunc("(Memory allocation failed, cannot show response)");
        }
    } else {
        /* Create a copy of just the headers */
        size_t headerLength = headerEnd - responseBuffer + 2;  /* Include the first \r\n */
        char* headerCopy = (char*)NewPtr(headerLength + 1);
        
        if (headerCopy != NULL) {
            /* Copy headers and null-terminate */
            BlockMoveData(responseBuffer, headerCopy, headerLength);
            headerCopy[headerLength] = '\0';
            
            /* Log the headers */
            logFunc(headerCopy);
            
            /* Clean up */
            DisposePtr(headerCopy);
        } else {
            /* Memory allocation failed, log what we can */
            logFunc("(Memory allocation failed, showing beginning of response)");
            tempBuf = (char*)NewPtr(256);
            if (tempBuf != NULL) {
                size_t len = responseLength < 255 ? responseLength : 255;
                BlockMoveData(responseBuffer, tempBuf, len);
                tempBuf[len] = '\0';
                logFunc(tempBuf);
                DisposePtr(tempBuf);
            }
        }
    }
    
    logFunc("----------------------------");
}

/* Enhance the CopyTextToClipboard function */
void CopyTextToClipboard(TEHandle textH) {
    OSErr err;
    long scrapLen;
    GrafPtr oldPort;
    Handle textHandle;
    char *textPtr;
    long length;
    
    if(textH == NULL || (*textH)->selStart == (*textH)->selEnd)
        return; //nothing selected
    
    // Calculate selection length
    length = (*textH)->selEnd - (*textH)->selStart;
    
    // Add safety check for very large selections
    if(length > 32000)
        length = 32000;
    
    // Create a handle to hold the text
    textHandle = NewHandle(length);
    if(textHandle == NULL)
        return; // Memory allocation failed
    
    HLock(textHandle);
    textPtr = *textHandle;
    
    // Add safety check to prevent out-of-bounds access
    if((*textH)->selStart + length <= (*textH)->teLength) {
        BlockMoveData(*(*textH)->hText + (*textH)->selStart, textPtr, length);
        
        GetPort(&oldPort);
        SetPort(gMainWindow);
        
        err = ZeroScrap();
        
        err = PutScrap(length, 'TEXT', textPtr);
        
        SetPort(oldPort);
    }
    
    HUnlock(textHandle);
    DisposeHandle(textHandle);
}

/* Simple debug log file writer for OS 8.6 */
OSStatus WriteDebugLogToFile(char* buffer, long bufferLength) {
    OSErr err;
    short refNum;
    
    /* Create a standard Pascal string (length byte followed by chars) */
    unsigned char fileName[] = "\pDebug-Log.txt";
    
    /* Delete any existing file with this name (ignore errors) */
    FSDelete(fileName, 0);
    
    /* Create new file in the application directory */
    err = Create(fileName, 0, 'TEXT', 'ttxt');
    if (err != noErr && err != dupFNErr) {
        char msg[100];
        sprintf(msg, "Failed to create debug file: error %d", err);
        AppendLogText(msg);
        return err;
    }
    
    /* Open the file for writing */
    err = FSOpen(fileName, 0, &refNum);
    if (err != noErr) {
        char msg[100];
        sprintf(msg, "Failed to open debug file: error %d", err);
        AppendLogText(msg);
        return err;
    }
    
    /* Write data to file */
    err = FSWrite(refNum, &bufferLength, buffer);
    
    /* Always close the file */
    FSClose(refNum);
    
    /* Report status */
    if (err == noErr) {
        char msg[100];
        sprintf(msg, "Successfully wrote %ld bytes to Debug-Log.txt", bufferLength);
        AppendLogText(msg);
    } else {
        char msg[100];
        sprintf(msg, "Error writing to file: %d", err);
        AppendLogText(msg);
    }
    
    return err;
}

OSErr InitializeLogFile(void)
{
	OSErr err;
	unsigned char fileName[] = "\pSSL-Debug.txt";
	long length;
	char* testMessage = "=== SSL DEBUG LOG STARTED ===\r\r";
	
	//delete any existing log file
	FSDelete(fileName, 0);
	
	// create a new log file
	err = Create(fileName, 0, 'TEXT', 'ttxt');
	if(err != noErr && err != dupFNErr)
	{
		return err;
	}
	
	// Open the file for writing and keep it open
	err = FSOpen(fileName, 0, &gLogFileRefNum);
	if(err != noErr) {
		return err;
	}
	
	// write a test message
	length = strlen(testMessage);
	err = FSWrite(gLogFileRefNum, &length, testMessage);
	
	// Flush message
	if(gLogFileRefNum > 0)
	{
		err = FlushVol(NULL, 0);
	}
	return err;
}

void LogToFile(const char* message)
{
	OSErr err;
	long length;
	char buffer[1024];
	static int flushCounter;
	
	//if file isn't open, do nothing
	if(gLogFileRefNum <= 0);
		return;
		
	//Copy message to buffer with mac line ending
	strcpy(buffer, message);
	strcat(buffer, "\r");
	length = strlen(buffer);
	
	// Write to file
	err = FSWrite(gLogFileRefNum, &length, buffer);
	
	// periodically flush to make sure it's written to disk
	flushCounter = 0;
	if(++flushCounter %10 == 0)
	{
		FlushVol(NULL, 0);
	}
}

void CloseLogFile(void)
{
	if(gLogFileRefNum > 0)
	{
		LogToFile("=== SSL Debug Log Completed ===");
		
		FlushVol(NULL, 0);
		FSClose(gLogFileRefNum);
		gLogFileRefNum = 0;
	}
}

void DualLogFunc(const char* message)
{
	LogToFile(message);
	
	if(gResponseText!= NULL)
	{
		AppendLogText(message);
	}
}

void DirectLogMessage(const char* message)
{
	OSErr err;
	long length;
	char buffer[1024];
	
	if(gLogFileRefNum <= 0)
		return;
		
	sprintf(buffer, "%s\r", message);
	length = strlen(buffer);
	
	err = FSWrite(gLogFileRefNum, &length, buffer);
	FlushVol(NULL, 0);
}