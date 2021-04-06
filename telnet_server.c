#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_LENGTH 256
#define MAXUSERS 10

pthread_t thread_id[MAXUSERS];
int MaxId = 0;

typedef struct Server
{
	int socket_ids[MAXUSERS];
	char name[MAXUSERS][20];
}Server_t;

Server_t ser;

/* Mutex variable */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Check is user exist
int IsUserExist(char *name, int sd)
{
	int i = 0;
	for(i = 0; i < MAXUSERS; i++)
	{
		if((strcmp(ser.name[i], name) == 0) || (ser.socket_ids[i] == sd))
		{
			return 0;
		}
	}
	return 1;
}

// Check is receiver user exist
int IsReceiverExist(char *name)
{
	int i = 0;
	for(i = 0; i < MAXUSERS; i++)
	{
		if((strncmp(ser.name[i], name, strlen(name)) == 0))
		{
			return (ser.socket_ids[i]);
		}
	}
	return -1;
}

// Get sender user name
char *GetSenderName(int sd)
{
	int i = 0;
	for(i = 0; i < MAXUSERS; i++)
	{
		if(ser.socket_ids[i] == sd)
		{
			return(ser.name[i]);
		}
	}
	return NULL;
}


// Handle multiple clients connection at the same time.
void *UsersHandler(void *arg)
{
	int sock = *(int*)&arg;

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

		int i = 0;
		int j = 0;
		//const int BUFFER_LENGTH = 128;
		char buff[BUFFER_LENGTH];

		memset(buff, '\0', BUFFER_LENGTH);

		// Server handle MAXUSERSimum 10 TCP clients
		if(MaxId > MAXUSERS)
		{
			printf("Client (%d): Database Full. Disconnecting User.\n", sock);
			printf("Error: Too Many Clients Connected.\n");
			strcpy(buff, "Too Many USERS. Dissconecting User.\n");
			send(sock,buff, strlen(buff), 0);
			MaxId--;
			close(sock);
			result = 0;
		}
		// If Client send JOIN command then server register the client entry 
		// in database with user name and socket discriptor
		else if((strncmp(buffer, "JOIN", 4)) == 0)
		{		
			printf("Client (%d): %s\n", sock, buffer);
			char name[20];
			memset(name, '\0', 20);

			// Parse name from string
			while(buffer[i] != '\r' || buffer[i+1] != '\n')
			{
				if(i > 4)
					name[j++] = buffer[i];
				i++;
			}
			name[i] = '\0';
			// If user already register
			if(IsUserExist(name, sock) == 0)
			{
				printf("Client (%d): User Already Registered. Discarding JOIN.\n", sock);
				sprintf(buff, "User Already Registered: Username (%s %d)\n", name, sock);

				send(sock,buff, strlen(buff), 0);
			}
			else
			{
				// Register the new user entry
				for(i = 0; i < MAXUSERS; i++)
				{
					if(ser.socket_ids[i] == 0) 
					{
						ser.socket_ids[i] = sock;
						strcpy(ser.name[i], name);
						break;
					}
				}

				memset(buff, '\0', 50);
				sprintf(buff, "JOIN %s Request Accepted\n", name);

				send(sock, buff, strlen(buff), 0);
			}
		}
		else if((strncmp(buffer, "LIST", 4)) == 0)
		{			
			// Check if user is register or not
			if(IsUserExist("#", sock) != 0)
			{
				printf("Unable to Locate Client (%d) in Database. Discarding LIST.\n", sock);
				strcpy(buff, "Unregistered User. Use \"JOIN <username>\" to Register.\n");

				send(sock,buff, strlen(buff), 0);
			}
			else
			{
				printf("Client (%d): LIST\n", sock);
				
				sprintf(buff, "%s","USERNAME\t\tFD\n");
				send(sock, buff, strlen(buff), 0);
				sprintf(buff, "%s","--------------------------------\n");
				send(sock, buff, strlen(buff), 0);
				
				for(i = 0; i < MAXUSERS; i++)
				{
					memset(buff, '\0', 128);
					sprintf(buff, "%s\t\t\t%d\n",ser.name[i], ser.socket_ids[i]);
					if(ser.socket_ids[i] != 0)
						send(sock, buff, strlen(buff), 0);
				}
				
				sprintf(buff, "%s","--------------------------------\n");
				send(sock, buff, strlen(buff), 0);
			}		
		}
		// If a registered client wants to send an individual message to another
		// registered client
		else if((strncmp(buffer, "MESG", 4)) == 0)
		{			
			// Check if sender client registered 
			if(IsUserExist("#", sock) != 0)
			{
				printf("Unable to Locate Client (%d) in Database. Discarding MESG.\n", sock);
				strcpy(buff, "Unregistered User. Use \"JOIN <username>\" to Register.\n");

				send(sock,buff, strlen(buff), 0);
			}
			else
			{
				printf("Client (%d): MESG\n", sock);

				char name[25];
				char msg[BUFFER_LENGTH];

				memset(name, '\0', 25);
				memset(msg, '\0', BUFFER_LENGTH);

				i = 5;
				j = 0;
				// Parse receiver client name 
				while(buffer[i] != ' ')
				{
					name[j++] = buffer[i];
					i++;
				}

				j = 0;
				// Parse message
				while(buffer[i] != '\r' && buffer[i] != '\n')
				{
					msg[j++] = buffer[i];
					i++;
				}
				msg[i] = '\0';

				int rv = IsReceiverExist(name);
				// Check if receiver client registered
				if(rv == -1)
				{
					printf("Unable to Locate Recipient (%s) in Database. Discarding MESG.\n", name);
					sprintf(buff, "Unknown Recipient (%s). MESG Discarded.\n", name);

					send(sock, buff, strlen(buff), 0);
				}
				else
				{
					// Get sender client name
					char *sender = GetSenderName(sock);	
					char senderName[25];
					memset(senderName, '\0', 25);

					for(i = 0; i < strlen(sender); i++)
						senderName[i] = sender[i];

					sprintf(buff, "FROM %s : %s\n", senderName, msg);

					send(rv, buff, strlen(buff), 0);
				}
			}	
		}
		// If a registered client wants to broadcast a message to all other registered
		// clients
		else if((strncmp(buffer, "BCST", 4)) == 0)
		{
			// Check if sender client registered 
			if(IsUserExist("#", sock) != 0)
			{
				printf("Unable to Locate Client (%d) in Database. Discarding BCST.\n", sock);
				strcpy(buff, "Unregistered User. Use \"JOIN <username>\" to Register.\n");

				send(sock,buff, strlen(buff), 0);
			}
			else
			{
				printf("Client (%d): BCST\n", sock);

				char senderName[25];
				memset(senderName, '\0', 25);

				// Parse message
				char *msg = buffer+5;				

				// Get sender client name
				char *sender = GetSenderName(sock);	

				for(i = 0; i< strlen(sender); i++)
					senderName[i] = sender[i];

				sprintf(buff, "FROM %s : %s\n", senderName, msg);

				i = 0;
				// broadcast message to all registerd clients
				while(i < MAXUSERS)
				{
					if(ser.socket_ids[i] != 0 && ser.socket_ids[i] != sock)
					{
						send(ser.socket_ids[i], buff, strlen(buff)+1, 0);
					}
					i++;
				}
			}	
		}
		// When a connected client wants to leave the service
		else if((strncmp(buffer, "QUIT", 4)) == 0)
		{
			printf("Client (%d): QUIT\n", sock);
			printf("Client (%d): LEAVE\n", sock);
			printf("Client (%d): Disconnecting User.\n", sock);

			if(IsUserExist(ser.name[sock], sock) == 0)
			{		
				for(i = 0; i < MAXUSERS; i++)
				{
					if(ser.socket_ids[i] == sock)
					{
						ser.socket_ids[i] = 0;
						memset(ser.name[i], '\0', 20);
						break;
					}
				}
				MaxId--;	
				close(sock);

				result = 0;
			}
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
			strcpy(buff, "UNKNOWN Message. Discarding UNKNOWN Message.\n");

			send(sock, buff, strlen(buff), 0);
		}

		// Release mutex lock
		pthread_mutex_unlock(&lock);

		memset(buffer, '\0', BUFFER_LENGTH);

	}while(result);

	return 0;
}


