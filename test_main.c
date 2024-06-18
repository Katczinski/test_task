#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define array_size(n) (sizeof(n) / sizeof(n[0]))

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Invalid number of arguments\n");
        return 1;
    } 

    int sock = 0;
    struct sockaddr_in server;
    int slen = sizeof(server);

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("Failed to create a socket\n");
        return 1;
    }
    long port = strtol(argv[2], NULL, 10);
    server.sin_family = AF_INET;
	server.sin_port = htons(port);
    if (inet_pton(AF_INET, argv[1], &(server.sin_addr)) != 1)
	{
        printf("Invalid IP address\n");
        return 1;
    }
    char *msgs[] = { "Hello", "I", "am", "a", "test", "UDP", "Client" };
    size_t i = 0;
    while (1) {
        char *msg = msgs[i];
        int bytes = sendto(sock, msg, strlen(msg) , 0 , (struct sockaddr *) &server, slen);
        if (bytes > 0) {
            printf("Sent %d bytes: %s\n", bytes, msg);
            if (++i >= array_size(msgs))
                i = 0;
        } else {
            printf("Failed to send a message: %s\n", strerror(errno));
        }
        sleep(2);
    }

    return 0;
}