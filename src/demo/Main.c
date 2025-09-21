/*
 * PostMac Demo Application
 *
 * A simple client for making HTTPS requests
 */

/* Include compatibility layer first */
#include "../common/MacPlatform.h"

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
#include "../ssl/SSLWrapper.h"
#include "Logging.h"
#include "Globals.h"  /* Include after SSLWrapper.h to get SSLState type */
#include "../libyuarel/yuarel.h"
#include "../http/HTTPClient.h"

/* Demo application constants */
#define DEFAULT_URL "https://640by480.com/api/v1/posts/"

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
ControlHandle gMultiRequestButton = NULL;
TEHandle gURLText = NULL;
InetSvcRef gInetService = kOTInvalidProviderRef;
char gResponseBuffer[RESPONSE_BUFFER_SIZE];
TEHandle gResponseText = NULL;
SSLState gSSLState;
ControlHandle gVertScrollBar = NULL;
short gLogFileRefNum = 0;


/* Function prototypes */
void InitializeToolbox(void);
void SetupMenus(void);
void HandleScrollBarClick(ControlHandle control, short controlPart, Point mousePoint);
void HandleMenuChoice(long menuChoice);
void HandleEvent(EventRecord *event);
void HandleMouseDown(EventRecord *event);
void SetupWindow(void);
void DoUpdate(WindowPtr window);
OSStatus InitializeNetwork(void);
OSStatus CheckSSLLibrary(LoggingCallback logFunc);
void CleanupNetwork(void);
void DisplayResponse(char* response, long responseLength);
void MultiRequestDemo(void);

