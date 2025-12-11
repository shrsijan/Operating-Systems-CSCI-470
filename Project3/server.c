// Group Members: 
// Sijan Shrestha
// Utsav Shah

#include "server.h"

int chat_serv_sock_fd; // Server socket

/////////////////////////////////////////////
// USE THESE LOCKS AND COUNTER TO SYNCHRONIZE

int numReaders = 0; // Keep count of the number of readers

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // mutex lock
pthread_mutex_t rw_lock = PTHREAD_MUTEX_INITIALIZER;  // read/write lock

/////////////////////////////////////////////

char const *server_MOTD = "Thanks for connecting to the BisonChat Server.\n\nchat>";

User *user_head = NULL;   // List of all users
Room *room_head = NULL;   // List of all rooms

int main(int argc, char **argv) {

    signal(SIGINT, sigintHandler);
    
    //////////////////////////////////////////////////////
    // Create the default room for all clients to join when 
    // initially connecting
    //////////////////////////////////////////////////////
    room_head = insertFirstR(room_head, DEFAULT_ROOM);

    // Open server socket
    chat_serv_sock_fd = get_server_socket();

    // Step 3: get ready to accept connections
    if (start_server(chat_serv_sock_fd, BACKLOG) == -1) {
        printf("start server error\n");
        exit(1);
    }
   
    printf("Server Launched! Listening on PORT: %d\n", PORT);
    
    // Main execution loop
    while (1) {
        // Accept a connection, start a thread
        int new_client = accept_client(chat_serv_sock_fd);
        if (new_client != -1) {
            int *client_socket = malloc(sizeof(int));
            *client_socket = new_client;
            pthread_t new_client_thread;
            pthread_create(&new_client_thread, NULL, client_receive, (void *)client_socket);
            pthread_detach(new_client_thread);
        }
    }

    close(chat_serv_sock_fd);
    return 0;
}

int get_server_socket() {
    int opt = TRUE;   
    int master_socket;
    struct sockaddr_in address; 
    
    // Create a master socket  
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }   
     
    // Set master socket to allow multiple connections
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }   
     
    // Type of socket created  
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons(PORT);   
         
    // Bind the socket to localhost port 8888  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }   

    return master_socket;
}

int start_server(int serv_socket, int backlog) {
    int status = 0;
    if ((status = listen(serv_socket, backlog)) == -1) {
        printf("socket listen error\n");
    }
    return status;
}

