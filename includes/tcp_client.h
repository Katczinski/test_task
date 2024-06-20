#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "return_codes.h"
#include <stddef.h>
#include <stdint.h>

ret_code tcp_client_init(char *ip_str, uint8_t *buff, size_t buff_size);
ret_code tcp_client_send_buff(size_t len);
ret_code tcp_client_reconnect();
ret_code tcp_client_shutdown();
ret_code tcp_client_iterate();
ret_code tcp_client_check_connection();

#endif