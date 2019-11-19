#include "list.h"
#include <string.h>


struct TELE
{
    char* string;
    int clientfd;
    struct list_head list;
};



struct TELE* new_node(char* str, int fd)
{
    struct TELE *node = (struct TELE*)malloc(sizeof(struct TELE));
    node->string = str;
    node->clientfd = fd;
    INIT_LIST_HEAD(&node->list);
    return node;
}
