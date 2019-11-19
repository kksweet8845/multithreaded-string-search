#ifndef SERVER_H
#define SERVER_H
#define __USE_XOPEN2K /* Extension from POSIX.1:2001. */
#define _DEFAULT_SOURCE
#include <netdb.h> /* struct addrinfo */
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "task_Q.h"



# define NEW_CONNECTION 0
# define READ_SOCK      1
# define NEITHER        -1



typedef struct TELE task_ele_t, *taskptr;

typedef struct FELE file_ele_t, *fsptr;

typedef struct SELE search_ele_t, *sptr;

struct FELE
{
    FILE* fptr;
    char* filename;
    char* path;
    struct list_head list;
};


struct NELE
{
    int clientfd, client_size;
    struct sockaddr_storage *client;
    pthread_t *thread;
    struct list_head list;
};
struct SELE
{
    char* target;
    char* filename;
    int count;
    struct NELE *sock;
    struct list_head list;
};


/* Some static variable */
static char *local_host, *port, *root_dir;
extern int errno;
extern char* optarg;
static int NUMTHREADS, sockfd, clientfd, num_files, MAX_FILES;
static task_ele_t taskHead, waitHead;
pthread_cond_t task_cond, wait_cond;
pthread_mutex_t task_mutex, wait_mutex, file_mutex;
pthread_t *threads, send_threads;
static struct list_head fileHead;
fd_set master;




void* worker();
int str_match( const char*, const char*);
void search_string(const char*, struct list_head *);
char* parse_sstruct(struct list_head *);



bool init_fd(fd_set *fd)
{
    bool ok = true;
    FD_ZERO(fd);
    return ok;
}


bool init_sock(struct addrinfo *hints, struct addrinfo **res)
{
    bool ok = true;
    /* Keep the hints struct all zero */
    memset(hints, 0, sizeof(struct addrinfo));
    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_PASSIVE;

    int status;
    if((status = getaddrinfo(local_host, port, hints, res)) != 0)
    {
        fprintf(stdout, "getaddrinfo error: %s\n", gai_strerror(status));
        ok = false;
        exit(1);
    }

    sockfd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
    if(sockfd == -1)
    {
        ok = false;
    }
    return ok;
}

bool init_sockstorage(struct sockaddr_storage **client)
{
    bool ok = true;
    *client = (struct sockaddr_storage*) malloc(sizeof(struct sockaddr_storage));
    if(*client == NULL)
    {
        ok = false;
        printf("Init the sock storage failed\n");
        return ok;
    }
    printf("success on init storage\n");
    memset(*client, 0, sizeof(struct sockaddr_storage));
    return ok;
}

bool init_queue()
{
    bool ok = true;
    INIT_LIST_HEAD(&taskHead.list);
    INIT_LIST_HEAD(&waitHead.list);
    INIT_LIST_HEAD(&fileHead);
    return ok;
}

