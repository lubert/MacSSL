#ifndef API_H
#define API_H

#define DEFAULT_URL "640by480.com/api/v1/posts/"  /* Default URL */
#define API_PORT 443  /* HTTPS port */
#define API_PORT_HTTP 80  /* HTTP port for non-secure connections */
#define RESPONSE_BUFFER_SIZE 8192  /* Size of buffer for reading response chunks */

/* Protocol types */
typedef enum {
    kProtocolHTTP = 0,
    kProtocolHTTPS = 1
} ProtocolType;

#endif