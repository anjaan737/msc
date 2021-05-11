#include<stdio.h>
#include<stdlib.h>
#include<string.h>

//Buffer size
#define BUF_SIZE 512

int main(int argc, char **argv){
    int count = 0;
    char lines[BUF_SIZE];
    memset(lines,'\0', BUF_SIZE);
    int i = 0;

    if(argc > 2){
        printf("Incorrect number of arguments\n");
        return 0;
    }

    //Reading from a file
    if(argc == 2){
        FILE * fp = fopen(argv[1], "r");
        if (fp == NULL){
            printf("Failed to opening file\n");
            return 0;
        }
        
        // Read file line by line
        while ((fgets(lines, BUF_SIZE, fp)) != NULL){
            size_t len = strlen(lines);

            for(i = 0; i < len; i++){
                char ch = lines[i];
                // Count the repeated characters
                if(ch == lines[i+1]){
                    count++;
                }
                else{ // Display character count
                    if(count != 0)
                        printf("%d%c", count+1, ch);
                    else
                        printf("%c", ch);

                    count = 0;
                }
            }
            memset(lines,'\0', BUF_SIZE);
        }
        fclose(fp);
    }
    //Reading from standard input
    else if(argc == 1)
    {
        while ((fgets(lines, BUF_SIZE, stdin)) != NULL){
            size_t len = strlen(lines);

            for(i = 0; i < len; i++){
                char ch = lines[i];

                if(ch == lines[i+1]){
                    count++;
                }
                else{
                    if(count != 0)
                        printf("%d%c", count+1, ch);
                    else
                        printf("%c", ch);

                    count = 0;
                }
            }
            memset(lines,'\0', BUF_SIZE);
        }
    }

    return 0;
}