bool init_condMutex()
{
    pthread_cond_init(&task_cond, NULL);
    pthread_cond_init(&wait_cond, NULL);
    pthread_mutex_init(&task_mutex, NULL);
    pthread_mutex_init(&wait_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
    return true;
}

bool init_threads()
{
    threads = (pthread_t*) malloc(NUMTHREADS * sizeof(pthread_t));
    for(int i=0; i<NUMTHREADS; i++)
    {
        pthread_create(threads+i, NULL, worker, NULL);
    }
}


bool accept_client(int *clientfd, struct sockaddr_storage **client, int *client_size)
{
    bool ok = true;
    struct sockaddr_storage *sockptr = *client;
    char s[50];
    *clientfd = accept(sockfd, (struct sockaddr*) sockptr, (socklen_t*) client_size );
    if(*clientfd == -1)
    {
        printf("The acceptance is failed\n");
        ok = false;
        return ok;
    }
    // printf("clientfd: %d\n", *clientfd);
    // printf("ip_type %d\n", sockptr->ss_family);
    assert((sockptr->ss_family == AF_INET));
    inet_ntop(sockptr->ss_family, &((struct sockaddr_in*)sockptr)->sin_addr, s, sizeof(s));
    printf("Connect to %s:%u\n", s, ntohs((short)((struct sockaddr_in*)sockptr)->sin_port));
    return ok;
}

char* strsave(char *s)
{
    size_t len;
    char *ss;
    if(!s)
        return NULL;
    len = strlen(s);
    ss = malloc(len+1);
    if(!ss)
        return NULL;
    return strcpy(ss, s);
}

char** parse_dstring(char* str, int *argc)
{
    bool ok = true;
    int len = strlen(str);
    char *buf = (char*) malloc(len+1);
    char *dst = buf;
    char *src = str;
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
    *argc = segment;
    return ok ? rst : (void*)NULL;
}



bool read_msg_and_assign_work(int clientfd)
{
    bool ok = true;
    char* buf = (char*) malloc(1000);
    int bytes;
    int argc;
    bytes = read(clientfd, (void*)buf, 1000);
    if(bytes <= 0)
    {
        close(clientfd);
        printf("Connection closed. \n");
        FD_CLR(clientfd, &master);
        return ok;
    }
    printf("Query %s\n", buf);
    char** argv = parse_dstring((char*)buf, &argc);
    for(int i=0; i<argc; i++)
    {
        int len = strlen(argv[i]) +1;
        char* questStr = (char*) malloc(len);
        memset(questStr, '\0', len);
        memcpy(questStr, argv[i], len);
        task_ele_t *node = new_node(questStr, clientfd);
        pthread_mutex_lock(&task_mutex);
        list_add_tail(&node->list, &taskHead.list);
        pthread_mutex_unlock(&task_mutex);
    }
    free(buf);
    if(ok) /* Signal the thread that it should check for new work. */
        pthread_cond_signal(&task_cond);
    return ok;
}

bool response(int clientfd)
{
    bool ok = true;
    struct list_head *cur;
    task_ele_t *item, *safe;
    pthread_mutex_lock(&wait_mutex);
    while(list_empty(&waitHead.list))
    {
        pthread_cond_wait(&wait_cond, &wait_mutex);
    }
    list_for_each_entry_safe(item, safe, &waitHead.list, list)
    {
        int bytes = strlen(item->string)+1;
        printf("item string %s\n", item->string);
        char* buf = (char*) malloc(bytes);
        memset(buf, '\0', bytes);
        memcpy(buf, item->string, bytes);
        ok = ok && !(write(clientfd, buf, bytes) < 0);
        list_del_init(item);
        free(item);
        free(buf);
    }
    pthread_mutex_unlock(&wait_mutex);
    return ok;
}

void* worker()
{

    struct list_head tHead;
    INIT_LIST_HEAD(&tHead);
    struct list_head *cur;
    task_ele_t *t;
    while(1)
    {
        pthread_mutex_lock(&task_mutex);
        while(list_empty(&taskHead.list))
        {
            pthread_cond_wait(&task_cond, &task_mutex);
        }
        cur = taskHead.list.next;
        /* Extract out from the list */
        list_del_init(cur);
        list_move_tail(cur, &tHead);
        pthread_mutex_unlock(&task_mutex);

        cur = tHead.next;
        list_del_init(cur);
        t = list_entry(cur, task_ele_t, list);
        /* Some works */
        struct list_head search_head;
        INIT_LIST_HEAD(&search_head);
        search_string(t->string, &search_head);
        char* result = parse_sstruct(&search_head);
        free(t->string);
        t->string = result;

        int bytes = strlen(t->string)+1;
        char* buf = (char*) malloc(bytes);
        memset(buf, '\0', bytes);
        memcpy(buf, t->string, bytes);
        write(clientfd, buf, bytes);
    }
    pthread_exit(NULL);
}

void printdir(char* dir, char* abs_dir, int depth, struct list_head *head)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    if((dp = opendir(dir)) == NULL)
    {
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return;
    }
    chdir(dir);
    while((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name,&statbuf);
        if(S_ISDIR(statbuf.st_mode))
        {
            /* Found a directory, but ignore . and .. */
            if(strcmp(".",entry->d_name) == 0 ||
                    strcmp("..",entry->d_name) == 0)
                continue;
            /* Recurse at a new indent level */
            int bytes = strlen(abs_dir) + strlen(entry->d_name) + 2;
            char* abs_paths = (char*) malloc(bytes);
            memset(abs_paths, '\0', bytes);
            strcat(abs_paths, abs_dir);
            strcat(abs_paths, "/");
            strcat(abs_paths, entry->d_name);
            printdir(entry->d_name, abs_paths, depth+4, head);
            free(abs_paths);
        }
        else
        {
            int bytes = strlen(abs_dir) + strlen(entry->d_name) + 2;
            char* buf = (char*) malloc(bytes);
            char* filename = (char*) malloc(strlen(entry->d_name) + 1);
            fsptr node = (fsptr) malloc(sizeof(file_ele_t));
            FILE *fp;
            memset(buf, '\0', bytes);
            strcat(buf, abs_dir);
            strcat(buf, "/");
            strcat(buf, entry->d_name);
            fp = fopen(entry->d_name, "r");
            if(fp == NULL)
            {
                /**
                 * Number 2 for no such file.
                 */
                printf(" error occurred %d\n", errno);
            }
            memcpy(filename, entry->d_name, strlen(entry->d_name) + 1);
            node->filename = filename;
            node->path = buf;
            node->fptr = fp;
            INIT_LIST_HEAD(&node->list);
            list_add_tail(&node->list, head);
        }
    }
    chdir("..");
    closedir(dp);
}



