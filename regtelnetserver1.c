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

#define BUFFER_LENGTH 256
pthread_t thread_id[10];
long client_sockets[10];
int tid = 0;

char database[32];

// Temporary database for active users
char *activeUser = "active.txt";

/* Mutex variable */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


/* Check if arrived client is 
registered or not */
int IsUserExist(char *usr)
{
    char word[25];
    memset(word,'\0', 25);

    FILE * fp = fopen(database, "r");
    if (fp == NULL)
        return -1;

    while (fscanf(fp, " %24s", word) == 1)
    {
               if(strstr(word, usr) != NULL)
        {
            fclose(fp);
            return 0;
        }
        memset(word,'\0', 25);
        }

    fclose(fp);
    return 1;
}

/* Validate user and password*/
int ValidateUserAndPassword(char *usr, char *password)
{
    char word[25];
    memset(word,'\0', 25);

    FILE * fp = fopen(database, "r");
    if (fp == NULL)
        return -1;

    while (fscanf(fp, " %24s", word) == 1)
    {
               if(strstr(word, usr) != NULL)
        {
            memset(word,'\0', 25);
            if(fscanf(fp, " %24s", word) == 1)
            {
                if(strncmp(word, password, strlen(password)-2) == 0)
                {
                    fclose(fp);
                    return 0;
                }
            }
        }
        memset(word,'\0', 25);
        }

    fclose(fp);
    return 1;
}

/* Check user active or not*/
int IsUserActive(char *user)
{
    char word[25];
    memset(word,'\0', 25);

    FILE * fp = fopen(activeUser, "r");
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


/* Get the list of registered clients name
and socket descriptor from database */
void ListOfActivatedUsers(int sock)
{
    char word[64];
    memset(word,'\0', 64);
    int i = 0;
    FILE * fp = fopen(activeUser, "r");
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


/* Register client name and socket discriptor
in the file */
int ActiveUsersDatabase(char *str)
{
    if(access(activeUser, F_OK) == -1) 
    {
        FILE *fp = fopen(activeUser, "w");
        fputs("USERNAME\t\tID\n", fp);
        fputs("-------------------------------------\n", fp);
        fclose(fp);
    }

    FILE *fp = fopen(activeUser, "a");
    if (fp== NULL)
    {
        printf("Issue in opening the Output file");
        return 1;
    }
    fputs(str, fp);
    fclose(fp);
    return 0;
}

/* Remove activated client history from database */
void RemoveActivatedUserFromDatabase(char *usrId)
{
    char word[25];
    char tmp[50];
    int cnt = 0;
    int line = 1;

    memset(word,'\0', 25);
    memset(tmp,'\0', 50);

    FILE * fp = fopen(activeUser, "r");
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
    remove(activeUser);
    rename("tmp.txt", activeUser); 

    fclose(fp);
    fclose(fpt);
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

        const int SIZE = 128;
        char str[SIZE];

        memset(str, '\0', SIZE);

        // convert socket id from int to string
        size_t len = snprintf(NULL, 0, "%d", sock);
        char* sockTmp = malloc(len + 1);
        snprintf(sockTmp, len + 1, "%d", sock);

        // If Client send LOGIN command then server register the client entry 
        // in database with user name and password
        if((strncmp(buffer, "LOGIN", 5)) == 0)
        {        
            int i = 0;
            int c = 0;
            char *UserName;
            char *Password;
            int check = 1;

            // Check message in proper format
            for(i = 0; buffer[i] != '\n'; i++)
            {
                if(buffer[i] == ' ')
                    c++;
            }

            // Get username and password
            if(c == 2)
            {
                strtok(buffer, " ");
                UserName = strtok(NULL, " ");
                Password = strtok(NULL, " ");
            }
            else
            {
                strcpy(str, "ERR : LOGIN command is not in proper format\n");
                printf("%s", str);
                len = strlen(str);

                send(sock,str, len, 0);
                c = 0;                
            }

            if(c == 2)
                check = ValidateUserAndPassword(UserName, Password);

            if(check == 0)
            {
                int a = IsUserActive(UserName);

                if(a == 0)
                {
                    strcat(str, "ERR : User already active\n");
                    printf("%s", str);
                    len = strlen(str);
                    send(sock,str, len, 0);
                }
                else
                {
                    Password[strcspn(Password, "\r\n")] = 0;
                    strcpy(str, UserName);
                    strcat(str, "\t\t\t");
                    strcat(str, sockTmp);
                    strcat(str,"\n");
    
                    // Active users
                    ActiveUsersDatabase(str);

                    send(sock, "OK\n", 3, 0);
                }
            }
            else if(c == 2)
            {
                strcpy(str, "ERR : May be username or Password are wrong.\n");
                printf("%s", str);
                len = strlen(str);

                send(sock,str, len, 0);
            }
        }
        // LIST of registered users
        else if((strncmp(buffer, "LIST", 4)) == 0)
        {            
            printf("Client (%d): LIST\n", sock);

            int a = IsUserActive(sockTmp);
            //printf("");
            if(a == 0)
            {
                // Send list of register users
                ListOfActivatedUsers(sock);    
            }
            else
            {
                memset(str, '\0', SIZE);
                strcpy(str, "ERR : User is not LOGIN in database.\nPlease first Login in database using LOGIN command\n");
                printf("%s", str);
                len = strlen(str);

                send(sock,str, len, 0);
            }
        }
        // When a connected client wants to LOGOUT the service
        else if((strncmp(buffer, "LOGOUT", 6)) == 0)
        {
            int a = IsUserActive(sockTmp);
            if(a == 0)
            {
                send(sock, "OK\n", 3, 0);
                thread_id[tid] = '\0';
                tid--;    
                close(sock);
                result = 0;
                RemoveActivatedUserFromDatabase(sockTmp);
            }
            else
            {
                memset(str, '\0', SIZE);
                strcpy(str, "ERR : User is not LOGIN in database.\nPlease first Login in database using LOGIN command\n");
                printf("%s", str);
                len = strlen(str);

                send(sock,str, len, 0);
            }
        }
        // When a connected client wants to leave the service
        else if((strncmp(buffer, "QUIT", 4)) == 0)
        {
            printf("Client (%d): Disconnecting User.\n", sock);
            thread_id[tid] = '\0';
            tid--;    
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

        if(tid == 0)
            remove(activeUser);

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
    remove(activeUser);
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
    FILE * fp = fopen(database, "r");
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
