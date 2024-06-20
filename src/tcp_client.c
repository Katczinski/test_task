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
#include <fcntl.h>

#include <string.h>
#include <unistd.h>
#include <stdbool.h>

struct tcp_client_s
{
    int sock;
    struct sockaddr_in server;
} tcp_client;

ret_code tcp_client_init(char *ip_str)
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

    // connect the client socket to server socket
    reset_errno();
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) != 0)
    {
        log_add("Failed to connect to '%s': %s\n", ip_str, get_errno_str());
        // return RET_ERROR;
    }
    //  Doing after connect. idc if I block while initializing, but when loop is running - I must not
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

    tcp_client.sock = sock;
    tcp_client.server = server;

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
        return RET_ERROR;
    }
    log_add("Connection established");
    return RET_OK;
}

ret_code tcp_client_shutdown()
{
    close(tcp_client.sock);
    log_add("TCP client shutdown");
    return RET_OK;
}

ret_code tcp_client_check_connection()
{
    //  NOT FINISHED YET
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&readfds);
    FD_SET(tcp_client.sock, &readfds);
    FD_ZERO(&writefds);
    FD_SET(tcp_client.sock, &writefds);
    FD_ZERO(&exceptfds);
    FD_SET(tcp_client.sock, &exceptfds);

    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = 1;
    // tv.tv_usec = 1;
    reset_errno();
    retval = select(tcp_client.sock + 1, &readfds, &writefds, &exceptfds, &tv);
    /* Не полагаемся на значение tv! */
    if (retval)
    {
        log_add("Данные доступны. %d: %s", retval, get_errno_str());
        /* Теперь FD_ISSET(0, &rfds) вернет истинное значение. */
        if (FD_ISSET(tcp_client.sock, &readfds))
        {
            log_add("FD_ISSET readfds: %d", FD_ISSET(tcp_client.sock, &readfds));
            uint8_t buff[1024] = {0};
            struct sockaddr_in clientaddr;
            socklen_t client_len = sizeof(struct sockaddr);
            int len = recvfrom(tcp_client.sock, buff, 1024, 0, (struct sockaddr *)&clientaddr, &client_len);
            log_add("read %d bytes: %s", len, buff);
            FD_CLR(tcp_client.sock, &readfds);
        }
        if (FD_ISSET(tcp_client.sock, &writefds))
        {
            log_add("FD_ISSET writefds: %d", FD_ISSET(tcp_client.sock, &writefds));
            FD_CLR(tcp_client.sock, &writefds);
            // return RET_OK;
        }
        if (FD_ISSET(tcp_client.sock, &exceptfds))
        {
            log_add("FD_ISSET exceptfds: %d", FD_ISSET(tcp_client.sock, &exceptfds));
            FD_CLR(tcp_client.sock, &exceptfds);
        }
        // return RET_OK;
    }
    else
        log_add("Данные не появились %d: %s", retval, get_errno_str());
    // return RET_ERROR;
    if (fcntl(tcp_client.sock, F_GETFL) == -1)
        log_add("socket is not valid: %s", get_errno_str());
    return RET_OK;
}

ret_code tcp_client_iterate(uint8_t *buff, size_t len)
{
    (void)buff;
    (void)len;
    // tcp_client_check_connection();
    int result;
    int con_ret = connect(tcp_client.sock, (struct sockaddr *)&tcp_client.server, sizeof(tcp_client.server));
    log_add("connect returned %d: %s", con_ret, get_errno_str());
    socklen_t result_len = sizeof(result);
    if (getsockopt(tcp_client.sock, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0)
    {
        // error, fail somehow, close socket
        log_add("error during getsockopt: %s", get_errno_str());
        return RET_ERROR;
    }

    if (result != 0)
    {
        // connection failed; error code is in 'result'
        log_add("error: %d", result);
        return RET_ERROR;
    }

    // socket is ready for read()/write()
    // int ret = send(tcp_client.sock, buff, len, MSG_NOSIGNAL | MSG_DONTWAIT);
    // log_add("send returned %d", ret);
    log_add("socket is connected");
    return RET_OK;
}

ret_code tcp_client_send(uint8_t *buff, size_t len)
{
    // TODO:    Investigate
    // Issue:   Send returns success for some time after the pipe has been closed from another side
    //          MSG_DONTWAIT returns EAGAIN if the operation would block, gotta check this too

    if ((size_t)send(tcp_client.sock, buff, len, MSG_NOSIGNAL | MSG_DONTWAIT) == len)
    {
        log_add("TCP client: sent %d bytes: %s", len, get_errno_str());
        return RET_OK;
    }
    return RET_ERROR;
}