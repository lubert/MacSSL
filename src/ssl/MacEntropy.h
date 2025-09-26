#ifndef MAC_ENTROPY_H
#define MAC_ENTROPY_H

#include <Types.h>

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen);
int mac_entropy_func(void *data, unsigned char *output, size_t len);

#endif