#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "return_codes.h"
#include "defines.h"
#include <stdint.h>
#include <stddef.h>

ret_code udp_server_init(char *ip_str, uint8_t *buff, size_t buff_size);
ret_code udp_server_install_handler(callback_handle_t handler);
ret_code udp_server_iterate(int timeout);
ret_code udp_server_shutdown();

#endif