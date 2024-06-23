#include "socket.h"

#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

bool is_closed(int sock)
{
    fd_set rfd;
    struct timeval tv = {0};

    FD_ZERO(&rfd);
    FD_SET(sock, &rfd);
    
    select(sock + 1, &rfd, 0, 0, &tv);
    if (!FD_ISSET(sock, &rfd)) {
        return false;
    }

    int n = 0;
    ioctl(sock, FIONREAD, &n);
    return n == 0;
}

bool is_connected(int sock)
{
    fd_set wrfd;
    struct timeval tv = {0};

    FD_ZERO(&wrfd);
    FD_SET(sock, &wrfd);

    if (select(sock + 1, 0, &wrfd, 0, &tv) > 0)
    {
        int errCode = 0;
        socklen_t len = sizeof(errCode);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &errCode, &len);

        if (errCode == 0)
            return true;
    }
    return false;
}

void flush_recv_buff(int sock)
{
    int             n = 0;
    static uint8_t  drain_buff[256] = { 0 };
    do {
        ioctl(sock, FIONREAD, &n);
        if (n > 0)
            recv(sock, drain_buff, 256, MSG_DONTWAIT);
    } while (n);
}