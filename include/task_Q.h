#include "list.h"
#include <string.h>


struct TELE
{
    char* string;
    struct list_head list;
};



struct TELE* new_node(char* str)
{
    struct TELE *node = (struct TELE*)malloc(sizeof(struct TELE));
    node->string = str;
    INIT_LIST_HEAD(&node->list);
    return node;
}
