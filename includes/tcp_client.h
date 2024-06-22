#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "return_codes.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

ret_code    tcp_client_init(char *ip_str);
ret_code    tcp_client_reconnect();
ret_code    tcp_client_shutdown();
ret_code    tcp_client_check_connection();
int         tcp_client_send(uint8_t *buff, size_t len);
void        tcp_client_flush_recv();
#endif