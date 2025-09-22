#ifndef PROTOCOL_TYPES_H
#define PROTOCOL_TYPES_H

/* Standard networking ports */
#define HTTPS_PORT 443
#define HTTP_PORT 80

/* Buffer size for HTTP responses - increased to handle larger payloads */
#define RESPONSE_BUFFER_SIZE 16384

/* Protocol types for HTTP communication */
typedef enum {
    kProtocolHTTP = 0,
    kProtocolHTTPS = 1
} ProtocolType;

#endif