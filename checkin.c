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
#include <signal.h>

#define BUFFER_LENGTH 512

pthread_t thread_id[10];
long client_sockets[10];
int tid = 0;

char database[32];


/* Mutex variable */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


/* Check user register or not*/
int IsUserRegistered(char *user)
{
    char word[25];
    memset(word,'\0', 25);

    FILE * fp = fopen(database, "r");
    if (fp == NULL)
    {
        return -1;
    }

    while (fscanf(fp, " %24s", word) == 1)
    {
               if(strstr(word, user) != NULL)
        {
            fclose(fp);
            return 0;        
        }
        memset(word,'\0', 25);
        }

    fclose(fp);
    return 1;
}


/* Update location in database */
void UpdateLocation(char *usr, char *loc)
{
    char word[25];
    char tmp[50];
    int cnt = 0;
    int line = 1;

    memset(word,'\0', 25);
    memset(tmp,'\0', 50);

    FILE * fp = fopen(database, "r");
    FILE * fpt = fopen("tmp.txt", "w");
    if (fp == NULL)
        return;

    usr[strcspn(usr, "\r\n")] = 0;
    while (fgets(word, 50, fp) != NULL)
    {
               if(strstr(word, usr) != NULL)
        {
            cnt++;
            break;
        }
        cnt++;
        memset(word,'\0', 25);
        }
    rewind(fp);
    cnt = (cnt/2);
    while ((fgets(tmp, 50, fp)) != NULL)
    {
            if(cnt != line)
                    fputs(tmp, fpt);
        else
        {
            char str11[50];
            memset(str11, '\0', 50);
            strcpy(str11, usr);
            strcat(str11, "\t\t\t");
            strcat(str11, loc);
            strcat(str11, "\n");
            fputs(str11, fpt);
        }
            line++;
        }
    remove(database);
    rename("tmp.txt", database); 

    fclose(fp);
    fclose(fpt);
}

/* Remove client from database */
void RemoveUserFromDatabase(char *usrId)
{
    char word[25];
    char tmp[50];
    int cnt = 0;
    int line = 1;

    memset(word,'\0', 25);
    memset(tmp,'\0', 50);

    FILE * fp = fopen(database, "r");
    FILE * fpt = fopen("tmp.txt", "w");
    if (fp == NULL)
        return;

    while (fscanf(fp, " %24s", word) == 1)
    {
               if(strstr(word, usrId) != NULL)
        {
            cnt++;
            break;
        }
        cnt++;
        memset(word,'\0', 25);
        }
    rewind(fp);
    cnt = (cnt/2);
    while ((fgets(tmp, 50, fp)) != NULL)
    {
            if(cnt != line)
                    fputs(tmp, fpt);
            line++;
        }
    remove(database);
    rename("tmp.txt", database); 

    fclose(fp);
    fclose(fpt);
}

/* Get the list of registered clients name
from database */
void ListOfRegisteredUsers(int sock)
{
    char word[64];
    memset(word,'\0', 64);
    int i = 0;
    FILE * fp = fopen(database, "r");
    if (fp == NULL)
        return;

    while ((fgets(word, 64, fp)) != NULL)
    {
        size_t len = strlen(word);
        send(sock, word, len, 0);
            memset(word,'\0', 64);
        }
    
    fclose(fp);
}

/* Register client name and location
in the file */
int RegisterUser(char *str)
{
    if(access(database, F_OK) == -1) 
    {
        FILE *fp = fopen(database, "w");
        fputs("USERNAME\t\tLocation\n", fp);
        fputs("-------------------------------------\n", fp);
        fclose(fp);
    }

    FILE *fp = fopen(database, "a");
    if (fp== NULL)
    {
        printf("Issue in opening the Output file");
        return 1;
    }
    fputs(str, fp);
    fclose(fp);
    return 0;
}


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
        if((strncmp(buffer, "CHECKIN", 7)) == 0)
        {        
            printf("Client (%d): CHECKIN\n", sock);

            int i = 0;
            int c = 0;
            char *UserName;
            char *Location;
            int check = 1;

            // Check message in proper format
            for(i = 0; buffer[i] != '\n'; i++)
            {
                if(buffer[i] == ' ')
                    c++;
            }

            // Get username and location
            if(c == 2)
            {
                strtok(buffer, " ");
                UserName = strtok(NULL, " ");
                Location = strtok(NULL, " ");
            }            
            else
            {
                strcpy(str, "ERR : CHECKIN command is not in proper format\n");
                printf("%s", str);
                len = strlen(str);

                send(sock,str, len, 0);
                c = 0;                
            }
            if(c == 2)
                check = IsUserRegistered(UserName);

            if(check == 0)
            {
                UserName[strcspn(UserName, "\r\n")] = 0;
                Location[strcspn(Location, "\r\n")] = 0;
                UpdateLocation(UserName, Location);
                send(sock,"OK\n", 3, 0);
            }
            else if(c == 2)
            {
                Location[strcspn(Location, "\r\n")] = 0;

                memset(str, '\0', 512);
                strcpy(str, UserName);
                strcat(str, "\t\t\t");
                strcat(str, Location);
                strcat(str, "\n");
                RegisterUser(str);
                send(sock,"OK\n", 3, 0);
            }
        }
        // LIST of all Quotes
        else if((strncmp(buffer, "LIST", 4)) == 0)
        {            
            printf("Client (%d): LIST\n", sock);

            ListOfRegisteredUsers(sock);
        }
        // When a connected client wants to LOGOUT the service
        else if((strncmp(buffer, "CHECKOUT", 8)) == 0)
        {
            send(sock, "OK\n", 3, 0);
            thread_id[tid] = '\0';
            tid--;    
            free(str);
            close(sock);
            result = 0;    
            RemoveUserFromDatabase(sockTmp);    
        }
        // If a registered client sends an unrecognizable request
        else
        {
            printf("Client (%d): Unrecognizable Message. Discarding UNKNOWN Message.\n", sock);
            strcpy(str, "UNKNOWN Message. Discarding UNKNOWN Message.\n");
                
            len = strlen(str);

            send(sock, str, len, 0);
        }
    
        if(tid == 0)
            remove(database);

        // Release mutex lock
        pthread_mutex_unlock(&lock);

        memset(buffer, '\0', BUFFER_LENGTH);

    }while(result);

    return 0;
}

/* If user close server program using ctrl+c 
then removed active users database file before 
terminate the program */
void My_Handler(int s)
{
    remove(database);
        exit(1); 
}


/* Main entry point of program */
int main(int argc, char **argv)
{
    int sd;
    int rc;
    int on;

    struct sockaddr_in serveraddr;
    struct sockaddr_in their_addr;
    struct sigaction sigHandler;

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

        // Raise signal if client terminate the program
        sigHandler.sa_handler = My_Handler;
        sigemptyset(&sigHandler.sa_mask);
        sigHandler.sa_flags = 0;

        sigaction(SIGINT, &sigHandler, NULL);

        tid++;
    }
    return 0;
}
