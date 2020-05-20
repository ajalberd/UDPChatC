#include <stdio.h>      /*for printf() and fprintf()*/
#include <sys/socket.h> /*for socket(), connect(), send(), and recv()*/
#include <arpa/inet.h>  /*for sockaddr_in and inet_addr()*/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <pthread.h>
#define RCVBUFSIZE 1024
#define NUMCLIENT 3
#define STARTPORT 4444
void DieWithError(char *errorMessage);
/* Error handling function */

struct Client{
    socklen_t clientlen;
    struct sockaddr_in clntAddr;
    char username[RCVBUFSIZE];
};
int num; //tracks number of Clients.
struct thread_args{
    struct Client client;
    int sock;
    int clientNum;
};
struct username_args{
    int portnum;
    char *address; //keep track of clients by their number at first...
};
struct thread_args threadarg1[NUMCLIENT]; 
void *getfromclient(void *args);
void *sendtoclient(void *args);
void *getusername(void *args);
int main(int argc, char *argv[])
{
    int threadct[NUMCLIENT];
    pthread_t threadno[NUMCLIENT]; //NUMCLIENT connections
    char *address = argv[1];
    struct username_args usernameargs[NUMCLIENT];
    
    for(int i=0; i<NUMCLIENT; i++){ //set the 4 ports, for each create a listening thread
    //increment portnum, send portnum and loopback
        usernameargs[i].address = address;
        usernameargs[i].portnum = STARTPORT + i; //port range
        threadct[i] = pthread_create(&threadno[i], NULL, getusername, (void *) &usernameargs[i]); //send this struct to getusername
        if(threadct[i]){
            printf("ERROR CREATING THREAD: CODE %d\n", threadct[i]);
        }
    }
    for(int i=0; i<NUMCLIENT; i++){
        pthread_join(threadno[i], NULL);
    }
    return 0;
}

//Make a function that takes each Client object, and takes in its text input. Send this text input to all other clients, calling another function..
//For each Client object the server needs to take in its text input and send to all clients while sending text to it as well.

void *getusername(void *args){ //get the username first then let it send text.
    struct username_args *info = args;
    int servSock;
    struct sockaddr_in echoServAddr;   
    pthread_t threadnum;
    struct thread_args threadarg1;
    
    servSock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&echoServAddr, '\0', sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_port = htons(info->portnum);
    echoServAddr.sin_addr.s_addr = inet_addr(info->address);  
    
    if(bind(servSock, (struct sockaddr*)&echoServAddr, sizeof(echoServAddr)) !=0){
        DieWithError("Error binding");
    }

    typedef struct Client Client;
    struct Client *client = (Client*)malloc(sizeof(Client));
    client->clientlen = sizeof(struct sockaddr_in);
    while(1){ //Create an instance of the client and give it a username.
        int r;
        //Recieve from one of the clients. Get username and then don't get any connections from them here again.
        if ((r = recvfrom(servSock, client->username, RCVBUFSIZE, 0, (struct sockaddr*)&client->clntAddr, &client->clientlen)) != 0){
            //if there is a comma disregard, only the username without comma should go through.
            printf("Username is: %s\n", client->username);
            //Now create a new thread for that client to send it data.
            threadarg1.client = *client;
            threadarg1.sock = servSock;
            threadarg1.clientNum = num;
            //DO NOT CREATE ANOTHER THREAD!! - ENTER ANOTHER FUNCTION FROM NOW ON TO GET THEIR DATA - 
            //THE USERNAME FOR THIS CLIENT HAS ALREADY BEEN RETRIEVED -put call below:
        }
    }
}

void *getfromclient(void *args){ //Get the text from a single client. Then broadcast to all clients.
    //Each thread should be handling a single client. This way each client instance can call sendtoclient by itself.
    struct thread_args *info = args;
    char recievebuf[RCVBUFSIZE];
    while(1){ 
        if(recvfrom(info->sock, recievebuf, 1024, 0, (struct sockaddr*)&info->client.clntAddr, sizeof(struct sockaddr_in)) < 0){
            printf("data is %s and username is %s\n", recievebuf, info->client.username);
        }
    }
}
void broadcasttoclients(void *args){ //Take in list of clients, synchronize the data. 
    struct thread_args *info = args;
    char recievebuf[RCVBUFSIZE];
    
}