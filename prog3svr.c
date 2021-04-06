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

/* Mutex variable */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


///////////////////////////////////////////////
// Check if arrived client is registered or
// not
//////////////////////////////////////////////
int checkUserExist(char *usr)
{
	char word[25];
	memset(word,'\0', 25);

	FILE * fp = fopen("register.txt", "r");
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

///////////////////////////////////////////////
// One registered client can send an individual 
// message to another registered client
//////////////////////////////////////////////
int SendMsgToSelectedClient(char *usr)
{
	char word[25];
	memset(word,'\0', 25);

	FILE * fp = fopen("register.txt", "r");
	if (fp == NULL)
		return -1;

	while (fscanf(fp, " %24s", word) == 1)
	{
       		if(strstr(word, usr) != NULL)
		{
			memset(word,'\0', 25);
			if(fscanf(fp, " %24s", word) == 1)
			{
				fclose(fp);
				return atoi(word);
			}
		}
		memset(word,'\0', 25);
    	}

	fclose(fp);
	return -1;
}

///////////////////////////////////////////////
// Get the registered client name from
// the database
//////////////////////////////////////////////
char *GetRegisterClientName(char *usr)
{
	char word[50];
	char tmp[25];

	memset(word,'\0', 50);
	memset(tmp,'\0', 25);

	FILE * fp = fopen("register.txt", "r");
	if (fp == NULL)
		return NULL;

	while ((fgets(word, 50, fp)) != NULL)
	{		
       		if(strstr(word, usr) != NULL)
		{
			int i = 0;
			size_t len = strlen(word);
			for(; i < len; i++)
			{
				if(word[i] != '\t')
				{
					tmp[i] = word[i];			
				}
				else
				{	
					fclose(fp);
					usr = tmp;
					return usr;
				}
			}
		}
		memset(word,'\0', 25);
    	}
	
	fclose(fp);
	return NULL;
}

///////////////////////////////////////////////
// Get the list of registered clients name
// and socket descriptor from database
//////////////////////////////////////////////
void GetTheListOfRegisterUser(int sock)
{
	char word[50];
	memset(word,'\0', 50);

	FILE * fp = fopen("register.txt", "r");
	if (fp == NULL)
		return;

	while ((fgets(word, 50, fp)) != NULL)
	{
		size_t len = strlen(word);
		send(sock,word, len, 0);
        	memset(word,'\0', 50);
    	}
	
	fclose(fp);
}

///////////////////////////////////////////////
// Remove registered client history from database 
// if registered client want to quit
//////////////////////////////////////////////
void RemoveClientEntryFromDatabase(char *usrId)
{
	char word[25];
	char tmp[50];
	int cnt = 0;
	int line = 1;

	memset(word,'\0', 25);
	memset(tmp,'\0', 50);

	FILE * fp = fopen("register.txt", "r");
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
	remove("register.txt");
	rename("tmp.txt", "register.txt"); 

	fclose(fp);
	fclose(fpt);
}

///////////////////////////////////////////////
// Register client name and socket discriptor
// in the file
//////////////////////////////////////////////
int RegisterUser(char *str)
{
	if(access("register.txt", F_OK) == -1) 
	{
		FILE *fp = fopen("register.txt", "w");
		fputs("USERNAME\t\tFD\n", fp);
		fputs("-------------------------------------\n", fp);
		fclose(fp);
	}

	FILE *fp = fopen("register.txt", "a");
	if (fp== NULL)
	{
		printf("Issue in opening the Output file");
		return 1;
	}
	fputs(str, fp);
	fclose(fp);
	return 0;
}


///////////////////////////////////////////////
// The socket handler handle multiple clients 
// connection at the same time.
//////////////////////////////////////////////
void *socketHandler(void *arg)
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
		//printf("read : %s\n", buffer);

		// mutex lock for synchronization
		pthread_mutex_lock(&lock);

		int x = 0;
		int y = 0;
		const int SIZE = 128;
		char str[SIZE];

		memset(str, '\0', SIZE);

		// convert socket id from int to string
		size_t len = snprintf(NULL, 0, "%d", sock);
		char* sockTmp = malloc(len + 1);
		snprintf(sockTmp, len + 1, "%d", sock);

		// Server handle maximum 10 TCP clients
		if(tid > 10)
		{
			printf("Client (%d): Database Full. Disconnecting User.\n", sock);
			printf("Error: Too Many Clients Connected.\n");
			strcpy(str, "Too Many USERS. Dissconecting User.\n");
			len = strlen(str);
			send(sock,str, len, 0);
			tid--;
			close(sock);
			free(sockTmp);
			result = 0;
		}
		// If Client send JOIN command then server register the client entry 
		// in database with user name and socket discriptor
		else if((strncmp(buffer, "JOIN", 4)) == 0)
		{		
			printf("Client (%d): %s\n", sock, buffer);
			char name[20];
			memset(name, '\0', 20);

			char *temp = buffer+5;
			
			// Parse name from string
			while(temp[x] != '\r' || temp[x+1] != '\n')
			{
				name[y++] = temp[x];
				x++;
			}

			int check = checkUserExist(sockTmp);
			// If user already register
			if(check == 0)
			{
				printf("Client (%d): User Already Registered. Discarding JOIN.\n", sock);
				strcpy(str, "User Already Registered: Username (");
				strcat(str, name);
				strcat(str, "), FD (");
				strcat(str, sockTmp);
				strcat(str, ")\n");
				len = strlen(str);

				send(sock,str, len, 0);
			}
			else
			{
				strcpy(str, name);
				strcat(str, "\t\t\t");
				strcat(str,sockTmp);
				strcat(str,"\n");

				// Register the new user entry in database
				RegisterUser(str);

				memset(str, '\0', 50);
				strcpy(str, "JOIN ");
				strcat(str, name);
				strcat(str," Request Accepted\n");

				len = strlen(str);

				send(sock,str, len, 0);
			}
		}
		else if((strncmp(buffer, "LIST", 4)) == 0)
		{			
			int check = checkUserExist(sockTmp);
			// Check if user is register or not
			if(check != 0)
			{
				printf("Unable to Locate Client (%d) in Database. Discarding LIST.\n", sock);
				strcpy(str, "Unregistered User. Use \"JOIN <username>\" to Register.\n");
				
				len = strlen(str);

				send(sock,str, len, 0);
			}
			else
			{
				printf("Client (%d): LIST\n", sock);
				// Send list of register users with descriptor
				GetTheListOfRegisterUser(sock);
			}		
		}
		// If a registered client wants to send an individual message to another
		// registered client
		else if((strncmp(buffer, "MESG", 4)) == 0)
		{			
			int check = checkUserExist(sockTmp);
			// Check if sender client registered 
			if(check != 0)
			{
				printf("Unable to Locate Client (%d) in Database. Discarding MESG.\n", sock);
				strcpy(str, "Unregistered User. Use \"JOIN <username>\" to Register.\n");

				len = strlen(str);

				send(sock,str, len, 0);
			}
			else
			{
				printf("Client (%d): MESG\n", sock);

				char name[25];
				char msg[SIZE];

				memset(name, '\0', 25);
				memset(msg, '\0', SIZE);

				char *temp = buffer+5;

				x = 0;
				// Parse receiver client name 
				while(temp[x] != ' ' && temp[x] != '\r' && temp[x+1] != '\n')
				{
					name[x] = temp[x];
					x++;
				}
	
				y = 0;
				// Parse message
				while(temp[x] != '\r' && temp[x] != '\n')
				{
					msg[y++] = temp[x];
					x++;
				}
				msg[y] = '\0';

				int reciverSocket = SendMsgToSelectedClient(name);
				// Check if receiver client registered
				if(reciverSocket == -1)
				{
					printf("Unable to Locate Recipient (%s) in Database. Discarding MESG.\n", name);
					strcpy(str, "Unknown Recipient (");
					strcat(str, name);
					strcat(str, "). MESG Discarded.\n");
					
					len = strlen(str);

					send(sock, str, len, 0);
				}
				else
				{
					// Get sender client name
					char *sender = GetRegisterClientName(sockTmp);	

					char senderName[25];
					memset(senderName, '\0', 25);

					len = strlen(sender);

					for(x = 0; x < len; x++)
						senderName[x] = sender[x];

					strcpy(str, "FROM ");					
					strcat(str, senderName);
					strcat(str, " : ");
					strcat(str,  msg);
					strcat(str,  "\n");
					
					len = strlen(str)+1;

					send(reciverSocket, str, len, 0);
				}
			}	
		}
		// If a registered client wants to broadcast a message to all other registered
		// clients
		else if((strncmp(buffer, "BCST", 4)) == 0)
		{
			// Check if sender client registered 
			int check = checkUserExist(sockTmp);
			if(check != 0)
			{
				printf("Unable to Locate Client (%d) in Database. Discarding BCST.\n", sock);
				strcpy(str, "Unregistered User. Use \"JOIN <username>\" to Register.\n");
				
				len = strlen(str);

				send(sock,str, len, 0);
			}
			else
			{
				printf("Client (%d): BCST\n", sock);


				char senderName[25];
				memset(senderName, '\0', 25);
				
				// Parse message
				char *msg = buffer+5;				
				len = strlen(msg)+1;

				// Get sender client name
				char *sender = GetRegisterClientName(sockTmp);	

				len = strlen(sender);

				for(x = 0; x < len; x++)
					senderName[x] = sender[x];

				strcpy(str, "FROM ");					
				strcat(str, senderName);
				strcat(str, " : ");
				strcat(str,  msg);
				strcat(str,  "\n");
					
				len = strlen(str)+1;

				x = 0;
				// broadcast message to all registerd clients
				while(x < 10)
				{
					if(client_sockets[x] != '\0' && client_sockets[x] != sock)
					{
						send(client_sockets[x], str, len, 0);
					}
					x++;
				}
			}	
		}
		// When a connected client wants to leave the service
		else if((strncmp(buffer, "QUIT", 4)) == 0)
		{
			printf("Client (%d): QUIT\n", sock);
			printf("Client (%d): LEAVE\n", sock);
			printf("Client (%d): Disconnecting User.\n", sock);
			int check = checkUserExist(sockTmp);
			// Remove history of registerd client
			if(check == 0)
				RemoveClientEntryFromDatabase(sockTmp);
			thread_id[tid] = '\0';
			tid--;	
			close(sock);
			free(sockTmp);
			result = 0;

			if(tid == 0)
				remove("register.txt");
		}
		// If client send PING messge to check client conectivity
		else if((strncmp(buffer, "PING", 4)) == 0)
		{
			printf("Client (%d): PONG\n", sock);
			send(sock, "PONG\n", 5, 0);
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

//////////////////////////////////////////////
// If user close server program using ctrl+c 
// then removed clients register file before 
// terminate the program
//////////////////////////////////////////////
void My_Handler(int s)
{
	remove("register.txt");
        exit(1); 
}

///////////////////////////////////
/// Main entry point of program
//////////////////////////////////
int main(int argc, char **argv)
{
	int sd;
	int rc;
	int on;

	struct sockaddr_in serveraddr;
	struct sockaddr_in their_addr;

	struct sigaction sigHandler;

	if (argc != 2) 
	{
		printf("usage: ./prog3svr <svr_port>\n");
		exit(0);
	}

	// Create the socket.
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Server-socket() error");
		exit (-1);
	}

	// Set master socket to allow multiple connections, this is just a good habit,
	// it will work without this 
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
		if(pthread_create( &thread_id[tid], NULL , socketHandler, (void*)client_sockets[tid]) < 0)
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
