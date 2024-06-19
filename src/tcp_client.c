#include "tcp_client.h"
#include "errors.h"
#include "log.h"
#include "utils.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>

struct tcp_client_s
{
    int                 sock;
    struct sockaddr_in  server;
    size_t              buff_size;
    uint8_t             *buff;
    bool                connected;
} tcp_client;

ret_code tcp_client_init(char *ip_str, uint8_t *buff, size_t buff_size)
{
    int sock = 0;
    int flags = 0;
    struct sockaddr_in server;

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

    flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1)
    {
        log_add("Failed to read socket flags: %s", get_errno_str());
        return RET_ERROR;
    }

    flags |= O_NONBLOCK;
    if (fcntl(sock, F_SETFL, flags) != 0)
    {
        log_add("Failed to set socket flags: %s", get_errno_str());
        return RET_ERROR;
    }

    // connect the client socket to server socket
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) != 0)
    {
        log_add("Failed to connect to '%s': %s\n", ip_str, get_errno_str());
        tcp_client.connected = false;
        // return RET_ERROR;
    }
    else
        tcp_client.connected = true;

    tcp_client.sock = sock;
    tcp_client.server = server;
    tcp_client.buff = buff;
    tcp_client.buff_size = buff_size;

    log_add("TCP client inited on %s", ip_str);

    return RET_OK;
}

ret_code tcp_client_reconnect()
{
    log_add("Reconnecting to TCP server...");
    close(tcp_client.sock);
    if ((tcp_client.sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        log_add("Failed to create socket: %s", get_errno_str());
        return RET_ERROR;
    }
    if (connect(tcp_client.sock, (struct sockaddr *)&tcp_client.server, sizeof(tcp_client.server)) != 0)
    {
        log_add("Failed to reconnect: %s\n", get_errno_str());
        tcp_client.connected = false;
        return RET_ERROR;
    }
    log_add("Connection established");
    tcp_client.connected = true;
    return RET_OK;
}

ret_code tcp_client_shutdown()
{
    close(tcp_client.sock);
    log_add("TCP client shutdown");
    return RET_OK;
}

void tcp_client_check_connection()
{
    // NOT IMPLEMENTED YET
}

ret_code tcp_client_iterate()
{
    // check if connected
    // do something else
    return RET_OK;
}

ret_code tcp_client_send_buff(size_t len)
{
    // TODO:    Investigate
    // Issue:   Send returns success for some time after the pipe has been closed from another side
    //          MSG_DONTWAIT returns EAGAIN if the operation would block, gotta check this too
    if ((size_t)send(tcp_client.sock, tcp_client.buff, len, MSG_NOSIGNAL | MSG_DONTWAIT) == len)
        return RET_OK;
    return RET_ERROR;
}