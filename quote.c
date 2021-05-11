#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include<pthread.h>

#define BUFFER_LENGTH 512

pthread_t thread_id[10];
long client_sockets[10];
int tid = 0;

char database[32];


/* Mutex variable */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


/* The socket handler handle multiple clients 
connection at the same time. */
void *UsersHandler(void *arg)
{
    int sock = client_sockets[tid-1];
    char buffer[BUFFER_LENGTH];
    int result = 0;

    memset(buffer, '\0', BUFFER_LENGTH);

    printf("Client (%d): Connection Handler Assigned\n", sock);

    do
    {
        // Read data from the socket
        result = read(sock, buffer, BUFFER_LENGTH);
        
        // mutex lock for synchronization
        pthread_mutex_lock(&lock);

        char *str = (char*)malloc(BUFFER_LENGTH);

        memset(str, '\0', BUFFER_LENGTH);

        // convert socket id from int to string
        size_t len = snprintf(NULL, 0, "%d", sock);
        char* sockTmp = malloc(len + 1);
        snprintf(sockTmp, len + 1, "%d", sock);

        // If Client send ADD command then server add line into database
        if((strncmp(buffer, "ADD", 3)) == 0)
        {        
            printf("Client (%d): ADD\n", sock);
            str = buffer+4;
            
            printf("str : %s\n", str);
            FILE *fp = fopen(database, "a");
            if (fp == NULL)
            {
                // If failed to add line in database
                strcpy(str, "ERR : ADD command is not able to add quote in database\n");
                printf("%s", str);
                len = strlen(str);

                send(sock,str, len, 0);
            }
            else
            {
                fputs(str, fp);
                send(sock, "OK\n", 3, 0);
                fclose(fp);
            }
        }
        // LIST of all Quotes
        else if((strncmp(buffer, "LIST", 4)) == 0)
        {            
            printf("Client (%d): LIST\n", sock);

            int cnt = 0;
            char lines[BUFFER_LENGTH];
            memset(lines,'\0', BUFFER_LENGTH);
    
            FILE * fp = fopen(database, "r");
            if (fp == NULL)
                return;

            while ((fgets(lines, BUFFER_LENGTH, fp)) != NULL)
            {
                cnt++;
                }

            memset(str,'\0', BUFFER_LENGTH);
            sprintf(str, "%d Quotes\n",cnt);

            send(sock, str, strlen(str), 0);
            sprintf(str, "--------------------\n");
            send(sock, str, strlen(str), 0);

            rewind(fp);

            while ((fgets(lines, BUFFER_LENGTH, fp)) != NULL)
            {
                len = strlen(lines);
                send(sock, lines, len, 0);
                    memset(lines,'\0', BUFFER_LENGTH);
                }
    
            fclose(fp);
    
        }
        // Serch for a word
        else if((strncmp(buffer, "Quote", 5)) == 0)
        {
            char lines[BUFFER_LENGTH];
            memset(lines,'\0', BUFFER_LENGTH);
            memset(str,'\0', BUFFER_LENGTH);

            strtok(buffer, " ");
            char *word = strtok(NULL, " ");
            word[strcspn(word, "\r\n")] = 0;

            FILE * fp = fopen(database, "r");
            if (fp == NULL)
                return;

            sprintf(str, "--------------------\n");
            send(sock, str, strlen(str), 0);

            while ((fgets(lines, BUFFER_LENGTH, fp)) != NULL)
            {
                if(strstr(lines, word) != NULL)
                {
                    len = strlen(lines);
                    send(sock, lines, len, 0);
                        memset(lines,'\0', BUFFER_LENGTH);
                }
                }
    
            fclose(fp);
        }
        // When a connected client wants to LOGOUT the service
        else if((strncmp(buffer, "LOGOUT", 6)) == 0)
        {
            send(sock, "OK\n", 3, 0);
            thread_id[tid] = '\0';
            tid--;    
            free(str);
            close(sock);
            result = 0;        
        }
        // If a registered client sends an unrecognizable request
        else
        {
            printf("Client (%d): Unrecognizable Message. Discarding UNKNOWN Message.\n", sock);
            strcpy(str, "UNKNOWN Message. Discarding UNKNOWN Message.\n");
                
            len = strlen(str);

            send(sock, str, len, 0);
        }

        // Release mutex lock
        pthread_mutex_unlock(&lock);

        memset(buffer, '\0', BUFFER_LENGTH);

    }while(result);

    return 0;
}


/* Main entry point of program */
int main(int argc, char **argv)
{
    int sd;
    int rc;
    int on;

    struct sockaddr_in serveraddr;
    struct sockaddr_in their_addr;

    if (argc != 3) 
    {
        printf("usage: <executable_name> <svr_port> <database_file>\n");
        exit(0);
    }

    memset(database, '\0', 32);

    strcpy(database, argv[2]);

    // Check database file exist 
    FILE * fp = fopen(database, "a+");
    if (fp == NULL)
    {
        printf("%s is not exist\n", database);
        return -1;
    }
    fclose(fp);

    // Create the socket.
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Server-socket() error");
        exit (-1);
    }

    // Set master socket to allow multiple connections
    if((rc = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on))) < 0)
    {
        perror("Server-setsockopt() error");
        close(sd);
        exit (-1);
    }

    // Configure settings of the server
    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(argv[1]));
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket
    if((rc = bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
    {
        perror("Server-bind() error");
        close(sd);
        exit(-1);
    }

    // Listen on the socket, with 10 max connection requests queued
    if((rc = listen(sd, 10)) < 0)
    {
        perror("Server-listen() error");
        close(sd);
        exit (-1);
    }

    printf("Waiting for Incoming Connections... Client\n");
    while(1)
    {    
        long sd2;

        // Accept call creates a new socket for the incoming connection
        socklen_t sin_size = sizeof(struct sockaddr_in);
        if((sd2 = accept(sd, (struct sockaddr*)&their_addr, &sin_size)) < 0)
        {
            perror("Server-accept() error");
            close(sd);
            exit (-1);
        }


        printf("(%ld): Connection Accepted\n", sd2);

        // Store socket descriptor in array.
        client_sockets[tid] = sd2;

        // Create thread for every client connection
        if(pthread_create( &thread_id[tid], NULL , UsersHandler, (void*)client_sockets[tid]) < 0)
        {
            perror("could not create thread");
            return 1;
        }

        tid++;
    }
    return 0;
}
