/*
 * 640by480 Classic Mac Client - Retro68 Version
 *
 * A simple client for the 640by480 photo sharing service
 * Ported from CodeWarrior Pro 4 to Retro68 GCC toolchain
 * for Classic Mac OS 7.1-9.2
 */

/* Include compatibility layer first */
#include "retro68_compat.h"

/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* Mac OS System Headers */
#include <Types.h>
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
#include <Scrap.h>
#include <Files.h>

/* Networking Headers */
#include <OpenTransport.h>
#include <OpenTptInternet.h>

/* MbedTLS headers */
#include "mbedtls/base64.h"
#include "mbedtls/ssl.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha1.h"

/* Application-specific headers */
#include "ssl_wrapper.h"
#include "logging.h"
#include "globals.h"  /* Include after SSLWrapper.h to get SSLState type */
#include "yuarel.h"   /* URL parsing library */

/* Missing Mac Toolbox constants for Retro68 */
#ifndef radioButProc
#define radioButProc 2
#endif

#ifndef pushButProc
#define pushButProc 0
#endif

#ifndef scrollBarProc
#define scrollBarProc 16
#endif

#ifndef kControlButtonPart
#define kControlButtonPart 10
#endif

#ifndef kFontIDGeneva
#define kFontIDGeneva 3
#endif

/* Missing Mac Toolbox utility functions for Retro68 */
#ifndef HiWord
static short HiWord(long longValue) {
    return (short)((longValue >> 16) & 0xFFFF);
}
#endif

#ifndef LoWord
static short LoWord(long longValue) {
    return (short)(longValue & 0xFFFF);
}
#endif

/* Global variables */
Boolean gDone = false;
Boolean gNetworkInitialized = false;

MenuHandle gFileMenu;
MenuHandle gEditMenu;
WindowPtr gMainWindow = NULL;
ControlHandle gConnectButton = NULL;
ControlHandle gHandshakeButton = NULL;
TEHandle gURLText = NULL;
EndpointRef gTCPEndpoint = kOTInvalidEndpointRef;
InetSvcRef gInetService = kOTInvalidProviderRef;
char gResponseBuffer[RESPONSE_BUFFER_SIZE];
char gRequestBuffer[1024];   /* Request buffer for HTTP requests */
char gURLBuffer[256];        /* Buffer for URL input */
TEHandle gResponseText = NULL;
ControlHandle gProtocolRadio[2];  /* Radio buttons for HTTP/HTTPS selection */
SSLState gSSLState;
ProtocolType gProtocolType = kProtocolHTTPS;
ControlHandle gVertScrollBar = NULL;
short gLogFileRefNum = 0;

/* Edit menu constants */
#define kEditMenuID 129
#define kEditSelectAll 1
#define kEditCopy 3

