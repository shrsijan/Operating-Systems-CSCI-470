#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_NAME_LEN 50

// Forward declarations
struct User;
struct Room;

// Node for user list within a room
typedef struct RoomUser {
    char username[MAX_NAME_LEN];
    struct RoomUser *next;
} RoomUser;

// Node for direct message connections
typedef struct DirectConn {
    char username[MAX_NAME_LEN];
    struct DirectConn *next;
} DirectConn;

// Node for room list within a user
typedef struct RoomList {
    char roomname[MAX_NAME_LEN];
    struct RoomList *next;
} RoomList;

// User structure
typedef struct User {
    int socket;
    char username[MAX_NAME_LEN];
    RoomList *rooms;           // Rooms this user belongs to
    DirectConn *directConns;   // Direct connections (DMs)
    struct User *next;
} User;

// Room structure  
typedef struct Room {
    char name[MAX_NAME_LEN];
    RoomUser *users;           // Users in this room
    struct Room *next;
} Room;

/////////////////// USER FUNCTIONS //////////////////////////
User* insertFirstU(User *head, int socket, char *username);
User* findU(User *head, char* username);
User* findUBySocket(User *head, int socket);
User* deleteU(User *head, int socket);
void renameU(User *head, int socket, const char *newName);

/////////////////// ROOM FUNCTIONS //////////////////////////
Room* insertFirstR(Room *head, const char *roomname);
Room* findR(Room *head, const char *roomname);
void addUserToR(Room *room, const char *username);
void removeUserFromR(Room *room, const char *username);

/////////////////// DIRECT CONNECTION FUNCTIONS //////////////////////////
void addDirectConn(User *user, const char *targetUsername);
void removeDirectConn(User *user, const char *targetUsername);
DirectConn* findDirectConn(User *user, const char *targetUsername);

/////////////////// ROOM LIST FUNCTIONS //////////////////////////
void addRoomToUser(User *user, const char *roomname);
void removeRoomFromUser(User *user, const char *roomname);

/////////////////// CLEANUP FUNCTIONS //////////////////////////
void freeAllUsers(User **head);
void freeAllRooms(Room **head);

#endif