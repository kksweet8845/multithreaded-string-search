/* Implementation of simple command line interface */

/* Each command defined in terms of a function */
typedef bool (*cmd_function)(int argc, char* argv[]);


/* Information about each command */
/* Organized as linked list in alphabetical order */
typedef struct CELE cmd_ele, *cmd_ptr;

struct CELE
{
    char* name;
    cmd_function operation;
    char* documentation;
    cmd_ptr next;
};


char* strsave(char *s);

void free_block(void *b, size_t bytes);
void free_string(char* s);
void free_array(void *b);
/* Initialize interpreter */
void init_cmd();

/* Add a new command */
void add_cmd(char *name, cmd_function operation, char *documentation);

/* Execute a command from a command line */
bool interpret_cmd(char *cmdline);

bool run_console();

bool finish_cmd();