void ConvertLineEndings(char* text, size_t length);
int ParseURL(const char* url, char* hostname, char* path, size_t hostnameSize, size_t pathSize);
ProtocolType GetProtocolFromURL(const char* url);
void AppendLogText(const char* message);
void ClearLogText(void);
OSErr InitializeLogFile(void);
void DirectLogMessage(const char* message);
void CloseLogFile(void);
void LogMessage(const char* message);
void CopyTextToClipboard(TEHandle textH);

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
        LogMessage("PostMac started");
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
    Rect textRect;
    Rect visibleTextRect;
    Rect scrollBarRect;

    /* Create main window with larger dimensions to fit all controls */
    SetRect(&windowRect, 50, 50, 500, 400);
    gMainWindow = NewWindow(NULL, &windowRect, "\pPostMac", true, documentProc,
                            (WindowPtr)-1, true, 0);

    if (gMainWindow != NULL) {
        /* Set as active window */
        SetPort(gMainWindow);

        /* Create URL input field - position it on first row */
        SetRect(&textRect, 10, 10, 420, 30);
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

        /* Create buttons on second row */
        /* Connect button on left */
        SetRect(&buttonRect, 10, 40, 140, 60);
        gConnectButton = NewControl(gMainWindow, &buttonRect, "\pGET",
                              true, 0, 0, 0, pushButProc, kControlButtonPart);

        /* Test handshake button on right */
        SetRect(&buttonRect, 150, 40, 280, 60);
        gHandshakeButton = NewControl(gMainWindow, &buttonRect, "\pHandshake",
                              true, 0, 0, 0, pushButProc, kControlButtonPart);
        /* Multi-request demo button */
        SetRect(&buttonRect, 290, 40, 420, 60);
        gMultiRequestButton = NewControl(gMainWindow, &buttonRect, "\pMulti Request",
                              true, 0, 0, 0, pushButProc, kControlButtonPart);

        /* Create text edit field for response - position it below the controls */
        SetRect(&textRect, 10, 70, 420, 340);

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
            AppendLogText("Press 'GET'");

            /* Make it look better - set font and add a border */
            TextFont(kFontIDGeneva);
            TextSize(10);

            /* Draw border around text area */
            PenSize(1, 1);
            FrameRect(&textRect);
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
                        /* Multi-request demo button */
                        else if (control == gMultiRequestButton) {
                            MultiRequestDemo();
                        }
                        /* Vertical scrollbar */
                        else if (control == gVertScrollBar) {
                            HandleScrollBarClick(control, controlPart, mousePoint);
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

    /* Initialize SSL (always initialize it for potential HTTPS use) */
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

        /* We'll continue without SSL - HTTPS connections will fail */
        if (gResponseText != NULL) {
            AppendLogText("SSL failed to initialize. HTTPS connections will not work.");
        }
    }
    else if (gResponseText != NULL) {
        AppendLogText("SSL initialized successfully.");
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
    /* Always close SSL connection if active (safe to call) */
    SSL_Close(&gSSLState);


    /* Close internet service */
    if (gInetService != kOTInvalidProviderRef) {
        OTCloseProvider(gInetService);
        gInetService = kOTInvalidProviderRef;
    }

    CloseOpenTransport();
    gNetworkInitialized = false;
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

/* Determine protocol type from URL */
ProtocolType GetProtocolFromURL(const char* url) {
    if (url == NULL) {
        return kProtocolHTTPS; /* Default to HTTPS */
    }

    if (strncmp(url, "http://", 7) == 0) {
        return kProtocolHTTP;
    } else if (strncmp(url, "https://", 8) == 0) {
        return kProtocolHTTPS;
    } else {
        /* No protocol specified, default to HTTPS */
        return kProtocolHTTPS;
    }
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

/**
 * @brief Demonstrate the new granular HTTP client interface.
 *
 * This function shows how to connect once and make multiple requests,
 * demonstrating the advantage of the new interface over the monolithic approach.
 */
void MultiRequestDemo(void)
{
    OSStatus err = noErr;
    HTTPClientState clientState;
    HTTPResponse response;
    char hostname[256];
    char path[512];
    char url[512];
    int urlLen;
    ProtocolType protocolType;
    char statusMsg[256];

    /* Show wait cursor */
    SetCursor(*GetCursor(watchCursor));

    AppendLogText("=== Multi-Request Demo ===");

    /* Get URL from text field */
    if (gURLText == NULL) {
        AppendLogText("Error: URL field not initialized");
        SetCursor(&qd.arrow);
        return;
    }

    urlLen = (*gURLText)->teLength;
    if (urlLen >= sizeof(url)) {
        AppendLogText("Error: URL too long");
        SetCursor(&qd.arrow);
        return;
    }

    /* Copy URL from TextEdit handle */
    memcpy(url, *((*gURLText)->hText), urlLen);
    url[urlLen] = '\0';

    /* Determine protocol from URL */
    protocolType = GetProtocolFromURL(url);

    /* Parse URL into hostname and path */
    if (ParseURL(url, hostname, path, sizeof(hostname), sizeof(path)) != 0) {
        AppendLogText("Error: Could not parse URL");
        SetCursor(&qd.arrow);
        return;
    }

    if (strlen(hostname) == 0) {
        AppendLogText("Error: Please enter a hostname");
        SetCursor(&qd.arrow);
        return;
    }

    /* Only support HTTPS for now */
    if (protocolType != kProtocolHTTPS) {
        AppendLogText("Error: Multi-request demo only supports HTTPS URLs");
        SetCursor(&qd.arrow);
        return;
    }

    /* Step 1: Initialize HTTP client */
    AppendLogText("Step 1: Initializing HTTP client...");
    err = HttpInit(&clientState, AppendLogText);
    if (err != noErr) {
        sprintf(statusMsg, "Failed to initialize HTTP client. Error: %d", (int)err);
        AppendLogText(statusMsg);
        SetCursor(&qd.arrow);
        return;
    }

    /* Step 2: Connect to server (once) */
    sprintf(statusMsg, "Step 2: Connecting to %s...", hostname);
    AppendLogText(statusMsg);
    err = HttpConnect(&clientState, hostname, 443);
    if (err != noErr) {
        sprintf(statusMsg, "Failed to connect to %s. Error: %d", hostname, (int)err);
        AppendLogText(statusMsg);
        HttpClose(&clientState);
        SetCursor(&qd.arrow);
        return;
    }

    /* Step 3: Make multiple requests using the same connection */
    AppendLogText("Step 3: Making multiple HTTP requests...");

    /* Request 1: GET original path */
    sprintf(statusMsg, "Request 1: GET %s", path);
    AppendLogText(statusMsg);
    err = HttpGet(&clientState, path, &response);
    if (err == noErr) {
        sprintf(statusMsg, "Request 1 successful! Status: %d, Body length: %ld bytes",
                response.statusCode, (long)response.bodyLen);
        AppendLogText(statusMsg);
    } else {
        sprintf(statusMsg, "Request 1 failed. Error: %d", (int)err);
        AppendLogText(statusMsg);
    }

    /* Request 2: GET root path (to show different request) */
    AppendLogText("Request 2: GET /");
    err = HttpGet(&clientState, "/", &response);
    if (err == noErr) {
        sprintf(statusMsg, "Request 2 successful! Status: %d, Body length: %ld bytes",
                response.statusCode, (long)response.bodyLen);
        AppendLogText(statusMsg);
    } else {
        sprintf(statusMsg, "Request 2 failed. Error: %d", (int)err);
        AppendLogText(statusMsg);
    }

    /* Request 3: Add a custom header and make another request */
    AppendLogText("Request 3: Adding custom header and making request...");
    err = HttpSetHeader(&clientState, "User-Agent", "PostMac/1.0");
    if (err == noErr) {
        err = HttpGet(&clientState, path, &response);
        if (err == noErr) {
            sprintf(statusMsg, "Request 3 successful! Status: %d, Body length: %ld bytes",
                    response.statusCode, (long)response.bodyLen);
            AppendLogText(statusMsg);

            /* Display the final response */
            if (response.headersLen + response.bodyLen > 0) {
                char* responseStr = (char*)response.pBuffer;
                DisplayResponse(responseStr, response.headersLen + response.bodyLen);
            }
        } else {
            sprintf(statusMsg, "Request 3 failed. Error: %d", (int)err);
            AppendLogText(statusMsg);
        }
    } else {
        sprintf(statusMsg, "Failed to set custom header. Error: %d", (int)err);
        AppendLogText(statusMsg);
    }

    /* Step 4: Clean up */
    AppendLogText("Step 4: Closing connection...");
    HttpClose(&clientState);

    /* Restore cursor */
    SetCursor(&qd.arrow);

    AppendLogText("=== Multi-Request Demo Complete ===");
}