// Main entry point
int main(int argc, char **argv)
{
	if (argc != 2) 
	{
		printf("usage: ./prog3svr <svr_port>\n");
		return 0;
	}

	int sd;
	int rc;

	struct sockaddr_in ser_addr;
	struct sockaddr_in their_addr;

	// Create the socket.
	if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Failed to create socket\n");
		return 0;
	}

	int opt;
	// Set master socket to allow multiple connections, this is just a good habit,
	// it will work without this 
	if((rc = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt))) < 0)
	{
		printf("Failed setsockopt\n");
		close(sd);
		return 0;
	}

	// Configure settings of the server
	memset(&ser_addr, 0x00, sizeof(struct sockaddr_in));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(atoi(argv[1]));
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Bind the socket
	if((rc = bind(sd, (struct sockaddr *)&ser_addr, sizeof(ser_addr))) < 0)
	{
		printf("bind failed\n");
		close(sd);
		return 0;
	}

	// Listen on the socket, with 10 MAXUSERS connection requests queued
	if((rc = listen(sd, 10)) < 0)
	{
		printf("listen failed\n");
		close(sd);
		return 0;
	}

	printf("Waiting for Incoming Connections... Client\n");
	while(1)
	{	
		// Accept call creates a new socket for the incoming connection
		socklen_t sin_size = sizeof(struct sockaddr_in);

		long newSd = accept(sd, (struct sockaddr*)&their_addr, &sin_size);
		if(newSd < 0)
		{
			printf("accept failed\n");
			close(sd);
			return 0;
		}
		printf("(%ld): Connection Accepted\n", newSd);

		// Create thread for every client connection
		if(pthread_create(&thread_id[MaxId], NULL, UsersHandler, (void*)newSd) < 0)
		{
			printf("could not create thread\n");
			return 0;
		}

		MaxId++;
	}
	return 0;
}
