// Group Members: 
// Sijan Shrestha
// Utsav Shah

#include "server.h"

#define DEFAULT_ROOM "Lobby"

char *trimwhitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0)  
        return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void *client_receive(void *ptr) {
    int client = *(int *) ptr;
    free(ptr);  // Free the allocated socket pointer
    
    int received;
    char buffer[MAXBUFF], sbuffer[MAXBUFF];  
    char tmpbuf[MAXBUFF];  
    char cmd[MAXBUFF], username[MAX_NAME_LEN];
    char *arguments[80];
    const char *delimiters = " \t\n\r";

    send(client, server_MOTD, strlen(server_MOTD), 0);

    // Create a guest username
    snprintf(username, sizeof(username), "guest%d", client);

    // Add user with write lock
    pthread_mutex_lock(&rw_lock);
    addUser(client, username);
    addUserToRoom(username, DEFAULT_ROOM);
    pthread_mutex_unlock(&rw_lock);

    while (1) {
        if ((received = read(client, buffer, MAXBUFF - 1)) > 0) {
            buffer[received] = '\0'; 
            strcpy(cmd, buffer);  
            strcpy(sbuffer, buffer);

            // Tokenize
            arguments[0] = strtok(cmd, delimiters);
            int i = 0;
            while (arguments[i] != NULL) {
                arguments[++i] = strtok(NULL, delimiters);
                if (arguments[i] != NULL)
                    arguments[i] = trimwhitespace(arguments[i]);
            }

            if (arguments[0] == NULL) {
                // Empty command
                snprintf(buffer, sizeof(buffer), "\nchat>");
                send(client, buffer, strlen(buffer), 0);
                continue;
            }

            // Locking strategy:
            // For commands that read from data: use reader lock
            // For commands that modify data: use writer lock

            if (strcmp(arguments[0], "create") == 0 && arguments[1] != NULL) {
                pthread_mutex_lock(&rw_lock);
                addRoom(arguments[1]);
                pthread_mutex_unlock(&rw_lock);

                snprintf(buffer, sizeof(buffer), "Room '%s' created.\nchat>", arguments[1]);
                send(client, buffer, strlen(buffer), 0);
            }
            else if (strcmp(arguments[0], "join") == 0 && arguments[1] != NULL) {
                pthread_mutex_lock(&rw_lock);
                User *u = findUserBySocket(client);
                if (u && findRoomByName(arguments[1])) {
                    addUserToRoom(u->username, arguments[1]);
                    snprintf(buffer, sizeof(buffer), "Joined room '%s'.\nchat>", arguments[1]);
                } else {
                    snprintf(buffer, sizeof(buffer), "Room '%s' does not exist.\nchat>", arguments[1]);
                }
                pthread_mutex_unlock(&rw_lock);
                send(client, buffer, strlen(buffer), 0);
            }
            else if (strcmp(arguments[0], "leave") == 0 && arguments[1] != NULL) {
                pthread_mutex_lock(&rw_lock);
                User *u = findUserBySocket(client);
                if (u) {
                    removeUserFromRoom(u->username, arguments[1]);
                    snprintf(buffer, sizeof(buffer), "Left room '%s'.\nchat>", arguments[1]);
                } else {
                    snprintf(buffer, sizeof(buffer), "User not found.\nchat>");
                }
                pthread_mutex_unlock(&rw_lock);
                send(client, buffer, strlen(buffer), 0);
            }
            else if (strcmp(arguments[0], "connect") == 0 && arguments[1] != NULL) {
                pthread_mutex_lock(&rw_lock);
                User *u = findUserBySocket(client);
                User *target = findUserByName(arguments[1]);
                if (u && target) {
                    addDirectConnection(u->username, target->username);
                    snprintf(buffer, sizeof(buffer), "Connected (DM) with '%s'.\nchat>", target->username);
                } else {
                    snprintf(buffer, sizeof(buffer), "User '%s' not found.\nchat>", arguments[1]);
                }
                pthread_mutex_unlock(&rw_lock);
                send(client, buffer, strlen(buffer), 0);
            }
            else if (strcmp(arguments[0], "disconnect") == 0 && arguments[1] != NULL) {
                pthread_mutex_lock(&rw_lock);
                User *u = findUserBySocket(client);
                if (u) {
                    removeDirectConnection(u->username, arguments[1]);
                    snprintf(buffer, sizeof(buffer), "Disconnected from '%s'.\nchat>", arguments[1]);
                } else {
                    snprintf(buffer, sizeof(buffer), "User not found.\nchat>");
                }
                pthread_mutex_unlock(&rw_lock);
                send(client, buffer, strlen(buffer), 0);
            }
            else if (strcmp(arguments[0], "rooms") == 0) {
                // Reader lock for listing
                pthread_mutex_lock(&mutex);
                numReaders++;
                if (numReaders == 1) pthread_mutex_lock(&rw_lock);
                pthread_mutex_unlock(&mutex);

                // Perform read
                listAllRooms(client);

                pthread_mutex_lock(&mutex);
                numReaders--;
                if (numReaders == 0) pthread_mutex_unlock(&rw_lock);
                pthread_mutex_unlock(&mutex);
            }
            else if (strcmp(arguments[0], "users") == 0) {
                // Reader lock for listing
                pthread_mutex_lock(&mutex);
                numReaders++;
                if (numReaders == 1) pthread_mutex_lock(&rw_lock);
                pthread_mutex_unlock(&mutex);

                // Perform read
                listAllUsers(client, client);

                pthread_mutex_lock(&mutex);
                numReaders--;
                if (numReaders == 0) pthread_mutex_unlock(&rw_lock);
                pthread_mutex_unlock(&mutex);
            }
            else if (strcmp(arguments[0], "login") == 0 && arguments[1] != NULL) {
                pthread_mutex_lock(&rw_lock);
                renameUser(client, arguments[1]);
                pthread_mutex_unlock(&rw_lock);

                snprintf(buffer, sizeof(buffer), "Logged in as '%s'.\nchat>", arguments[1]);
                send(client, buffer, strlen(buffer), 0);
            }
            else if (strcmp(arguments[0], "help") == 0) {
                snprintf(buffer, sizeof(buffer), 
                    "Commands:\n"
                    "login <username>\n"
                    "create <room>\n"
                    "join <room>\n"
                    "leave <room>\n"
                    "users\n"
                    "rooms\n"
                    "connect <user>\n"
                    "disconnect <user>\n"
                    "exit\n"
                    "chat>");
                send(client, buffer, strlen(buffer), 0);
            }
            else if (strcmp(arguments[0], "exit") == 0 || strcmp(arguments[0], "logout") == 0) {
                pthread_mutex_lock(&rw_lock);
                User *u = findUserBySocket(client);
                if (u) {
                    removeAllUserConnections(u->username);
                    removeUser(client);
                }
                pthread_mutex_unlock(&rw_lock);
                close(client);
                pthread_exit(NULL);
            } 
            else {
                // Sending a message:
                // Find the user who sent it
                pthread_mutex_lock(&mutex);
                numReaders++;
                if (numReaders == 1) pthread_mutex_lock(&rw_lock);
                pthread_mutex_unlock(&mutex);

                User *sender = findUserBySocket(client);

                if (sender == NULL) {
                    pthread_mutex_lock(&mutex);
                    numReaders--;
                    if (numReaders == 0) pthread_mutex_unlock(&rw_lock);
                    pthread_mutex_unlock(&mutex);
                    
                    snprintf(tmpbuf, sizeof(tmpbuf), "\nchat>");
                    send(client, tmpbuf, strlen(tmpbuf), 0);
                    continue;
                }

                // Format the message
                snprintf(tmpbuf, sizeof(tmpbuf), "\n::%s> %s\nchat>", sender->username, trimwhitespace(sbuffer));
                
                // Send message to all recipients (room members and DM connections)
                sendMessageToRecipients(sender, tmpbuf);
                
                // Also send back to sender as confirmation
                send(client, tmpbuf, strlen(tmpbuf), 0);

                pthread_mutex_lock(&mutex);
                numReaders--;
                if (numReaders == 0) pthread_mutex_unlock(&rw_lock);
                pthread_mutex_unlock(&mutex);
            }

            memset(buffer, 0, sizeof(buffer));
        } 
        else if (received == 0) {
            // Client disconnected
            pthread_mutex_lock(&rw_lock);
            User *u = findUserBySocket(client);
            if (u) {
                removeAllUserConnections(u->username);
                removeUser(client);
            }
            pthread_mutex_unlock(&rw_lock);
            close(client);
            pthread_exit(NULL);
        }
        else {
            // Error reading
            pthread_mutex_lock(&rw_lock);
            User *u = findUserBySocket(client);
            if (u) {
                removeAllUserConnections(u->username);
                removeUser(client);
            }
            pthread_mutex_unlock(&rw_lock);
            close(client);
            pthread_exit(NULL);
        }
    }

    return NULL;
}