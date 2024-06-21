#include "tcp_client.h"
#include "errors.h"
#include "log.h"
#include "utils.h"
#include "socket.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>


struct tcp_client_s
{
    struct sockaddr_in  server;
    int                 sock;
} tcp_client;

ret_code tcp_client_init(char *ip_str)
{
    int                 sock = 0;
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

    reset_errno();
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) != 0)
    {
        log_add("Failed to connect to '%s': %s\n", ip_str, get_errno_str());
    }

    tcp_client.sock = sock;
    tcp_client.server = server;

    log_add("TCP client inited on %s", ip_str);

    return RET_OK;
}

ret_code tcp_client_reconnect()
{
//  Since this is performed every iteration of the main loop (when connection is down), to avoid clogging up the log it goes silent after the first failed connect
    static bool silent = false;

    if (!silent) log_add("Reconnecting to TCP server...");

    close(tcp_client.sock);
    if ((tcp_client.sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        if (!silent) log_add("Failed to create socket: %s", get_errno_str());
        silent = true;
        return RET_ERROR;
    }

    if (connect(tcp_client.sock, (struct sockaddr *)&tcp_client.server, sizeof(tcp_client.server)) != 0)
    {
        if (!silent) log_add("Failed to reconnect: %s\n", get_errno_str());
        silent = true;
        return RET_ERROR;
    }

    silent = false;
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
    bool closed = is_closed(tcp_client.sock);
    bool connected = is_connected(tcp_client.sock);
    return (closed || !connected) ? RET_ERROR : RET_OK;
}

int tcp_client_send(uint8_t *buff, size_t len)
{
    reset_errno();
    int sent = send(tcp_client.sock, buff, len, MSG_NOSIGNAL | MSG_DONTWAIT);
    if (sent > 0)
    {
        log_add("TCP client: sent %d bytes: %s", len, get_errno_str());
    }
    else
    {
        log_add("Send returned %d: (%d) %s", sent, get_errno(), get_errno_str());
    }

    return sent;
}