#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "return_codes.h"
#include "defines.h"
#include <stdint.h>
#include <stddef.h>

ret_code    udp_server_init(char *ip_str);
ret_code    udp_server_shutdown();
int         udp_server_recv(uint8_t *buff, size_t buff_size, int timeout);

#endif