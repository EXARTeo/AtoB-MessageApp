#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include "sender_receiver.h"
#include <sys/time.h>

void *send_messages(void *dt){
    SharedData *data = (SharedData*)dt;     //Gaining access to the shared memory
    struct timeval start;

    while(data->flag){
        //If it is the first time in this fuction for this thread just proceed to read input
        //if it is not it is waiting for the receiver to print the message
        sem_wait(&(data->receiver));
        if( fgets(data->message, MESSAGE_SIZE + 1, stdin) == NULL){     //+1 so that we can read 15 characters (+1 for NULL terminator)
            //If we are reading from a file, when we finished reading (AKA we read EOF)
            //or the user don't want to give other message (AKA ^D == EOF)
            strcpy(data->message, "#BYE#"); //It means that we have to end the process
        }
        gettimeofday(&start, NULL);         //After we read we get the current time
        if( ((strcmp(data->message , "#BYE#\n") == 0) || (strcmp(data->message , "#BYE#") == 0)) ){
            //If we read #BYE# (or EOF so we put #BYE# to the buffer) we are ending the loops and proceed to end the threads
            data->flag = false;
        }
        else{
            data->time -= start.tv_usec;    //We subtruct the current time only if we read a message to print
            data->numofchunks++;            //For every chunk of message sent +1 to the counter
        }
        sem_post(&(data->sender));          //Telling the receiver to start printing 
    }
    return NULL;
}

void *receive_messages(void *dt){

    User *user = (User*)dt;
    SharedData *data = user->info;  //Gaining access to the shared memory
    struct timeval end;

    bool printFlag = true;          //Needed to know when to print the username of the other process

    while(data->flag){
        //Waits for the sender to send a message to print
        sem_wait(&(data->sender));
        //If it received #BYE# it means no more actions just end the loop and the threads!
        if( (strcmp(data->message , "#BYE#\n") != 0) && (strcmp(data->message , "#BYE#") != 0) && data->flag ){
            if(user->file == NULL){                         //If there is no file to print in just print them in stdin
                if(printFlag){
                    printf("%s",user->username);
                }
                printf("%s",data->message);
            }
            else{                                           //Make sure if there is a file to print
                if(printFlag){                              //the username
                    fprintf(user->file, "%s",user->username);
                }
                fprintf(user->file, "%s",data->message);    //and the messages there
            }
            gettimeofday(&end, NULL);   //Right after printing we getting the current time 
            data->time += end.tv_usec;  //and add it to the time so that we actually adding the distance (end - start)

            //Now we are making sure if the chunk we received is part of a bigger message
            if( (strlen(data->message) < MESSAGE_SIZE) || (data->message[14] == '\n') ){
            //If it is not we count it as a message
            //or if it is BUT it is the last chunk we count it as a message too!

                printFlag = true;
                data->numofmessage++;
            }
            else{
            //If it is and it is not the last, we don't have to print the username again and we are just printing the chunks left
                printFlag = false;
            }
        }
        //Tell the sender to read more inputs
        sem_post(&(data->receiver));
    }
    return NULL;
}