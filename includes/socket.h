#ifndef SOCKET_H
#define SOCKET_H

#include <stdbool.h>

bool is_closed(int sock);
bool is_connected(int sock);
void flush_recv_buff(int sock);

#endif