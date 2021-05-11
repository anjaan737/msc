#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Number of threads
#define NUM_THREADS 3
// Buffer size
#define BUF_SIZE 512

// Conditional variable
int done = 0;

// Declaration of thread condition variable
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Declaring mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// File handling structure
typedef struct FileStruct
{
    char* filename;
    int start_index;
    int read_line;
    FILE *fp;
}FileStruct_t;

// Create struct object
FileStruct_t fs;

// Thread function
void* Thread_Handler(void *arg)
{
    int a = *((int*)arg);
    int cnt = 0;

    // Acquire a mutex lock
    pthread_mutex_lock(&lock);

    int count = 0;
    char lines[BUF_SIZE];
    memset(lines,'\0', BUF_SIZE);
    int i = 0;

    // If file name provide command line with excecutable
    if(a == 2){

        // Read file line by line
        while ((fgets(lines, BUF_SIZE, fs.fp)) != NULL){
            cnt++;

            for(i = 0; i < strlen(lines); i++){

                // Count repeated charactors
                if(lines[i] == lines[i+1]){
                    count++;
                }else{ // Display charactors with count
                    if(count != 0)
                        printf("%d%c", count+1, lines[i]);
                    else
                        printf("%c", lines[i]);

                    count = 0;
                }
            }

            memset(lines,'\0', BUF_SIZE);
            if(cnt == fs.read_line){
                cnt = 0;
                done++;
                // wait up the main thread (if it is sleeping) to test the value of done  
                pthread_cond_signal(&cond); 
                // release lock
                pthread_mutex_unlock(&lock);
                pthread_exit(NULL);
            }
        }
    }
    // If file name with cat command
    else if(a == 1){

        // Read file line by line from standard input
        while ((fgets(lines, BUF_SIZE, stdin)) != NULL){

            cnt++;

            for(i = 0; i < strlen(lines); i++){

                if(lines[i] == lines[i+1]){
                    count++;
                }else{
                    if(count != 0)
                        printf("%d%c", count+1, lines[i]);
                    else
                        printf("%c", lines[i]);

                    count = 0;
                }
            }
            memset(lines,'\0', BUF_SIZE);
        }
    }

    return NULL;
}

// Main entry point code
int main(int argc, char **argv){

    int i = 0;
    pthread_t tid[NUM_THREADS];
    int cnt = 0;
    char lines[512];

    if(argc > 2){
        printf("Incorrect number of arguments\n");
        return 0;
    }

    if(argc == 2){
        fs.filename = argv[1];
        fs.start_index = 0;

        fs.fp = fopen(fs.filename, "r");
        if (fs.fp == NULL)
            return -1;

        // Count the number of lines of file
        while ((fgets(lines, BUF_SIZE, fs.fp)) != NULL){
            cnt++;
        }

        // set file pointer to beginning of file
        rewind(fs.fp);

        // Devide the lines of each thread
        fs.read_line = cnt / NUM_THREADS;

        // Mutex lock initialize
        pthread_mutex_init(&lock, 0);

        // Create threads
        for(i = 0; i < NUM_THREADS; i++){

            pthread_create(&tid[i], NULL, Thread_Handler, (void*)&argc);
        }

        // Are the other threads still busy?
        while(done < NUM_THREADS){

            // Last thread read all remaining lines of file 
            if(done == (NUM_THREADS - 1))
                fs.read_line = cnt - (fs.read_line * (NUM_THREADS-1)); 

            // Block this thread until another thread signals cond. 
            pthread_cond_wait(&cond, &lock); 
        }

        // Waiting for thread exit
        for(i = 0; i < NUM_THREADS; i++){
            pthread_join(tid[i], NULL);
        }

        // close file handle
        fclose(fs.fp);

        // Destroy mutex lock
        pthread_mutex_destroy(&lock);

    }// for cat command
    else if(argc == 1){
        pthread_create(&tid[0], NULL, Thread_Handler, (void*)&argc);

        pthread_join(tid[i], NULL);
    }
    return 0;
}