int accept_client(int serv_sock) {
    int reply_sock_fd = -1;
    socklen_t sin_size = sizeof(struct sockaddr_storage);
    struct sockaddr_storage client_addr;

    // Accept a connection request from a client
    if ((reply_sock_fd = accept(serv_sock, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
        printf("socket accept error\n");
    }
    return reply_sock_fd;
}

/* Handle SIGINT (CTRL+C) */
void sigintHandler(int sig_num) {
    printf("\nShutting down server gracefully...\n");
   
    // Acquire write lock for cleanup
    pthread_mutex_lock(&rw_lock);
   
    // Close all client sockets and notify users
    User *current = user_head;
    while (current != NULL) {
        send(current->socket, "Server is shutting down. Goodbye!\n", 36, 0);
        close(current->socket);
        current = current->next;
    }
   
    // Free all users
    freeAllUsers(&user_head);
   
    // Free all rooms
    freeAllRooms(&room_head);
   
    // Release write lock
    pthread_mutex_unlock(&rw_lock);
   
    printf("--------CLOSING ACTIVE USERS--------\n");
   
    close(chat_serv_sock_fd);
    exit(0);
}

/////////////////////////////////////////////
// Helper function implementations
/////////////////////////////////////////////

void addRoom(const char *roomname) {
    room_head = insertFirstR(room_head, roomname);
}

void addUser(int socket, const char *username) {
    user_head = insertFirstU(user_head, socket, (char *)username);
}

void addUserToRoom(const char *username, const char *roomname) {
    Room *room = findR(room_head, roomname);
    User *user = findU(user_head, (char *)username);
    
    if (room != NULL && user != NULL) {
        addUserToR(room, username);
        addRoomToUser(user, roomname);
    }
}

User *findUserBySocket(int socket) {
    return findUBySocket(user_head, socket);
}

Room *findRoomByName(const char *roomname) {
    return findR(room_head, roomname);
}

void removeUserFromRoom(const char *username, const char *roomname) {
    Room *room = findR(room_head, roomname);
    User *user = findU(user_head, (char *)username);
    
    if (room != NULL) {
        removeUserFromR(room, username);
    }
    if (user != NULL) {
        removeRoomFromUser(user, roomname);
    }
}

User *findUserByName(const char *username) {
    return findU(user_head, (char *)username);
}

void addDirectConnection(const char *fromUser, const char *toUser) {
    User *from = findU(user_head, (char *)fromUser);
    User *to = findU(user_head, (char *)toUser);
    
    if (from != NULL && to != NULL) {
        // Add bidirectional connection
        addDirectConn(from, toUser);
        addDirectConn(to, fromUser);
    }
}

void removeDirectConnection(const char *fromUser, const char *toUser) {
    User *from = findU(user_head, (char *)fromUser);
    User *to = findU(user_head, (char *)toUser);
    
    if (from != NULL) {
        removeDirectConn(from, toUser);
    }
    if (to != NULL) {
        removeDirectConn(to, fromUser);
    }
}

void listAllRooms(int client_socket) {
    char buffer[MAXBUFF];
    char temp[MAXBUFF];
    
    strcpy(buffer, "Rooms:\n");
    
    Room *current = room_head;
    while (current != NULL) {
        snprintf(temp, sizeof(temp), "  - %s\n", current->name);
        strncat(buffer, temp, sizeof(buffer) - strlen(buffer) - 1);
        current = current->next;
    }
    
    strncat(buffer, "chat>", sizeof(buffer) - strlen(buffer) - 1);
    send(client_socket, buffer, strlen(buffer), 0);
}

void listAllUsers(int client_socket, int requesting_socket) {
    char buffer[MAXBUFF];
    char temp[MAXBUFF];
    
    strcpy(buffer, "Users:\n");
    
    User *current = user_head;
    while (current != NULL) {
        snprintf(temp, sizeof(temp), "  - %s\n", current->username);
        strncat(buffer, temp, sizeof(buffer) - strlen(buffer) - 1);
        current = current->next;
    }
    
    strncat(buffer, "chat>", sizeof(buffer) - strlen(buffer) - 1);
    send(client_socket, buffer, strlen(buffer), 0);
}

void renameUser(int socket, const char *newName) {
    User *user = findUBySocket(user_head, socket);
    if (user == NULL) return;
    
    char oldName[MAX_NAME_LEN];
    strncpy(oldName, user->username, MAX_NAME_LEN - 1);
    oldName[MAX_NAME_LEN - 1] = '\0';
    
    // Update username in user struct
    renameU(user_head, socket, newName);
    
    // Update username in all rooms
    Room *room = room_head;
    while (room != NULL) {
        RoomUser *ru = room->users;
        while (ru != NULL) {
            if (strcmp(ru->username, oldName) == 0) {
                strncpy(ru->username, newName, MAX_NAME_LEN - 1);
                ru->username[MAX_NAME_LEN - 1] = '\0';
            }
            ru = ru->next;
        }
        room = room->next;
    }
    
    // Update direct connections of other users
    User *otherUser = user_head;
    while (otherUser != NULL) {
        DirectConn *dc = otherUser->directConns;
        while (dc != NULL) {
            if (strcmp(dc->username, oldName) == 0) {
                strncpy(dc->username, newName, MAX_NAME_LEN - 1);
                dc->username[MAX_NAME_LEN - 1] = '\0';
            }
            dc = dc->next;
        }
        otherUser = otherUser->next;
    }
}

void removeAllUserConnections(const char *username) {
    User *user = findU(user_head, (char *)username);
    if (user == NULL) return;
    
    // Remove user from all rooms
    RoomList *rl = user->rooms;
    while (rl != NULL) {
        Room *room = findR(room_head, rl->roomname);
        if (room != NULL) {
            removeUserFromR(room, username);
        }
        rl = rl->next;
    }
    
    // Remove all direct connections (bidirectional)
    DirectConn *dc = user->directConns;
    while (dc != NULL) {
        User *otherUser = findU(user_head, dc->username);
        if (otherUser != NULL) {
            removeDirectConn(otherUser, username);
        }
        dc = dc->next;
    }
}

void removeUser(int socket) {
    user_head = deleteU(user_head, socket);
}

// Send message to all users in same rooms or with direct connections
void sendMessageToRecipients(User *sender, const char *message) {
    if (sender == NULL) return;
    
    // Track which users we've already sent to (to avoid duplicates)
    int sentSockets[MAX_USERS];
    int sentCount = 0;
    
    // Send to all users in the same rooms
    RoomList *rl = sender->rooms;
    while (rl != NULL) {
        Room *room = findR(room_head, rl->roomname);
        if (room != NULL) {
            RoomUser *ru = room->users;
            while (ru != NULL) {
                User *recipient = findU(user_head, ru->username);
                if (recipient != NULL && recipient->socket != sender->socket) {
                    // Check if we've already sent to this user
                    int alreadySent = 0;
                    for (int i = 0; i < sentCount; i++) {
                        if (sentSockets[i] == recipient->socket) {
                            alreadySent = 1;
                            break;
                        }
                    }
                    if (!alreadySent) {
                        send(recipient->socket, message, strlen(message), 0);
                        sentSockets[sentCount++] = recipient->socket;
                    }
                }
                ru = ru->next;
            }
        }
        rl = rl->next;
    }
    
    // Send to all direct connections
    DirectConn *dc = sender->directConns;
    while (dc != NULL) {
        User *recipient = findU(user_head, dc->username);
        if (recipient != NULL && recipient->socket != sender->socket) {
            // Check if we've already sent to this user
            int alreadySent = 0;
            for (int i = 0; i < sentCount; i++) {
                if (sentSockets[i] == recipient->socket) {
                    alreadySent = 1;
                    break;
                }
            }
            if (!alreadySent) {
                send(recipient->socket, message, strlen(message), 0);
                sentSockets[sentCount++] = recipient->socket;
            }
        }
        dc = dc->next;
    }
}