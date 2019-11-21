#ifndef CLIENT_H
#define CLIENT_H
#define __USE_XOPEN2K

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <pthread.h>
#include "console.h"
#include "queue.h"
#include <errno.h>

#define MAX_BUF_SIZE 10000


static char* local_host;
static char* port;
static struct list_head msg_head;
static pthread_t recv_thread;
extern int errno;
static bool sent = false;


static int sockfd;
#define RECVTHREADS 2
#define PRINTTHREADS 2



char* check_dq(int *argcp, char* argv[]);

bool do_query_cmd(int argc, char* argv[]);
void* recv_msg();

/* Support section */
char* gen_str_space(int argc, char*argv[])
{
    int i=0, len=0;
    for(i=0; i<argc; i++)
    {
        len += strlen(argv[i])+1;
    }
    char* rst = (char*)malloc(len);
    memset(rst, '\0', len);
    for(i=0; i<argc; i++)
    {
        strcat(rst, argv[i]);
        strcat(rst, " ");
    }
    rst[len-1] = '\0';
    return rst;
}




void init_queue()
{
    INIT_LIST_HEAD(&msg_head);
}

bool init_thread()
{
    /* Threads recieve message */
    bool ok = true;
    if(pthread_create(&recv_thread, NULL, recv_msg, NULL) != 0)
    {
        ok = false;
        printf("%d\n", errno);
    }
    return ok;
}


bool init_app()
{
    init_cmd();
    add_cmd("Query", do_query_cmd, "               | Query something");
    return true;
}

bool init_sock(struct addrinfo *hints, struct addrinfo **res)
{

    bool ok = true;
    memset(hints, 0, sizeof(struct addrinfo));
    int status;
    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(local_host, port, hints, res)) != 0)
    {
        fprintf(stdout, "getaddrinfo error: %s\n", gai_strerror(status));
        ok = false;
        exit(1);
    }

    sockfd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    return ok;
}


char* check_dq(int *argcp, char* argv[])
{
    bool ok = true;
    if(*argcp == 0)
    {
        ok = false;
        return NULL;
    }
    char ** aargv = argv+1;
    char* merged_str = gen_str_space(*argcp-1, aargv);
    int len = strlen(merged_str);
    char *buf = (char*) malloc(len+1);
    char *dst = buf;
    char *src = merged_str;
    bool d_state = true;
    int segment = 0;
    char c;
    while((c = *src++) != '\0')
    {
        if(c == '\"' && d_state)
        {
            d_state = 0;
            *dst++ = ' ';
        }
        else if(c == '\"' && !d_state)
        {
            d_state = 1;
            *dst++ = '\0';
            segment++;
        }
        else
        {
            if(d_state && isalpha(c))
                ok = false;
            *dst++ = c;
        }
    }
    if(!ok || d_state == 0)
        return NULL;
    char** rst = calloc(segment, sizeof(char*));
    src = buf;
    for(int i=0; i<segment; i++)
    {
        while(*(src++) == 32);
        src--;
        rst[i] = strsave(src);
        src += strlen(rst[i]) + 1;
    }
    *argcp = segment;
    return merged_str;
}



bool do_query_cmd(int argc, char* argv[])
{

    /* Check the double quote */
    bool ok = true;
    char* merged_str = check_dq(&argc, argv);
    if(merged_str == NULL)
        ok = false;
    if(!ok)
        return ok;
    int bytes = strlen(merged_str) + 1;
    void * buf = malloc(bytes);
    memcpy(buf, (void*) merged_str, bytes);
    write(sockfd, buf, bytes);
    return ok;
}


void* recv_msg()
{
    /* Keep read the message */
    bool waiting = true;
    while(sockfd)
    {
        char* buf = (char*) malloc(MAX_BUF_SIZE);
        int bytes = read(sockfd, (void*)buf, MAX_BUF_SIZE);
        if(bytes <= 0)
        {
            waiting = true;
            continue;
        }
        printf("%s \n", buf);
    }
}













#endif