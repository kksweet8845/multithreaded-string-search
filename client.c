#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include "client.h"




extern char* optarg;
int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *servinfo;
    bool ok = true;
    /* Get the  locao host and port*/
    if(argc == 1)
    {
        printf("Not enough argument");
        exit(1);
    }

    int cmd_opt;
    while((cmd_opt = getopt(argc, argv, "h:p:")) != -1)
    {
        switch(cmd_opt)
        {
        case 'h':
            local_host = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        case '?':
            printf("Known option exists\n");
            break;
        }
    }

    printf("local host: %s\n", local_host);
    printf("port %s\n", port);

    ok = ok && init_sock(&hints, &servinfo);
    ok = ok && init_app();
    ok = ok && init_thread();
    while(1)
    {
        ok = !connect(sockfd, (struct sockaddr*) servinfo->ai_addr, (socklen_t) servinfo->ai_addrlen);
        if(ok)
            break;
    }
    // ok = ok && !(connect(sockfd, (struct sockaddr*) servinfo->ai_addr, (socklen_t) servinfo->ai_addrlen));
    ok = ok && run_console();
    ok = ok && finish_cmd();
    close(sockfd);

    return ok ? 0 : 1;
}