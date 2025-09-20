#ifndef API_H
#define API_H

#define API_HOST "640by480.com"
#define API_PORT 443  /* HTTPS port */
#define API_PORT_HTTP 80  /* HTTP port for non-secure connections */
#define API_PATH "/api/v1/posts/"
#define MAX_RESPONSE_SIZE 8192

/* Protocol types */
typedef enum {
    kProtocolHTTP = 0,
    kProtocolHTTPS = 1
} ProtocolType;

#endif