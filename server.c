#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include "server.h"

#define MAX_FD 30

int main(int argc, char *argv[])
{
    /* Define the read descriptor */
    fd_set read_fds;
    int newfd; /* The new accepted socket descriptor */


    struct addrinfo hints, *servinfo;
    bool ok = true;
    int cmd_opt;
    int client_size;
    int yes; /* For the setsockopt */
    int fd_max; /* The max number of fd */
    struct sockaddr_storage *client;


    while((cmd_opt = getopt(argc, argv, "r:p:n:h:")) != -1)
    {
        switch(cmd_opt)
        {
        case 'r':
            root_dir = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        case 'n':
            NUMTHREADS = atoi(optarg);
            break;
        case 'h':
            local_host = optarg;
            break;
        case '?':
            printf("Unknown opt\n");
            break;
        }
    }
    // printf("root_dir %s\n", root_dir);
    // printf("port %s\n", port);
    // printf("number threads %d\n", NUMTHREADS);
    // printf("localHost %s\n", local_host);
    ok = ok && init_queue();
    ok = ok && open_files();
    ok = ok && init_condMutex();
    ok = ok && init_threads();
    ok = ok && init_sock(&hints, &servinfo);
    ok = ok && init_fd(&master) && init_fd(&read_fds);
    if(!ok)
    {
        printf("fuck\n");
    }
    /* Set the reuse the port */
    ok = ok && !setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if(!ok)
    {
        printf("set sock option failed\n");
        printf("error code : %d\n", errno);
    }
    /* Bind the socket with the port of this computer */
    ok = ok && !bind(sockfd, (struct sockaddr*) servinfo->ai_addr, (socklen_t) servinfo->ai_addrlen);
    if(!ok)
    {
        printf("bind errno : %d\n", errno);
    }
    /* Listen this port whether there is any request */
    ok = ok && !listen(sockfd, SOMAXCONN);
    if(!ok)
    {
        printf("listen errno : %d\n", errno);
    }
    if(!ok)
    {
        printf("errno : %d\n", errno);
    }
    // printf("Numeric: %u\n", ntohl(((struct sockaddr_in*)client)->sin_addr.s_addr));
    /* accept the client */

    /* Add the sockfd into the master set */
    FD_SET(sockfd, &master);
    printf("sockfd %d\n", sockfd);
    fd_max = sockfd;

    ok = ok && init_sockstorage(&client);
    client_size = sizeof(*client);
    while(1)
    {
        /* Copy the master fd_set*/
        read_fds = master;
        int status = NEITHER;
        // printf("ok %d\n", ok);
        ok = ok && fd_selecting(&read_fds, &fd_max, &status, &clientfd);

        if(!ok)
            continue;
        printf("status : %d\n", status);
        switch(status)
        {
        case NEW_CONNECTION:
            ok = ok && accept_client(&clientfd, &client, &client_size);
            FD_SET(clientfd, &master);
            if(clientfd > fd_max)
            {
                fd_max = clientfd;
                printf("fd_max changed to %d\n", fd_max);
            }
            break;
        case READ_SOCK:
            ok = ok && read_msg_and_assign_work(clientfd);
            break;
        case NEITHER:
            break;
        }

        // ok = ok && accept_client(&clientfd, &client, &client_size);

        // while(1)
        // {
        //     /* Receive transmitted data */
        //     ok = ok && read_msg_and_assign_work(clientfd);
        //     /* Send response */
        //     if(!ok)
        //     {
        //         printf("The clientfd %d is closed!\n", clientfd);
        //         ok = true;
        //         break;
        //     }
        // }
        // free(client);
    }
    return 0;
}