#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "sender_receiver.h"

void *BAshared_memory;
void *ABshared_memory;

int main(int argc, char *argv[]){

    FILE *file;
    if(argc > 1){           //Looking for output file for the receiver to print there
        file = fopen(argv[argc - 1], "w");
        if(file == NULL){
            perror("Unable to open the file");
            exit(EXIT_FAILURE);
        }
    }
    else{
        file = NULL;
    }
        /*Gaining access to the shared memory created by the other process for both buffers*/

//The shared memory needed to send messages from A to B
    int fdAB = shm_open("/ABsharedmemory", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(fdAB == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    ABshared_memory = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fdAB, 0);
    if(ABshared_memory == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    User *dataAB;               //Design preference for better user experience
    dataAB = malloc(sizeof(User));
    char username[] = "A : ";   //The receiver will print the name of the other process with the message that it sent
    dataAB->username = malloc(sizeof(char) * (strlen(username) + 1));
    strcpy(dataAB->username, username);
    dataAB->file = file;        //So that the receiver will know where to print the sent messages

    dataAB->info = (SharedData*)ABshared_memory;    //Just gaining access to the already initialized shared memory

//The shared memory needed to send messages from B to A
    int fdBA = shm_open("/BAsharedmemory", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(fdBA == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    BAshared_memory = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fdBA, 0);
    if(BAshared_memory == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    SharedData *dataBA;
    dataBA = (SharedData*)BAshared_memory;  //Just gaining access to the already initialized shared memory

    printf("======================================\n");
    printf("==========type #BYE# to exit==========\n");
    printf("======================================\n\n\n");

    pthread_t sendingThread, receivingThread;

    if(pthread_create(&sendingThread, NULL, send_messages, (void *)dataBA) != 0){       //One thread is sending the messages to the other process
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&receivingThread, NULL, receive_messages, (void *)dataAB) != 0){  //The other one is receiving the other process' messages
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    pthread_join(receivingThread, NULL);    //First we join with receivingThread because we need to see when it finishes
    pthread_cancel(sendingThread);          //so that we can cancel the sendingThread
    pthread_join(sendingThread, NULL);      //to make sure that it isn't stuck on keep reading inputs with the "fgets()"
    dataBA->flag = false;                   //Make sure to end the receivingThread from the other process too
    sem_post(&(dataBA->sender));            //By making flag to false and post the semaphore so that the thread sees the flag and end itself

        /*Printing the stats of this process*/
    printf("\n\n\n");
    printf("======================================\n");
    printf("= Messages sent : %lld \n",dataBA->numofmessage);
    printf("= Messages received : %lld \n",dataAB->info->numofmessage);
    printf("======================================\n");

    printf("\n\n\n");
    printf("======================================\n");
    printf("= Chunks sent : %lld \n",dataBA->numofchunks);
    printf("= Chunks received : %lld \n",dataAB->info->numofchunks);
    printf("======================================\n");

    long double ChunkPerSent = 0;
    long double ChunkPerReceived = 0;

    if(dataBA->numofmessage > 0){           //Make sure that the process has sent massages
        ChunkPerSent = (long double)dataBA->numofchunks / (long double)dataBA->numofmessage;
    }
    if(dataAB->info->numofmessage > 0){     //Make sure that the process has received massages
        ChunkPerReceived = (long double)dataAB->info->numofchunks / (long double)dataAB->info->numofmessage;
    }
    printf("\n\n\n");
    printf("======================================\n");
    printf("= Chunks per message sent : %Lf \n",ChunkPerSent);
    printf("= Chunks per message received : %Lf \n",ChunkPerReceived);
    printf("======================================\n");

    if(dataBA->numofchunks != 0){           //Making sure that the process has sent at least one message
        dataBA->time = dataBA->time / dataBA->numofchunks;
        printf("\n\n\n");
        printf("======================================\n");
        printf("= Average microseconds needed for sent chunk to be printed : %lld \n",dataBA->time);
        printf("======================================\n");
    }
    else{                                   //If not there is no time wasted to be printed
        printf("\n\n\n");
        printf("======================================\n");
        printf("===NO MESSAGE HAVE BEEN SENT WITH B===\n");
        printf("======================================\n");
    }

//Destroy all the semaphores
    sem_destroy(&(dataAB->info->sender));
    sem_destroy(&(dataAB->info->receiver));
    sem_destroy(&(dataBA->sender));
    sem_destroy(&(dataBA->receiver));

//Free the memory space for the username
    free(dataAB->username);

//Unmap the shared memory
    munmap(BAshared_memory, sizeof(SharedData));
    munmap(ABshared_memory, sizeof(SharedData));

//If a file has been opened by a terminal argument 
    free(dataAB);
    if(file != NULL){
        fclose(file);   //Close it
    }

//Close the shared memory that the process had gained access to
    close(fdBA);
    close(fdAB);

    return 0;
}