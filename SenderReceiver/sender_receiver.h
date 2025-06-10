#pragma once //#include one time only

#define MESSAGE_SIZE 15             //The size of the message that will passed each time through the two processes

//All the data the two processes need both to know
typedef struct SharedData{
    char message[MESSAGE_SIZE + 1]; //+1 For the Null terminator
    bool flag;                      //Responsible for ending the thread's loops and by extension the threads themselves
    sem_t sender;                   //Tells receiver when the sender is done reading so it can start printing
    sem_t receiver;                 //Tells sender when the receiver is done printing so it can start reading
    unsigned long long numofmessage;    //Counts the messages
    unsigned long long numofchunks;     //Counts the chunks
    long long time;                 //Counts the average time needed to print a chunk
} SharedData;

typedef struct User{                //We use it to know the followings :
    SharedData *info;
    FILE *file;                     //if there is a file so we prints the received messages there
    char *username;                 //and the name of the other process for user's better experience 
} User;

void *send_messages(void *dt);
void *receive_messages(void *dt);