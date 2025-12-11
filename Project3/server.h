// Group Members: 
// Sijan Shrestha
// Utsav Shah

#ifndef SERVER_H
#define SERVER_H

/* System Header Files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <ctype.h>
#include <pthread.h>

/* Local Header Files */
#include "list.h"

#define MAX_READERS 25
#define TRUE   1  
#define FALSE  0  
#define PORT 8888  
#define max_clients  30
#define DEFAULT_ROOM "Lobby"
#define MAXBUFF   2096
#define BACKLOG 2
#define MAX_ROOMS 100
#define MAX_USERS 100
#define MAX_DIRECT_CONN 50
// Could add these macros to server.h
#define READER_LOCK_ACQUIRE() do { \
    pthread_mutex_lock(&mutex); \
    numReaders++; \
    if (numReaders == 1) pthread_mutex_lock(&rw_lock); \
    pthread_mutex_unlock(&mutex); \
} while(0)

#define READER_LOCK_RELEASE() do { \
    pthread_mutex_lock(&mutex); \
    numReaders--; \
    if (numReaders == 0) pthread_mutex_unlock(&rw_lock); \
    pthread_mutex_unlock(&mutex); \
} while(0)
// External variables - defined in server.c
extern User *user_head;
extern Room *room_head;
extern int numReaders;
extern pthread_mutex_t rw_lock;
extern pthread_mutex_t mutex;
extern char const *server_MOTD;


// Server socket functions
int get_server_socket();
int start_server(int serv_socket, int backlog);
int accept_client(int serv_sock);
void sigintHandler(int sig_num);

// Client thread function
void *client_receive(void *ptr);

// Helper functions for commands
void addRoom(const char *roomname);
void addUser(int socket, const char *username);
void addUserToRoom(const char *username, const char *roomname);
User *findUserBySocket(int socket);
Room *findRoomByName(const char *roomname);
void removeUserFromRoom(const char *username, const char *roomname);
User *findUserByName(const char *username);
void addDirectConnection(const char *fromUser, const char *toUser);
void removeDirectConnection(const char *fromUser, const char *toUser);
void listAllRooms(int client_socket);
void listAllUsers(int client_socket, int requesting_socket);
void renameUser(int socket, const char *newName);
void removeAllUserConnections(const char *username);
void removeUser(int socket);
void sendMessageToRecipients(User *sender, const char *message);

#endif