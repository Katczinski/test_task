#include "tcp_client.h"
#include "errors.h"
#include "log.h"
#include "utils.h"

#include <time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

typedef enum {
    SOCK_READY,
    SOCK_PENDING,
    SOCK_CLOSED,
    SOCK_ERROR,
} sock_status;

struct tcp_client_s
{
    struct sockaddr_in  server;
    int                 sock;
    bool                connected;
    sock_status         status;
} tcp_client;

ret_code tcp_client_init(char *ip_str)
{
    int                 sock = 0;
    bool                connected = true;
    sock_status         status = SOCK_READY;
    struct sockaddr_in  server;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        log_add("Failed to create socket: %s", get_errno_str());
        return RET_ERROR;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    if (!extract_ip_port(ip_str, &server.sin_addr.s_addr, &server.sin_port))
    {
        log_add("Failed to parse ip:port '%s': %s", ip_str, get_errno_str());
        return RET_ERROR;
    }

    // connect the client socket to server socket
    reset_errno();
    
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) != 0)
    {
        log_add("Failed to connect to '%s': %s\n", ip_str, get_errno_str());
        connected = false;
        status = SOCK_ERROR;
        // return RET_ERROR;
    }
    //  Doing after connect. idc if I block while initializing, but when loop is running - I must not
    // int flags = fcntl(sock, F_GETFL, 0);
    // if (flags == -1)
    // {
    //     log_add("Failed to read socket flags: %s", get_errno_str());
    //     return RET_ERROR;
    // }

    // flags |= O_NONBLOCK;
    // if (fcntl(sock, F_SETFL, flags) != 0)
    // {
    //     log_add("Failed to set socket flags: %s", get_errno_str());
    //     return RET_ERROR;
    // }

    tcp_client.sock = sock;
    tcp_client.server = server;
    tcp_client.connected = connected;
    tcp_client.status = status;

    log_add("TCP client inited on %s", ip_str);

    return RET_OK;
}

ret_code tcp_client_reconnect(bool silent)
{
    if (!silent) log_add("Reconnecting to TCP server...");
    close(tcp_client.sock);
    if ((tcp_client.sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        if (!silent) log_add("Failed to create socket: %s", get_errno_str());
        return RET_ERROR;
    }
    if (connect(tcp_client.sock, (struct sockaddr *)&tcp_client.server, sizeof(tcp_client.server)) != 0)
    {
        if (!silent) log_add("Failed to reconnect: %s\n", get_errno_str());
        return RET_PENDING;
    }
    tcp_client.connected = true;
    tcp_client.status = SOCK_READY;
    log_add("Connection established");
    return RET_OK;
}

ret_code tcp_client_shutdown()
{
    close(tcp_client.sock);
    tcp_client.connected = false;
    tcp_client.status = SOCK_CLOSED;
    log_add("TCP client shutdown");
    return RET_OK;
}

// TODO: move to "socket.c" or smth
bool is_closed(int sock)
{
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(sock, &rfd);
    struct timeval tv = {0};
    select(sock + 1, &rfd, 0, 0, &tv);
    if (!FD_ISSET(sock, &rfd))
        return false;
    int n = 0;
    ioctl(sock, FIONREAD, &n);
    return n == 0;
}

ret_code tcp_client_check_connection()
{
    return (is_closed(tcp_client.sock) ? RET_ERROR : RET_OK);
}

ret_code tcp_client_iterate()
{
    if (is_closed(tcp_client.sock))
    {
        bool silent = (tcp_client.connected == false);
        tcp_client.connected = false;

        if (!silent) log_add("socket has been closed from another side");
        return tcp_client_reconnect(silent);
    }
    
    return RET_OK;
}

int tcp_client_send(uint8_t *buff, size_t len)
{
    // TODO:    Investigate
    // Issue:   MSG_DONTWAIT returns EAGAIN if the operation would block, gotta check this too
    if (!tcp_client.connected) {
        log_add("TCP client: not connected. Message discarded");
        return 0;
    }
    
    reset_errno();
    int sent = send(tcp_client.sock, buff, len, MSG_NOSIGNAL | MSG_DONTWAIT);
    if ((size_t)sent == len)
    {
        log_add("TCP client: sent %d bytes: %s", len, get_errno_str());
    }
    else
    {
        if (get_errno() == EAGAIN) {
            tcp_client.status = SOCK_PENDING;
        } else {
            log_add("Send returned %d: (%d) %s", sent, get_errno(), get_errno_str());
        }
    }

    return sent;
}