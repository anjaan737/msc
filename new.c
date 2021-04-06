#include <stdio.h> 
#include <string.h>
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> 

#define TRUE 1 
#define FALSE 0 

#define MAX_CLI 10
#define BUF_SIZE 512

const char* database = "database.txt";

int IsUserRegistered(char*);

int main(int argc, char **argv)
{ 
	int opt = TRUE; 
	int m_sock, new_sock, client_socket[MAX_CLI];
	int i, activity, sd, max_sd; 
 
	FILE *fp;
	struct sockaddr_in address; 

	char buffer[BUF_SIZE];

	//set of socket descriptors 
	fd_set readfds; 

	if (argc != 2) 
	{
		printf("usage: ./prog3svr <svr_port>\n");
		exit(0);
	}

	//initialise all client_socket[] to 0 so not checked 
	for (i = 0; i < MAX_CLI; i++) 
	{ 
		client_socket[i] = 0; 
	} 

	//create a master socket 
	if( (m_sock = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 

	//set master socket to allow multiple connections , 
	//this is just a good habit, it will work without this 
	if( setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 

	//type of socket created 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons(atoi(argv[1])); 

	//bind the socket to localhost port 
	if (bind(m_sock, (struct sockaddr *)&address, sizeof(address))<0) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 

	//try to specify maximum of 3 pending connections for the master socket 
	if (listen(m_sock, 3) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	} 

	//accept the incoming connection 
	int addrlen = sizeof(address); 

	printf("Waiting for Incoming Connections... Client\n");

	remove(database);

	while(TRUE) 
	{ 
		//clear the socket set 
		FD_ZERO(&readfds);  

		//add master socket to set 
		FD_SET(m_sock, &readfds);  
		max_sd = m_sock; 

		//add child sockets to set 
		for ( i = 0 ; i < MAX_CLI ; i++) 
		{ 
			//socket descriptor 
			sd = client_socket[i]; 

			//if valid socket descriptor then add to read list 
			if(sd > 0) 
				FD_SET( sd , &readfds); 

			//highest file descriptor number, need it for the select function 
			if(sd > max_sd) 
				max_sd = sd; 
		} 

		//wait for an activity on one of the sockets , timeout is NULL , 
		//so wait indefinitely 
		activity = select( max_sd + 1 , &readfds, NULL , NULL , NULL); 

		if ((activity < 0) && (errno!=EINTR)) 
		{ 
			printf("select error"); 
		} 

		//If something happened on the master socket , 
		//then its an incoming connection 
		if (FD_ISSET(m_sock, &readfds)) 
		{ 
			if ((new_sock = accept(m_sock, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
			{ 
				perror("accept"); 
				exit(EXIT_FAILURE); 
			} 

			printf("Client (%d): Connection Accepted\n", new_sock);
			printf("Client (%d): Connection Handler Assigned\n", new_sock);

			//add new socket to array of sockets 
			for (i = 0; i < MAX_CLI; i++) 
			{ 
				//if position is empty 
				if( client_socket[i] == 0 ) 
				{ 
					client_socket[i] = new_sock; 
					break; 
				} 
			} 
		} 

		//else its some IO operation on some other socket 
		for (i = 0; i < MAX_CLI; i++) 
		{ 
			int m = 0, n = 0;
			const int SIZE = 100;
			char data[SIZE];
			char recipient[50], sender[25], message[50];

			memset(data, '\0', SIZE);
			memset(recipient, '\0', 50);
			memset(sender, '\0', 50);
			memset(message, '\0', 50);

			sd = client_socket[i]; 

			// convert socket id from int to string
			size_t len = snprintf(NULL, 0, "%d", sd);
			char* sockTmp = malloc(len + 1);
			snprintf(sockTmp, len + 1, "%d", sd);

			if (FD_ISSET(sd, &readfds)) 
			{ 
				memset(buffer, '\0', BUF_SIZE);

				int result = read(sd, buffer, BUF_SIZE);

				if((strncmp(buffer, "JOIN", 4)) == 0)
				{		
					printf("Client (%d): %s\n", sd, buffer);
					char name[20];
					memset(name, '\0', 20);

					char *temp = buffer+5;

					// Parse name from string
					while(temp[m] != '\r' || temp[m+1] != '\n')
					{
						name[n++] = temp[m];
						m++;
					}

					// If user already register
					if(IsUserRegistered(sockTmp) == 0)
					{
						printf("Client (%d): User Already Registered. Discarding JOIN.\n", sd);
						sprintf(data, "%s%s%s%s%s","User Already Registered: Username (", name, "), FD (", sockTmp, ")\n");
						len = strlen(data);

						send(sd,data, len, 0);
					}
					else
					{
						sprintf(data, "%s %s %s %s", name, "\t\t\t", sockTmp, "\n");
						// Register the new user entry in database
						if(access(database, F_OK) == -1) 
						{
							fp = fopen(database, "w");
							fputs("USERNAME\t\tFD\n", fp);
							fputs("-------------------------------------\n", fp);
							fclose(fp);
						}

						fp = fopen(database, "a");
						fputs(data, fp);
						fclose(fp);

						memset(data, '\0', 50);
						sprintf(data, "%s %s %s", "JOIN", name, "Request Accepted\n");
						len = strlen(data);
						send(sd,data, len, 0);
					}
				}
				else if((strncmp(buffer, "LIST", 4)) == 0)
				{			
					// Check if user is register or not
					if(IsUserRegistered(sockTmp) != 0)
					{
						printf("Unable to Locate Client (%d) in Database. Discarding LIST.\n", sd);
						strcpy(data, "Unregistered User. Use \"JOIN <username>\" to Register.\n");
						len = strlen(data);
						send(sd,data, len, 0);
					}
					else
					{
						printf("Client (%d): LIST\n", sd);
						fp = fopen(database, "r");
						char data1[100];
						memset(data1,'\0', 100);
						while ((fgets(data1, 100, fp)) != NULL)
						{
							len = strlen(data1);
							send(sd, data1, len, 0);
							memset(data1,'\0', 100);
						}
						fclose(fp);
					}		
				}
				else if((strncmp(buffer, "MESG", 4)) == 0)
				{			
					// Check if sender client registered 
					if(IsUserRegistered(sockTmp) != 0)
					{
						printf("Unable to Locate Client (%d) in Database. Discarding MESG.\n", sd);
						strcpy(data, "Unregistered User. Use \"JOIN <username>\" to Register.\n");
						len = strlen(data);
						send(sd, data, len, 0);
					}
					else
					{
						printf("Client (%d): MESG\n", sd);

						char word[50];
						int getSd = 0, flg = 1;

						memset(word,'\0', 50);

						char *buff = buffer+5;

						m = 0;
						// Parse recipient client name and messege
						while(buff[m] != '\r' && buff[m+1] != '\n')
						{
							if((buff[m] != ' ') && (flg == 1))
								recipient[m] = buff[m];														
							else
							{
								flg = 0;
								message[n++] = buff[m];
							}
							m++;
						}
						message[n]='\0';

						fp = fopen(database, "r");
						while (fscanf(fp, " %49s", word) == 1)
						{
							if(strstr(word, recipient) != NULL)
							{
								memset(word,'\0', 50);
								if(fscanf(fp, " %49s", word) == 1)
								{
									getSd = atoi(word);
									break;
								}
							}
							memset(word,'\0', 25);
						}
						fclose(fp);

						// If recipient client is registered or not
						if(getSd == 0)
						{
							printf("Unable to Locate Recipient (%s) in Database. Discarding MESG.\n", recipient);
							sprintf(data, "%s%s%s","Unknown Recipient (", recipient, "). MESG Discarded.\n");					
							len = strlen(data);
							send(sd, data, len, 0);
						}
						else
						{
							memset(word,'\0', 50);

							fp = fopen(database, "r");

							while ((fgets(word, 50, fp)) != NULL)
							{		
								if(strstr(word, sockTmp) != NULL)
								{
									int ii = 0;
									len = strlen(word);
									for(; ii < len; ii++)
									{
										if(word[ii] != '\t')
										{
											sender[ii] = word[ii];			
										}
										else
										{
											sender[ii] = '\0';
											break;
										}
									}
								}
								memset(word,'\0', 50);
							}
							fclose(fp);

							sprintf(data, "%s%s%s%s\n", "FROM ",sender,":", message);
							len = strlen(data)+1;
							send(getSd, data, len, 0);
						}
					}	
				}
				// If a registered client wants to broadcast a message to all other registered
				// clients
				else if((strncmp(buffer, "BCST", 4)) == 0)
				{
					// Check if sender client registered 
					if(IsUserRegistered(sockTmp) != 0)
					{
						printf("Unable to Locate Client (%d) in Database. Discarding BCST.\n", sd);
						strcpy(data, "Unregistered User. Use \"JOIN <username>\" to Register.\n");
						len = strlen(data);
						send(sd, data, len, 0);
					}
					else
					{
						printf("Client (%d): BCST\n", sd);

						// Parse message
						char *message = buffer+5;				
						len = strlen(message)+1;

						char word[50];
						memset(word,'\0', 50);

						// Get sender client name
						fp = fopen(database, "r");

						while ((fgets(word, 50, fp)) != NULL)
						{		
							if(strstr(word, sockTmp) != NULL)
							{
								int ii = 0;
								len = strlen(word);
								for(; ii < len; ii++)
								{
									if(word[ii] != '\t')
									{
										sender[ii] = word[ii];			
									}
									else
									{
										sender[ii] = '\0';
										break;
									}
								}
							}
							memset(word,'\0', 50);
						}
						fclose(fp);


						sprintf(data, "%s%s%s%s\n","FROM ", sender,":", message);
						len = strlen(data)+1;

						m = 0;
						// broadcast message to all registerd clients
						while(m < 10)
						{
							if(client_socket[m] != '0' && client_socket[m] != sd)
							{
								send(client_socket[m], data, len, 0);
							}
							m++;
						}
					}	
				}
				else if((strncmp(buffer, "QUIT", 4)) == 0)
				{
					printf("Client (%d): QUIT\n", sd);
					printf("Client (%d): Disconnecting User.\n", sd);

					// Remove history of registerd client
					if(IsUserRegistered(sockTmp) == 0)
					{
						char tmp1[50];
						int c = 0, l = 0;

						memset(tmp1,'\0', 50);

						fp = fopen(database, "r");

						while (fscanf(fp, " %50s", tmp1) == 1)
						{
							if(strstr(tmp1, sockTmp) != NULL)
							{
								c++;
								break;
							}
							c++;
							memset(tmp1,'\0', 50);
						}
						rewind(fp);
						c = c/2;
						memset(tmp1,'\0', 50);

						FILE *fp1 = fopen("tmp.txt", "w");

						while ((fgets(tmp1, 50, fp)) != NULL)
						{
							if(c != l)
								fputs(tmp1, fp1);
							l++;
						}
						remove(database);
						rename("tmp.txt", database); 

						fclose(fp);
						fclose(fp1);						
					}
					client_socket[i] = 0;
					close(sd);
					free(sockTmp);
				}
				// If client send PING messge to check client conectivity
				else if((strncmp(buffer, "PING", 4)) == 0)
				{
					printf("Client (%d): PONG\n", sd);
					send(sd, "PONG\n", 5, 0);
				}
				// If a registered client sends an unrecognizable request
				else
				{
					printf("Client (%d): Unrecognizable Message. Discarding UNKNOWN Message.\n", sd);
					strcpy(data, "UNKNOWN Message. Discarding UNKNOWN Message.\n");
					len = strlen(data);
					send(sd, data, len, 0);
				}
			} 
		} 
	} 
	return 0; 
} 


int IsUserRegistered(char *usr)
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

