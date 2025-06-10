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

void *ABshared_memory;
void *BAshared_memory;

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

            /*Making the shared memory for both buffers*/

//The shared memory needed to send messages from A to B
    int fdAB = shm_open("/ABsharedmemory", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(fdAB == -1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    ftruncate(fdAB, sizeof(SharedData));

    ABshared_memory = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fdAB, 0);
    if(ABshared_memory == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    SharedData *dataAB;
    dataAB = (SharedData*)ABshared_memory;

//Variables initialization to carry messages from A to B

    dataAB->flag = true;
    if(sem_init(&(dataAB->sender), 1, 0) == -1){
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    if(sem_init(&(dataAB->receiver), 1, 1) == -1){      //So that send_messages() will start accepting inputs
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    dataAB->numofmessage = 0;
    dataAB->numofchunks = 0;
    dataAB->time = 0;

//The shared memory needed to send messages from B to A
    int fdBA = shm_open("/BAsharedmemory", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fdBA == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    ftruncate(fdBA, sizeof(SharedData));

    BAshared_memory = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fdBA, 0);
    if(BAshared_memory == MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }

//Variables initialization to carry messages from Β to Α

    User *dataBA;               //Design preference for better user experience
    dataBA = malloc(sizeof(User));
    char username[] = "B : ";   //The receiver will print the name of the other process with the message that it sent
    dataBA->username = malloc(sizeof(char) * (strlen(username) + 1));
    strcpy(dataBA->username, username);

    dataBA->info = (SharedData*)BAshared_memory;
    dataBA->info->flag = true;
    if(sem_init(&(dataBA->info->sender), 1, 0) == -1){
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    if(sem_init(&(dataBA->info->receiver), 1, 1) == -1){    //So that send_messages() will start accepting inputs
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    dataBA->info->numofmessage = 0;
    dataBA->info->numofchunks = 0;
    dataBA->info->time = 0;
    dataBA->file = file;                        //So that the receiver will know where to print the sent messages

    printf("======================================\n");
    printf("==========type #BYE# to exit==========\n");
    printf("======================================\n\n\n");

    pthread_t sendingThread, receivingThread;

    if(pthread_create(&sendingThread, NULL, send_messages, (void *)dataAB) != 0){       //One thread is sending the messages to the other process
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if(pthread_create(&receivingThread, NULL, receive_messages, (void *)dataBA) != 0){  //The other one is receiving the other process' messages
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    pthread_join(receivingThread, NULL);    //First we join with receivingThread because we need to see when it finishes
    pthread_cancel(sendingThread);          //so that we can cancel the sendingThread 
    pthread_join(sendingThread, NULL);      //to make sure that it isn't stuck on keep reading inputs with the "fgets()"
    dataAB->flag = false;                   //Make sure to end the receivingThread from the other process too
    sem_post(&(dataAB->sender));            //By making flag to false and post the semaphore so that the thread sees the flag and end itself

        /*Printing the stats of this process*/
    printf("\n\n\n");
    printf("======================================\n");
    printf("= Messages sent : %lld \n",dataAB->numofmessage);
    printf("= Messages received : %lld \n",dataBA->info->numofmessage);
    printf("======================================\n");

    printf("\n\n\n");
    printf("======================================\n");
    printf("= Chunks sent : %lld \n",dataAB->numofchunks);
    printf("= Chunks received : %lld \n",dataBA->info->numofchunks);
    printf("======================================\n");

    long double ChunkPerSent = 0;
    long double ChunkPerReceived = 0;

    if( dataAB->numofmessage > 0 ){         //Make sure that the process has sent massages
        ChunkPerSent = (long double)dataAB->numofchunks / (long double)dataAB->numofmessage;
    }
    if(dataBA->info->numofmessage > 0){     //Make sure that the process has received massages
        ChunkPerReceived = (long double)dataBA->info->numofchunks / (long double)dataBA->info->numofmessage;
    }
    printf("\n\n\n");
    printf("======================================\n");
    printf("= Chunks per message sent : %Lf \n",ChunkPerSent);
    printf("= Chunks per message received : %Lf \n",ChunkPerReceived);
    printf("======================================\n");

    if(dataAB->numofchunks != 0){           //Making sure that the process has sent at least one message
        dataAB->time = dataAB->time / dataAB->numofchunks;
        printf("\n\n\n");
        printf("======================================\n");
        printf("= Average microseconds needed for sent chunk to be printed : %lld \n",dataAB->time);
        printf("======================================\n");
    }
    else{                                   //If not there is no time wasted to be printed
        printf("\n\n\n");
        printf("======================================\n");
        printf("===NO MESSAGE HAVE BEEN SENT WITH A===\n");
        printf("======================================\n");
    }

//Destroy all the semaphores
    sem_destroy(&(dataAB->sender));
    sem_destroy(&(dataAB->receiver));
    sem_destroy(&(dataBA->info->sender));
    sem_destroy(&(dataBA->info->receiver));

//Free the memory space for the username
    free(dataBA->username);

//Unmap the shared memory
    munmap(ABshared_memory, sizeof(SharedData));
    munmap(BAshared_memory, sizeof(SharedData));

//Free the memory for the User type variable
    free(dataBA);

//If a file has been opened by a terminal argument 
    if(file != NULL){
        fclose(file);   //Close it
    }

//Close and Unlink all shared memory
    close(fdAB);
    close(fdBA);
    shm_unlink("/ABsharedmemory");
    shm_unlink("/BAsharedmemory");

    return 0;
}