# OS Homework 2
This is a implementation of multiple client available version of simple searching string server.
```
make all
./server -r ./testdir -p 7777 -n 2 -h 127.0.0.1
./client -h 127.0.0.1 -p 7777
./client -h 127.0.0.1 -p 7777
....
```

The format of querying string is `Query "string" ["string",]`