/* Function prototypes */
void InitializeToolbox(void);
void SetupMenus(void);
void HandleRadioClick(ControlHandle control);
void HandleScrollBarClick(ControlHandle control, short controlPart, Point mousePoint);
void HandleMenuChoice(long menuChoice);
void HandleEvent(EventRecord *event);
void HandleMouseDown(EventRecord *event);
void SetupWindow(void);
void DoUpdate(WindowPtr window);
OSStatus InitializeNetwork(void);
OSStatus CheckSSLLibrary(LoggingCallback logFunc);
void CleanupNetwork(void);
OSStatus ConnectToServer(void);
OSStatus TestSSLHandshake(void);
void DisplayResponse(char* response, long responseLength);
void AppendResponseChunk(char* chunk, long chunkLength);
void ConvertLineEndings(char* text, size_t length);
int ParseURL(const char* url, char* hostname, char* path, size_t hostnameSize, size_t pathSize);
void dummy_function(void);
void AppendLogText(const char* message);
void ClearLogText(void);
void LogTextf(const char* format, ...);
void LogMessage(const char* message);
void ClearLog(void);
void LogMessagef(const char* format, ...);
OSErr InitializeLogFile(void);
void DirectLogMessage(const char* message);
void CloseLogFile(void);
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

    /* Initialize log file */
    err = InitializeLogFile();
    if (err != noErr) {
        LogMessage("Warning: Could not initialize log file");
    } else {
        LogMessage("640by480 Client started - Retro68 version");
    }

    /* Initialize networking */
    err = InitializeNetwork();
    if (err != noErr) {
        /* Show error dialog */
        SysBeep(2);
    }

    /* Enter main event loop */
    while (!gDone) {
        if (WaitNextEvent(everyEvent, &event, 6, NULL)) {  /* 6 ticks = 1/10 second */
            HandleEvent(&event);
        } else {
            /* Handle idle time - make text cursor blink and update mouse cursor */
            if (gURLText != NULL) {
                TEIdle(gURLText);
            }

            /* Update mouse cursor based on position */
            Point mouseLoc;
            GetMouse(&mouseLoc);
            if (gURLText != NULL && PtInRect(mouseLoc, &(*gURLText)->viewRect)) {
                /* Mouse is over URL text field - show I-beam cursor */
                CursHandle iBeamHandle = GetCursor(iBeamCursor);
                if (iBeamHandle != NULL) {
                    SetCursor(*iBeamHandle);
                }
            } else {
                /* Mouse is elsewhere - show arrow cursor */
                SetCursor(&qd.arrow);
            }
        }
    }

    /* Clean up */
    CleanupNetwork();

    /* Clean up text handles if they exist */
    if (gResponseText != NULL) {
        TEDispose(gResponseText);
    }
    if (gURLText != NULL) {
        TEDispose(gURLText);
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

    /* Create main window with larger dimensions to fit all controls */
    SetRect(&windowRect, 50, 50, 500, 400);
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

        /* Create URL input field - position it on second row */
        SetRect(&textRect, 10, 35, 300, 55);
        visibleTextRect = textRect;
        InsetRect(&visibleTextRect, 3, 2);  /* Add padding inside the border */
        gURLText = TENew(&visibleTextRect, &textRect);
        if (gURLText != NULL) {
            /* Set default URL */
            TESetText(DEFAULT_URL, strlen(DEFAULT_URL), gURLText);
            /* Draw border around URL field */
            PenSize(1, 1);
            FrameRect(&textRect);
        }

        /* Create handshake test button - position it next to URL field */
        SetRect(&buttonRect, 310, 35, 420, 55);
        gHandshakeButton = NewControl(gMainWindow, &buttonRect, "\pTest Handshake",
                              true, 0, 0, 0, pushButProc, kControlButtonPart);

        /* Create connect button - position it to the right of the radio buttons */
        SetRect(&buttonRect, 200, 10, 340, 30);
        gConnectButton = NewControl(gMainWindow, &buttonRect, "\pConnect To Server",
                              true, 0, 0, 0, pushButProc, kControlButtonPart);

        /* Create text edit field for response - position it below the controls */
        SetRect(&textRect, 10, 65, 420, 335);

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

/* Handle radio button clicks for protocol switching */
void HandleRadioClick(ControlHandle control)
{
    ProtocolType newProtocol;

    /* Determine which protocol was selected */
    if (control == gProtocolRadio[kProtocolHTTP]) {
        newProtocol = kProtocolHTTP;
    } else if (control == gProtocolRadio[kProtocolHTTPS]) {
        newProtocol = kProtocolHTTPS;
    } else {
        return; /* Unknown control */
    }

    /* Only update if protocol actually changed */
    if (newProtocol != gProtocolType) {
        gProtocolType = newProtocol;

        /* Update radio button states */
        SetControlValue(gProtocolRadio[kProtocolHTTP], (gProtocolType == kProtocolHTTP) ? 1 : 0);
        SetControlValue(gProtocolRadio[kProtocolHTTPS], (gProtocolType == kProtocolHTTPS) ? 1 : 0);

        /* Log the protocol change */
        if (gProtocolType == kProtocolHTTP) {
            AppendLogText("Switched to HTTP protocol");
        } else {
            AppendLogText("Switched to HTTPS protocol");
        }
    }
}

void HandleScrollBarClick(ControlHandle control, short controlPart, Point mousePoint)
{
    int scrollAmount = 0;
    int currentValue, maxValue;

    if (gResponseText == NULL || control != gVertScrollBar) return;

    currentValue = GetControlValue(control);
    maxValue = GetControlMaximum(control);

    switch (controlPart) {
        case 20:  /* inUpButton */
            scrollAmount = -1;  /* Scroll up one line */
            break;
        case 21:  /* inDownButton */
            scrollAmount = 1;   /* Scroll down one line */
            break;
        case 22:  /* inPageUp */
            scrollAmount = -10; /* Scroll up one page */
            break;
        case 23:  /* inPageDown */
            scrollAmount = 10;  /* Scroll down one page */
            break;
        case 129: /* inThumb */
            /* User dragged the thumb - get new position */
            scrollAmount = TrackControl(control, mousePoint, NULL) - currentValue;
            break;
    }

    if (scrollAmount != 0) {
        int newValue = currentValue + scrollAmount;
        if (newValue < 0) newValue = 0;
        if (newValue > maxValue) newValue = maxValue;

        SetControlValue(control, newValue);

        /* Scroll the text */
        TEScroll(0, (currentValue - newValue) * 12, gResponseText); /* 12 pixels per line */
        TEUpdate(&(*gResponseText)->viewRect, gResponseText);
    }
}

void HandleMenuChoice(long menuChoice)
{
    short menu = HiWord(menuChoice);
    short item = LoWord(menuChoice);

    switch (menu) {
        case 128: /* File menu */
            switch (item) {
                case 1: /* Quit */
                    CloseLogFile();
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

void HandleEvent(EventRecord *event)
{
    WindowPtr window;
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
                HandleMenuChoice(MenuKey(key));
            } else {
                /* Send keystrokes to URL text field if it's active */
                if (gURLText != NULL) {
                    TEKey(key, gURLText);

                    /* Redraw the border that may have been erased by TEKey */
                    Rect borderRect = (*gURLText)->viewRect;
                    PenNormal();
                    FrameRect(&borderRect);
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
                    CloseLogFile();
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
                        /* Handshake test button */
                        else if (control == gHandshakeButton) {
                            TestSSLHandshake();
                        }
                        /* Vertical scrollbar */
                        else if (control == gVertScrollBar) {
                            HandleScrollBarClick(control, controlPart, mousePoint);
                        }
                        /* Radio buttons */
                        else if (control == gProtocolRadio[kProtocolHTTP] ||
                                control == gProtocolRadio[kProtocolHTTPS]) {
                            HandleRadioClick(control);
                        }
                    }
                }

                /* Handle clicks in URL text field */
                if (gURLText != NULL &&
                    PtInRect(mousePoint, &(*gURLText)->viewRect)) {
                    TEClick(mousePoint, (event->modifiers & shiftKey) != 0, gURLText);

                    /* Activate the text field and show cursor */
                    TEActivate(gURLText);

                    /* Redraw the border that may have been erased by TEClick */
                    Rect borderRect = (*gURLText)->viewRect;
                    PenNormal();
                    FrameRect(&borderRect);
                }
                /* Handle clicks in response text field */
                else if (gResponseText != NULL &&
                    PtInRect(mousePoint, &(*gResponseText)->viewRect)) {
                    /* Deactivate URL field if it was active */
                    if (gURLText != NULL) {
                        TEDeactivate(gURLText);
                    }
                    TEClick(mousePoint, (event->modifiers & shiftKey) != 0, gResponseText);
                }
                /* Handle clicks elsewhere - deactivate URL field */
                else {
                    if (gURLText != NULL) {
                        TEDeactivate(gURLText);
                    }
                }
            }
            break;
    }
}

void DoUpdate(WindowPtr window)
{
    Rect textBorderRect;

    if (window == gMainWindow) {
        /* Redraw our controls */
        UpdateControls(window, window->visRgn);

        /* Redraw the URL text field */
        if (gURLText != NULL) {
            TEUpdate(&(*gURLText)->viewRect, gURLText);

            /* Redraw border around URL field */
            textBorderRect = (*gURLText)->viewRect;
            PenNormal();
            FrameRect(&textBorderRect);
        }

        /* Redraw the response text */
        if (gResponseText != NULL) {
            TEUpdate(&(*gResponseText)->viewRect, gResponseText);

            /* Redraw border around text area */
            textBorderRect = (*gResponseText)->viewRect;
            InsetRect(&textBorderRect, -5,-5);
            PenNormal();
            FrameRect(&textBorderRect);
        }

        if(gVertScrollBar != NULL) {
            DrawControls(window);
        }
    }
}

/* Network initialization */
OSStatus InitializeNetwork(void) {
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

OSStatus CheckSSLLibrary(LoggingCallback logFunc) {
    /* Simple check - try to initialize an SSL state */
    SSLState testState;
    OSStatus result;

    memset(&testState, 0, sizeof(testState));

    if (logFunc) logFunc("Testing SSL library functions...");

    /* Try basic SSL initialization */
    result = SSL_Initialize(&testState, logFunc);

    if (result == noErr) {
        if (logFunc) logFunc("SSL library test passed");
        SSL_Close(&testState);
    } else {
        if (logFunc) logFunc("SSL library test failed");
    }

    return result;
}

void CleanupNetwork(void) {
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

OSStatus ConnectToServer(void) {
    OSStatus err = noErr;
    InetHostInfo hostInfo;
    InetAddress inAddr;
    unsigned long responseLength = 0;
    size_t bytesSent = 0;
    size_t bytesReceived = 0;
    char connectMsg[50];
    OTResult sendResult;
    int readAttempts;
    const int maxReadAttempts = 10;
    char hostname[256];
    char path[512];
    char url[512];
    int urlLen;

    /* Show wait cursor */
    SetCursor(*GetCursor(watchCursor));

    /* Get URL from text field */
    if (gURLText == NULL) {
        AppendLogText("Error: URL field not initialized");
        SetCursor(&qd.arrow);
        return -1;
    }

    urlLen = (*gURLText)->teLength;
    if (urlLen >= sizeof(url)) {
        AppendLogText("Error: URL too long");
        SetCursor(&qd.arrow);
        return -1;
    }

    /* Copy URL from TextEdit handle */
    memcpy(url, *((*gURLText)->hText), urlLen);
    url[urlLen] = '\0';

    /* Parse URL into hostname and path */
    if (ParseURL(url, hostname, path, sizeof(hostname), sizeof(path)) != 0) {
        AppendLogText("Error: Could not parse URL");
        SetCursor(&qd.arrow);
        return -1;
    }

    if (strlen(hostname) == 0) {
        AppendLogText("Error: Please enter a hostname");
        SetCursor(&qd.arrow);
        return -1;
    }

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
    err = OTInetStringToAddress(gInetService, hostname, &hostInfo);
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
        err = SSL_Connect(&gSSLState, &inAddr, hostname, gResponseText, AppendLogText);
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
        /* Use standard TCP for HTTP connection - simplified version */
        AppendLogText("HTTP connections not fully implemented yet");
        SetCursor(&qd.arrow);
        return paramErr;
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
    sprintf(gRequestBuffer, "GET %s HTTP/1.0\r\n", path);
    /* Add Host header - required for virtual hosting */
    sprintf(gRequestBuffer + strlen(gRequestBuffer), "Host: %s\r\n", hostname);
    /* Add User-Agent */
    sprintf(gRequestBuffer + strlen(gRequestBuffer), "User-Agent: 640by480-ClassicMacClient/1.0\r\n");
    /* Content type we're willing to accept */
    sprintf(gRequestBuffer + strlen(gRequestBuffer), "Accept: */*\r\n");
    /* Disable keep-alive to ensure connection closes after response */
    sprintf(gRequestBuffer + strlen(gRequestBuffer), "Connection: close\r\n");
    /* End of headers */
    sprintf(gRequestBuffer + strlen(gRequestBuffer), "\r\n");

    /* Send the request */
    AppendLogText("Sending HTTP request...");

    if (gProtocolType == kProtocolHTTPS) {
        err = SSL_Send(&gSSLState, gRequestBuffer, strlen(gRequestBuffer), &bytesSent, AppendLogText);
        if (err != noErr) {
            char errMsg[100];
            sprintf(errMsg, "Error: Failed to send request (code %d, sent %lu of %lu bytes)",
                    (int)err, (unsigned long)bytesSent, (unsigned long)strlen(gRequestBuffer));
            AppendLogText(errMsg);

            SSL_Close(&gSSLState);
            SetCursor(&qd.arrow);
            return err;
        }
    }

    /* Update status */
    if (gResponseText != NULL) {
        AppendLogText("Request sent. Waiting for response...");
    }

    /* Receive the response using buffered reading */
    responseLength = 0;
    long totalBytesReceived = 0;
    memset(gResponseBuffer, 0, RESPONSE_BUFFER_SIZE);
    AppendLogText("Request sent. Waiting for response...");

    readAttempts = 0;
    while (true) {
        if (gProtocolType == kProtocolHTTPS) {
            /* Use SSL for HTTPS connection */
            err = SSL_Receive(&gSSLState,
                             gResponseBuffer,
                             RESPONSE_BUFFER_SIZE - 1,
                             &bytesReceived,
                             AppendLogText);

            if (err != noErr) {
                if (bytesReceived == 0) {
                    /* Connection closed */
                    break;
                }
                /* Other error */
                char errMsg[100];
                sprintf(errMsg, "Error receiving data: %d", (int)err);
                AppendLogText(errMsg);
                break;
            }

            if (bytesReceived == 0) {
                /* Connection closed cleanly */
                break;
            }

            /* Null-terminate the current chunk */
            gResponseBuffer[bytesReceived] = '\0';

            /* Display this chunk immediately */
            if (totalBytesReceived == 0) {
                /* First chunk - display headers and start of response */
                DisplayResponse(gResponseBuffer, bytesReceived);
            } else {
                /* Subsequent chunks - append to display */
                AppendResponseChunk(gResponseBuffer, bytesReceived);
            }

            totalBytesReceived += bytesReceived;
            readAttempts = 0; /* Reset counter on successful read */
        } else {
            /* TCP not implemented yet */
            break;
        }

        readAttempts++;
        if (readAttempts > maxReadAttempts) {
            AppendLogText("Too many read attempts, stopping");
            break;
        }
    }

    /* Update final status */
    if (gResponseText != NULL) {
        char statusMsg[100];
        sprintf(statusMsg, "Received %lu bytes", totalBytesReceived);
        AppendLogText(statusMsg);
    }

    /* Check if we received any data */
    if (totalBytesReceived == 0) {
        AppendLogText("No data received from server");
    }

    /* Clean up connection */
    if (gProtocolType == kProtocolHTTPS) {
        SSL_Close(&gSSLState);
    }

    /* Restore cursor */
    SetCursor(&qd.arrow);

    return noErr;
}

OSStatus TestSSLHandshake(void) {
    OSStatus err = noErr;
    char hostname[256];
    char path[512];
    char url[512];
    int urlLen;
    InetAddress inAddr;
    char statusMsg[300];

    /* Clear response area */
    if (gResponseText != NULL) {
        ClearLogText();
        AppendLogText("Testing SSL handshake...");
    }

    /* Get URL from text field */
    if (gURLText == NULL) {
        AppendLogText("Error: URL field not initialized");
        return -1;
    }

    urlLen = (*gURLText)->teLength;
    if (urlLen >= sizeof(url)) {
        AppendLogText("Error: URL too long");
        return -1;
    }

    /* Copy URL from TextEdit handle */
    memcpy(url, *((*gURLText)->hText), urlLen);
    url[urlLen] = '\0';

    /* Parse URL into hostname and path */
    if (ParseURL(url, hostname, path, sizeof(hostname), sizeof(path)) != 0) {
        AppendLogText("Error: Could not parse URL");
        return -1;
    }

    if (strlen(hostname) == 0) {
        AppendLogText("Error: Please enter a hostname");
        return -1;
    }

    /* Show what we're testing */
    sprintf(statusMsg, "Testing SSL handshake with: %s", hostname);
    AppendLogText(statusMsg);

    /* Initialize SSL */
    AppendLogText("Initializing SSL...");
    err = SSL_Initialize(&gSSLState, AppendLogText);
    if (err != noErr) {
        sprintf(statusMsg, "SSL initialization failed. Error: %d", (int)err);
        AppendLogText(statusMsg);
        return err;
    }

    /* Look up the host address */
    InetHostInfo hostInfo;
    err = OTInetStringToAddress(gInetService, hostname, &hostInfo);
    if (err != noErr) {
        sprintf(statusMsg, "Could not resolve hostname: %s", hostname);
        AppendLogText(statusMsg);
        SSL_Close(&gSSLState);
        return err;
    }

    /* Set up the address for the remote host with HTTPS port */
    OTInitInetAddress(&inAddr, 443, hostInfo.addrs[0]);

    /* Connect and test handshake only */
    AppendLogText("Connecting...");
    err = SSL_Connect(&gSSLState, &inAddr, hostname, NULL, AppendLogText);
    if (err != noErr) {
        sprintf(statusMsg, "SSL handshake failed. Error: %d", (int)err);
        AppendLogText(statusMsg);
        SSL_Close(&gSSLState);
        return err;
    }

    AppendLogText("SSL handshake successful!");
    AppendLogText("Handshake test completed - no data transfer performed.");

    /* Clean up SSL connection */
    SSL_Close(&gSSLState);

    return noErr;
}

void DisplayResponse(char* response, long responseLength) {
    char displayBuffer[4096];
    char* bodyStart;
    char* headerEnd;
    int bytesToDisplay;
    char statusMsg[100];

    if (response == NULL || responseLength <= 0) {
        AppendLogText("No response to display");
        return;
    }

    /* Log the status */
    sprintf(statusMsg, "Processing %ld bytes of response data", responseLength);
    AppendLogText(statusMsg);

    /* Find the end of HTTP headers (look for \r\n\r\n) */
    headerEnd = strstr(response, "\r\n\r\n");
    if (headerEnd != NULL) {
        /* Display headers first */
        int headerLength = headerEnd - response;
        if (headerLength > 0 && headerLength < sizeof(displayBuffer) - 1) {
            memcpy(displayBuffer, response, headerLength);
            displayBuffer[headerLength] = '\0';
            AppendLogText("--- HTTP Headers ---");
            AppendLogText(displayBuffer);
        }

        /* Skip to body content */
        bodyStart = headerEnd + 4;  /* Skip past \r\n\r\n */

        /* Calculate body length */
        long bodyLength = responseLength - (bodyStart - response);

        if (bodyLength > 0) {
            AppendLogText("--- Response Body ---");
            sprintf(statusMsg, "JSON response received: %ld bytes (not displayed to prevent crashes)", bodyLength);
            AppendLogText(statusMsg);
        }
    } else {
        /* No clear header/body separation - just report the size */
        AppendLogText("--- Raw Response ---");
        sprintf(statusMsg, "Response received: %ld bytes (not displayed to prevent crashes)", responseLength);
        AppendLogText(statusMsg);
    }

    AppendLogText("--- End of Response ---");
}

void AppendResponseChunk(char* chunk, long chunkLength) {
    char statusMsg[100];
    char* displayBuffer;
    long maxDisplayLength = 2048;  /* Much larger display size */
    long displayLength;
    long pos;

    if (chunk == NULL || chunkLength <= 0) {
        return;
    }

    /* Log that we received another chunk */
    sprintf(statusMsg, "Processing additional %ld bytes...", chunkLength);
    AppendLogText(statusMsg);

    /* Allocate buffer for display */
    displayLength = (chunkLength < maxDisplayLength) ? chunkLength : maxDisplayLength;
    displayBuffer = (char*)malloc(displayLength + 1);
    if (displayBuffer == NULL) {
        AppendLogText("Error: Could not allocate memory for chunk display");
        return;
    }

    memcpy(displayBuffer, chunk, displayLength);
    displayBuffer[displayLength] = '\0';

    /* Convert line endings for Mac display */
    ConvertLineEndings(displayBuffer, displayLength);

    /* Break large chunks into smaller pieces for AppendLogText */
    AppendLogText("--- Chunk Content ---");
    for (pos = 0; pos < displayLength; pos += 500) {
        char pieceBuffer[501];
        long pieceLength = ((displayLength - pos) < 500) ? (displayLength - pos) : 500;

        memcpy(pieceBuffer, displayBuffer + pos, pieceLength);
        pieceBuffer[pieceLength] = '\0';

        AppendLogText(pieceBuffer);
    }

    if (chunkLength > displayLength) {
        sprintf(statusMsg, "... (%ld more bytes not shown)", chunkLength - displayLength);
        AppendLogText(statusMsg);
    }
    AppendLogText("--- End Chunk ---");

    free(displayBuffer);
}

/* Parse a URL into hostname and path components using libyuarel */
int ParseURL(const char* url, char* hostname, char* path, size_t hostnameSize, size_t pathSize) {
    struct yuarel parsed_url;
    char* url_copy;
    size_t url_len;

    if (url == NULL || hostname == NULL || path == NULL) {
        return -1;
    }

    /* libyuarel requires a scheme, so add https:// if missing */
    if (strstr(url, "://") == NULL) {
        /* No scheme found, prepend https:// */
        url_len = strlen(url) + 8; /* 8 = strlen("https://") */
        url_copy = (char*)malloc(url_len + 1);
        if (url_copy == NULL) {
            return -1; /* Memory allocation failed */
        }
        strcpy(url_copy, "https://");
        strcat(url_copy, url);
    } else {
        /* Scheme present, just make a copy */
        url_len = strlen(url);
        url_copy = (char*)malloc(url_len + 1);
        if (url_copy == NULL) {
            return -1; /* Memory allocation failed */
        }
        strcpy(url_copy, url);
    }

    /* Parse the URL using libyuarel */
    if (yuarel_parse(&parsed_url, url_copy) != 0) {
        free(url_copy);
        return -1; /* Parsing failed */
    }

    /* Extract hostname */
    if (parsed_url.host == NULL) {
        free(url_copy);
        return -1; /* No hostname found */
    }
    if (strlen(parsed_url.host) >= hostnameSize) {
        free(url_copy);
        return -1; /* Hostname too long */
    }
    strcpy(hostname, parsed_url.host);

    /* Extract path - if no path, use root */
    if (parsed_url.path == NULL || strlen(parsed_url.path) == 0) {
        strcpy(path, "/");
    } else {
        /* Add leading slash if not present */
        if (parsed_url.path[0] != '/') {
            if (strlen(parsed_url.path) + 2 >= pathSize) {
                free(url_copy);
                return -1; /* Path too long */
            }
            path[0] = '/';
            strcpy(path + 1, parsed_url.path);
        } else {
            if (strlen(parsed_url.path) >= pathSize) {
                free(url_copy);
                return -1; /* Path too long */
            }
            strcpy(path, parsed_url.path);
        }
    }

    free(url_copy);
    return 0; /* Success */
}

void dummy_function(void) {
    /* Placeholder function */
}

/* Convert Unix/Windows line endings to Mac line endings */
void ConvertLineEndings(char* text, size_t length) {
    size_t i, j;

    /* First pass: convert LF to CR */
    for (i = 0; i < length; i++) {
        if (text[i] == '\n') {
            text[i] = '\r';
        }
    }

    /* Second pass: remove duplicate CRs from CRLF conversion */
    j = 0;
    for (i = 0; i < length; i++) {
        if (i > 0 && text[i-1] == '\r' && text[i] == '\r') {
            /* Skip the duplicate CR */
            continue;
        }
        text[j++] = text[i];
    }

    /* Null terminate at the new length */
    if (j < length) {
        text[j] = '\0';
    }
}

void AppendLogText(const char* message)
{
    short textLen;
    char* convertedMessage;
    size_t messageLen;

    /* Also write to log file */
    DirectLogMessage(message);

    if (gResponseText == NULL)
        return;

    /* Create a copy of the message to convert line endings */
    messageLen = strlen(message);
    convertedMessage = NewPtr(messageLen + 1);
    if (convertedMessage == NULL)
        return;

    strcpy(convertedMessage, message);
    ConvertLineEndings(convertedMessage, messageLen);

    /* Get current text length */
    textLen = (*gResponseText)->teLength;

    /* Append newline if there's already text */
    if (textLen > 0) {
        TEInsert("\r", 1, gResponseText);  /* Use Mac line ending */
        textLen += 1;
    }

    /* Append the converted message */
    TEInsert(convertedMessage, strlen(convertedMessage), gResponseText);

    /* Auto-scroll to see the latest entry */
    TESetSelect(textLen + strlen(convertedMessage), textLen + strlen(convertedMessage), gResponseText);
    TESelView(gResponseText);

    /* Update the display */
    TEUpdate(&(*gResponseText)->viewRect, gResponseText);

    /* Update scrollbar range based on text content */
    if (gVertScrollBar != NULL) {
        int lineCount = (*gResponseText)->nLines;
        int visibleLines = ((*gResponseText)->viewRect.bottom - (*gResponseText)->viewRect.top) / (*gResponseText)->lineHeight;
        int maxScroll = lineCount - visibleLines;
        if (maxScroll < 0) maxScroll = 0;

        SetControlMaximum(gVertScrollBar, maxScroll);
        SetControlValue(gVertScrollBar, maxScroll); /* Auto-scroll to bottom */
    }

    /* Clean up */
    DisposePtr(convertedMessage);
}

void ClearLogText(void)
{
    if (gResponseText == NULL)
        return;

    /* Reset text */
    TESetText("", 0, gResponseText);
}

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

/* File logging functions */
OSErr InitializeLogFile(void) {
    OSErr err;
    Str255 fileName;
    long length;
    char testMessage[] = "Log file initialized\r";

    /* Create Pascal string for "out" */
    fileName[0] = 3;      /* Length */
    fileName[1] = 'o';
    fileName[2] = 'u';
    fileName[3] = 't';

    /* Try to create the file - this will fail if it already exists, which is OK */
    err = Create(fileName, 0, 'MACS', 'TEXT');
    /* Ignore error if file already exists */

    /* Open the file for writing */
    err = FSOpen(fileName, 0, &gLogFileRefNum);
    if (err != noErr) {
        return err;
    }

    /* Write a test message */
    length = strlen(testMessage);
    err = FSWrite(gLogFileRefNum, &length, testMessage);

    return err;
}

void DirectLogMessage(const char* message) {
    long length;
    OSErr err;
    char buffer[512];

    if (gLogFileRefNum <= 0) return;

    /* Copy message to buffer and add carriage return */
    strcpy(buffer, message);
    strcat(buffer, "\r");

    length = strlen(buffer);
    err = FSWrite(gLogFileRefNum, &length, buffer);
}

void CloseLogFile(void) {
    if (gLogFileRefNum > 0) {
        DirectLogMessage("Log file closed");
        FSClose(gLogFileRefNum);
        gLogFileRefNum = 0;
    }
}

void LogMessage(const char* message) {
    AppendLogText(message);
    DirectLogMessage(message);  /* Also write to file */
}

void ClearLog(void) {
    ClearLogText();
}

void LogMessagef(const char* format, ...) {
    char buffer[256];
    va_list args;

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    LogMessage(buffer);
}

void LogHTTPRequest(const char* requestBuffer, LoggingCallback logFunc) {
    if (logFunc) logFunc("HTTP request logging not yet implemented");
}

void LogHTTPResponse(const char* responseBuffer, long responseLength, LoggingCallback logFunc) {
    if (logFunc) logFunc("HTTP response logging not yet implemented");
}

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

OSStatus WriteResponseToFile(char* buffer, long bufferLength) {
    /* TODO: Implement file writing */
    return noErr;
}