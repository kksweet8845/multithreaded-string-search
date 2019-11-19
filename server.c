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

int main(int argc, char *argv[])
{

    struct addrinfo hints, *servinfo;
    bool ok = true;
    int cmd_opt;
    int client_size;
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
    while(1)
    {
        init_sockstorage(&client);
        client_size = sizeof(*client);
        ok = ok && accept_client(&clientfd, &client, &client_size);

        // struct NELE *sock_node = (struct NELE*) malloc(sizeof(struct NELE));
        // sock_node->clientfd = clientfd;
        // sock_node->client = client;
        // sock_node->client_size = client_size;
        // pthread_t *th = (pthread_t*) malloc(sizeof(pthread_t));
        // pthread_create(th, NULL, sub_server, (void*) sock_node);

        while(1)
        {
            /* Receive transmitted data */
            ok = ok && read_msg_and_assign_work(clientfd);
            /* Send response */
            if(!ok)
            {
                printf("The clientfd %d is closed!\n", clientfd);
                ok = true;
                break;
            }
        }
        free(client);
    }
}