bool open_files()
{
    bool ok = true;
    fsptr item;
    printdir(root_dir, root_dir, 0, &fileHead);
    return ok;
}



int str_match( const char* src, const char* target )
{

    char* head, *foot;
    char* tmp = target;
    int count = 0;
    for(head = src; *head != '\0'; head++)
    {
        foot = head;
        while(*foot++ == *tmp++)
        {
            if(*tmp == '\0')
            {
                count++;
                tmp = target;
                break;
            }
        }
        tmp = target;
    }
    return count;
}

void search_string(const char* str, struct list_head *head)
{

    file_ele_t *item;
    char *line = NULL;
    size_t len = 0, read;
    pthread_mutex_lock(&file_mutex);
    list_for_each_entry(item, &fileHead, list)
    {
        int count = 0;
        while((read = getline(&line, &len, item->fptr)) != -1)
        {
            if(read == 1) continue; /* Black line */
            count += str_match(line, str);
        }
        /* reopen the file stream */
        item->fptr = freopen(item->path, "r", item->fptr);
        sptr search_node = (sptr) malloc(sizeof(search_ele_t));
        INIT_LIST_HEAD(&search_node->list);
        search_node->filename = item->filename;
        search_node->target = str;
        search_node->count = count;
        list_add_tail(&search_node->list, head);
    }
    pthread_mutex_unlock(&file_mutex);

}

char* parse_sstruct(struct list_head *head)
{

    search_ele_t *item, *safe;
    char buf[50];
    memset(buf, '\0', 50);
    char* output = (char*) malloc(100);
    memset(output, '\0', 100);
    item = list_first_entry(head, search_ele_t, list);
    sprintf(buf, "String: \"%s\"\n", item->target);
    strcat(output, buf);
    int filecount = 0;
    list_for_each_entry_safe(item, safe, head, list)
    {
        if(item->count == 0)
        {
            free(item);
            filecount++;
            continue;
        }
        sprintf(buf, "File: %s, Count: %d\n", item->filename, item->count);
        strcat(output, buf);
        free(item);
    }
    if(filecount >= 2)
    {
        strcat(output, "Not found\n");
    }
    return output;
}


bool fd_selecting(fd_set *fdset, int *fd_max, int *statusbuf, int *clientfd)
{
    bool ok = true;

    printf("Waiting for activity!\n");
    if(select(*fd_max + 1, fdset, NULL, NULL, NULL) == -1)
    {
        printf("Select error!\n error code: %d", errno);
        exit(4);
    }

    for(int i=0; i<=*fd_max; i++)
    {
        /* Test if teh fd is part of set */
        if(FD_ISSET(i, fdset))
        {
            /* if the fd is this main process */
            if(i == sockfd)
            {
                /* Handle new connections */
                *statusbuf = NEW_CONNECTION;
                return ok;
            }
            else
            {
                /* Handle client request */
                *statusbuf = READ_SOCK;
                *clientfd = i;
                return ok;
            }
        }
    }
    *clientfd = -1;
    *statusbuf = NEITHER;
    return ok;
}


#endif