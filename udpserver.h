#ifndef UDPSERVER_H
#define UDPSERVER_H

typedef struct {
    IPaddress address;
    int active;
    int id;
} Client;

#define MAX_CLIENTS 3

void initiateServer(int argc, char **argv);

#endif // UDPSERVER_H
