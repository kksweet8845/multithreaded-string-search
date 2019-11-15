CC = gcc -std=c99
CCFLAGS = -Wall -O3

SERVER= server
CLIENT= client
CONSOLE=console

GIT_HOOKS := .git/hooks/applied

all: $(GIT_HOOKS) $(CONSOLE)
	$(CC) -o $(SERVER) $(CCFLAGS)  $(SERVER).c -lpthread -Iinclude
	$(CC) -o $(CLIENT) $(CCFLAGS)  $(CLIENT).c -lpthread -Iinclude $(CONSOLE).o

$(GIT_HOOKS):
	scripts/install-git-hooks

debug: $(GIT_HOOKS)
	$(CC) -o $(SERVER) $(CCFLAGS) -g $(SERVER).c -lpthread
	$(CC) -o $(CLIENT) $(CCFLAGS) -g $(CLIENT).c -lpthread

console:
	$(CC) -o $(CONSOLE).o $(CCFLAGS) -g $(CONSOLE).c -Iinclude -c

server:
	$(CC) -p $(SERVER) $(CCFLAGS) -g $(SERVER).c -lpthread

client:
	$(CC) -o $(CLIENT) $(CCFLAGS) -g $(CLIENT).c -lpthread $(CONSOLE).o

clean:
	rm -rf $(SERVER) $(CLIENT)