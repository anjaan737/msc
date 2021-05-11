#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

// buffers size
#define BUFFER_SIZE1 512
#define BUFFER_SIZE2 255

// Passphrase array
char *passphrase;
size_t len = 0;

// Encrypt or Decrypt algorithm
int EncryptDecryptFile(FILE *fpi, FILE *fpo, unsigned char *passphrase, unsigned char *ptr, int len) 
{
	size_t bytes = 0;
	size_t i = 0;

	// unsinged input and output buffers
	unsigned char *in_buffer = (unsigned char *)malloc(BUFFER_SIZE1 * sizeof(unsigned char));
	unsigned char *start = (unsigned char *)malloc(BUFFER_SIZE1 * sizeof(unsigned char));


	// Read input buffer from file
	while ((bytes = fread(in_buffer, sizeof(unsigned char), BUFFER_SIZE1, fpi)) > 0) 
	{	
		start = in_buffer;

		*(in_buffer + bytes) = '\0';

		// XOR buffer with passphrase and store in output buffer
		while(*in_buffer != '\0' || i < bytes)
		{
			// Recycling passphrase pointer
			if(*passphrase == '\0')
			{
				passphrase = ptr;
			}

			// XOR operation
			*in_buffer = (*in_buffer ^ *passphrase);

			// pointer arithmetic
			*in_buffer++;
			*passphrase++;
			i++;			
		}

		// Recycling pointer
		in_buffer = start;
		i = 0;

		// Write data in output file
		fwrite(in_buffer, sizeof(unsigned char), bytes, fpo);
	}

	// Free pointer
	free(start);

	return 0;
}

// Manage passphrase for encrypt file from decrpted file.
int ManagePassphrase(const char *fname, const char *passphrase)
{
	char word[BUFFER_SIZE2];
	memset(word,'\0', BUFFER_SIZE2);
	int isPwdValid = 0;

	FILE *fpod;
	FILE *fpid = fopen(".database.txt", "r");
	if(fpid == NULL)
	{
		fpod = fopen(".database.txt", "w");
		fclose(fpod);
	} 
	else
	{
		// Check if file name and pashphrase exist in databse
		while (fscanf(fpid, " %255s", word) == 1)
		{
			if(strncmp(word, fname, strlen(fname)) == 0)
			{
				fseek(fpid, +1, SEEK_CUR);
				memset(word,'\0', BUFFER_SIZE2);

				fgets(word, 255, fpid);

				// If filename and passphrase exist then file encrypt
				if(strncmp(word, passphrase, strlen(passphrase)) == 0)
				{
					isPwdValid = 1;
				}
				else
				{ // If only file name exist then enter valid passphrase
					isPwdValid = -1;
				}
			}
			memset(word,'\0', BUFFER_SIZE2);
		}
	}
	
	if(isPwdValid == 1)
	{
		fclose(fpid);
		return 1;
	}
	else if(isPwdValid == -1)
	{
		fclose(fpid);
		return -1;
	}

	// If both file name and passphrase not exist then file decrypt
	return 0;
}

// Validate the passphrase
void ValidatePassphrase()
{
	size_t i = 0;
	int flag = 1;

	memset(passphrase, '\0', BUFFER_SIZE2);

	while(flag)
	{
		printf("Enter passphrase\n");
		scanf(" %[^\n]", passphrase);

		len = strlen(passphrase);

		// If length is smaller than 20
		if(len < 20)
		{
			printf("Please enter 20 or more lenth of passphrase\n");
			flag = 1;
		}
		else 
		{// Check upper case, lower case and special charactors
			int isUper = 0;
			int isLower = 0;
			int isSpecial = 0;
			int isDigit = 0;

			for(i = 0; i < len; i++)
			{
				// upper case
				if(passphrase[i] >= 'A' && passphrase[i] <= 'Z')
				{
					isUper = 1;
				}
				// lower case
				else if(passphrase[i] >= 'a' && passphrase[i] <= 'z')
				{
					isLower = 1;
				}
				// digit
				else if(passphrase[i] >= 48 && passphrase[i] <= 57)
				{
					isDigit = 1;
				}
				// Special charactor
				else
				{
					isSpecial = 1;
				}
			}

			// If all condition satisfied then file encrypt or decrypt
			if(isUper != 0 && isLower != 0 && isSpecial != 0)
			{
				*(passphrase+len) = '\0';
				flag = 0;
			}
			else
			{
				printf("Please enter at least one uper, lower and spacial char of passphrase\n");
				flag = 1;
			}				
		}
	}
}

// Main entry code
int main(int argc, char *argv[]) 
{      
	if (argc != 3) 
	{
		printf("PLease provide enough argument\n");
		printf("./cipher <source_file> <dest_file>\n");
		return 0;
	}

	passphrase = (unsigned char*)malloc(BUFFER_SIZE2 * sizeof(unsigned char));

	// Open input file as a binary mode
	FILE *fp = fopen(argv[1], "rb");
	if (fp == NULL) 
	{
		printf("Failed to open %s file\n", argv[1]);
		return 0;
	}

	// open output file as a binary mode
	FILE *fpo = fopen(argv[2], "wb"); 
	if (fpo == NULL) 
	{
		printf("Failed to open %s file.\n", argv[2]);
		fclose(fpo);
		return 1;
	}

abc:
	ValidatePassphrase();

	int ret = ManagePassphrase(argv[1], passphrase);
	
	// Decrypt file
	if(ret == 0)
	{
		FILE *fpt = fopen(".database.txt", "a");
		if(fpt == NULL)
		{
			printf("Failed to open database file\n");
			return 0;
		}
		fprintf(fpt,"%s %s\n", argv[2], passphrase);
		EncryptDecryptFile(fp, fpo, passphrase, passphrase, len);
	}
	// Enter wrong file for encryption
	else if(ret == -1)
	{
		printf("You have entered wrong passphrase for encrypted file\n");
		goto abc;
	}
	// Encrpt file
	else if(ret == 1)
	{
		EncryptDecryptFile(fp, fpo, passphrase, passphrase, len);
	}

	// Free allocate memory
	free(passphrase);

	// Close file pointers
	fclose(fp);
	fclose(fpo);

	return 0;
}
