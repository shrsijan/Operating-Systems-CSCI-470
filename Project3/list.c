// Group Members: 
// Sijan Shrestha
// Utsav Shah

#include "list.h"

/////////////////// USER FUNCTIONS //////////////////////////

// Insert user at the first location
User* insertFirstU(User *head, int socket, char *username) {
    // Check for duplicate username
    if (findU(head, username) != NULL) {
        printf("Duplicate username: %s\n", username);
        return head;
    }
    
    // Create a new user node
    User *newUser = (User*) malloc(sizeof(User));
    if (newUser == NULL) {
        perror("malloc failed for User");
        return head;
    }
    
    newUser->socket = socket;
    strncpy(newUser->username, username, MAX_NAME_LEN - 1);
    newUser->username[MAX_NAME_LEN - 1] = '\0';
    newUser->rooms = NULL;
    newUser->directConns = NULL;
    newUser->next = head;
    
    return newUser;
}

// Find a user by username
User* findU(User *head, char* username) {
    User* current = head;
    
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Find a user by socket
User* findUBySocket(User *head, int socket) {
    User* current = head;
    
    while (current != NULL) {
        if (current->socket == socket) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Delete a user by socket
User* deleteU(User *head, int socket) {
    User *current = head;
    User *prev = NULL;
    
    while (current != NULL) {
        if (current->socket == socket) {
            // Free room list
            RoomList *rCurrent = current->rooms;
            while (rCurrent != NULL) {
                RoomList *rTemp = rCurrent;
                rCurrent = rCurrent->next;
                free(rTemp);
            }
            
            // Free direct connections
            DirectConn *dCurrent = current->directConns;
            while (dCurrent != NULL) {
                DirectConn *dTemp = dCurrent;
                dCurrent = dCurrent->next;
                free(dTemp);
            }
            
            // Remove from list
            if (prev == NULL) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return head;
        }
        prev = current;
        current = current->next;
    }
    return head;
}

// Rename a user
void renameU(User *head, int socket, const char *newName) {
    User *user = findUBySocket(head, socket);
    if (user != NULL) {
        strncpy(user->username, newName, MAX_NAME_LEN - 1);
        user->username[MAX_NAME_LEN - 1] = '\0';
    }
}

/////////////////// ROOM FUNCTIONS //////////////////////////

// Insert room at the first location
Room* insertFirstR(Room *head, const char *roomname) {
    // Check for duplicate room
    if (findR(head, roomname) != NULL) {
        printf("Duplicate room: %s\n", roomname);
        return head;
    }
    
    // Create a new room node
    Room *newRoom = (Room*) malloc(sizeof(Room));
    if (newRoom == NULL) {
        perror("malloc failed for Room");
        return head;
    }
    
    strncpy(newRoom->name, roomname, MAX_NAME_LEN - 1);
    newRoom->name[MAX_NAME_LEN - 1] = '\0';
    newRoom->users = NULL;
    newRoom->next = head;
    
    return newRoom;
}

// Find a room by name
Room* findR(Room *head, const char *roomname) {
    Room* current = head;
    
    while (current != NULL) {
        if (strcmp(current->name, roomname) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Add a user to a room
void addUserToR(Room *room, const char *username) {
    if (room == NULL) return;
    
    // Check if user already in room
    RoomUser *current = room->users;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return; // Already in room
        }
        current = current->next;
    }
    
    // Add new user to room
    RoomUser *newRoomUser = (RoomUser*) malloc(sizeof(RoomUser));
    if (newRoomUser == NULL) {
        perror("malloc failed for RoomUser");
        return;
    }
    
    strncpy(newRoomUser->username, username, MAX_NAME_LEN - 1);
    newRoomUser->username[MAX_NAME_LEN - 1] = '\0';
    newRoomUser->next = room->users;
    room->users = newRoomUser;
}

// Remove a user from a room
void removeUserFromR(Room *room, const char *username) {
    if (room == NULL) return;
    
    RoomUser *current = room->users;
    RoomUser *prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            if (prev == NULL) {
                room->users = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

/////////////////// DIRECT CONNECTION FUNCTIONS //////////////////////////

// Add a direct connection to a user
void addDirectConn(User *user, const char *targetUsername) {
    if (user == NULL) return;
    
    // Check if already connected
    if (findDirectConn(user, targetUsername) != NULL) {
        return;
    }
    
    DirectConn *newConn = (DirectConn*) malloc(sizeof(DirectConn));
    if (newConn == NULL) {
        perror("malloc failed for DirectConn");
        return;
    }
    
    strncpy(newConn->username, targetUsername, MAX_NAME_LEN - 1);
    newConn->username[MAX_NAME_LEN - 1] = '\0';
    newConn->next = user->directConns;
    user->directConns = newConn;
}

// Remove a direct connection from a user
void removeDirectConn(User *user, const char *targetUsername) {
    if (user == NULL) return;
    
    DirectConn *current = user->directConns;
    DirectConn *prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->username, targetUsername) == 0) {
            if (prev == NULL) {
                user->directConns = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Find a direct connection
DirectConn* findDirectConn(User *user, const char *targetUsername) {
    if (user == NULL) return NULL;
    
    DirectConn *current = user->directConns;
    while (current != NULL) {
        if (strcmp(current->username, targetUsername) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/////////////////// ROOM LIST FUNCTIONS //////////////////////////

// Add a room to a user's room list
void addRoomToUser(User *user, const char *roomname) {
    if (user == NULL) return;
    
    // Check if already in room
    RoomList *current = user->rooms;
    while (current != NULL) {
        if (strcmp(current->roomname, roomname) == 0) {
            return;
        }
        current = current->next;
    }
    
    RoomList *newRoomEntry = (RoomList*) malloc(sizeof(RoomList));
    if (newRoomEntry == NULL) {
        perror("malloc failed for RoomList");
        return;
    }
    
    strncpy(newRoomEntry->roomname, roomname, MAX_NAME_LEN - 1);
    newRoomEntry->roomname[MAX_NAME_LEN - 1] = '\0';
    newRoomEntry->next = user->rooms;
    user->rooms = newRoomEntry;
}

// Remove a room from a user's room list
void removeRoomFromUser(User *user, const char *roomname) {
    if (user == NULL) return;
    
    RoomList *current = user->rooms;
    RoomList *prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->roomname, roomname) == 0) {
            if (prev == NULL) {
                user->rooms = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

/////////////////// CLEANUP FUNCTIONS //////////////////////////

// Free all users
void freeAllUsers(User **head) {
    User *current = *head;
    while (current != NULL) {
        User *temp = current;
        current = current->next;
        
        // Free room list
        RoomList *rCurrent = temp->rooms;
        while (rCurrent != NULL) {
            RoomList *rTemp = rCurrent;
            rCurrent = rCurrent->next;
            free(rTemp);
        }
        
        // Free direct connections
        DirectConn *dCurrent = temp->directConns;
        while (dCurrent != NULL) {
            DirectConn *dTemp = dCurrent;
            dCurrent = dCurrent->next;
            free(dTemp);
        }
        
        free(temp);
    }
    *head = NULL;
}

// Free all rooms
void freeAllRooms(Room **head) {
    Room *current = *head;
    while (current != NULL) {
        Room *temp = current;
        current = current->next;
        
        // Free users in room
        RoomUser *uCurrent = temp->users;
        while (uCurrent != NULL) {
            RoomUser *uTemp = uCurrent;
            uCurrent = uCurrent->next;
            free(uTemp);
        }
        
        free(temp);
    }
    *head = NULL;
}