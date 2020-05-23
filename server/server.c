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
struct thread_args{ //Client, clientNum, and username
    struct Client client;
    int sock;
    int clientNum;
    char *username;
};
struct username_args{ //One for each thread. Stores the port number (and client number) as well as the address to bind.
    int portnum;
    char *address; //keep track of clients by their number at first...
};
struct thread_args threadarg1[NUMCLIENT]; 
void getfromclient(struct thread_args *threadarg1);
void broadcasttoclients(struct thread_args *threadarg1, char *recievebuf);
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
    //pthread_t threadnum;
    
    servSock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&echoServAddr, '\0', sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_port = htons(info->portnum);
    echoServAddr.sin_addr.s_addr = inet_addr(info->address);  
    
    if(bind(servSock, (struct sockaddr*)&echoServAddr, sizeof(echoServAddr)) !=0){
        DieWithError("Error binding");
    }
    int clientnum = info->portnum-STARTPORT;
    typedef struct Client Client;
    struct Client *client = (Client*)malloc(sizeof(Client));
    client->clientlen = sizeof(struct sockaddr_in);
    while(1){ //Create an instance of the client and give it a username.
        int r;
        //Recieve from one of the clients. Get username and then don't get any connections from them here again.
        if ((r = recvfrom(servSock, client->username, RCVBUFSIZE, 0, (struct sockaddr*)&client->clntAddr, &client->clientlen)) != 0){
            printf("Username is: %s\n", client->username);
            //Now create a new thread for that client to send it data.
            threadarg1[clientnum].client = *client;
            threadarg1[clientnum].sock = servSock;
            threadarg1[clientnum].clientNum = clientnum;
            threadarg1[clientnum].username = client->username;
            //verified working


            //Now, broadcast to this client all usernames in play. i.e "FROM SERVER: USERNAMES X X X X"
            //Assume andy2 is created (and owns this thread) after andy is made. Send to andy2 that andy is in the chat.
            for(int x=0; x< NUMCLIENT; x++){
                if(threadarg1[x].username && (x!=clientnum)){
                    sendto(servSock, threadarg1[x].client.username, RCVBUFSIZE, 0, (struct sockaddr*)&threadarg1[clientnum].client.clntAddr, sizeof(threadarg1[clientnum].client.clntAddr));
                    sendto(servSock, "Server: ", RCVBUFSIZE, 0, (struct sockaddr*)&threadarg1[clientnum].client.clntAddr, sizeof(threadarg1[clientnum].client.clntAddr));
                }
            }

            //DO NOT CREATE ANOTHER THREAD!! - ENTER ANOTHER FUNCTION FROM NOW ON TO GET THEIR DATA - 
            //THE USERNAME FOR THIS CLIENT HAS ALREADY BEEN RETRIEVED -put call below:
            getfromclient(&threadarg1[clientnum]);
            
        }
    }
}

void getfromclient(struct thread_args *threadarg){ //Get the text from a single client. Then broadcast to all clients.
    //Each thread should be handling a single client. This way each client instance can call sendtoclient by itself.
    //struct thread_args *threadarg1 = threadarg1;
    char recievebuf[RCVBUFSIZE];
    char *s;
    while(1){ //Each client should be connecting to a different sock.
        if(recvfrom(threadarg->sock, recievebuf, RCVBUFSIZE, 0, (struct sockaddr*)&threadarg->client.clntAddr, sizeof(struct sockaddr_in)) < 0){
            if ((s = strstr(recievebuf, ","))){
                //Username,data to send to a specific username. Do not call the broadcasttoclient 
                char *usern = strtok(recievebuf, ",");
                for(int i=0; i<NUMCLIENT;i++){
                    if((threadarg1[i].username) && !(strcmp(threadarg1[i].username, usern))){
                        usern = strtok(NULL, ",");
                        char databuf[RCVBUFSIZE];
                        strcpy(databuf, usern);
                        sendto(threadarg1[i].sock, databuf, RCVBUFSIZE, MSG_CONFIRM, (struct sockaddr*)&threadarg1[i].client.clntAddr, sizeof(threadarg1[i].client.clntAddr)); //using sendto() for UDP messages.
                        sendto(threadarg1[i].sock, threadarg->username, RCVBUFSIZE, MSG_CONFIRM, (struct sockaddr*)&threadarg1[i].client.clntAddr, sizeof(threadarg1[i].client.clntAddr)); //using sendto() for UDP messages.
                        break;
                    }
                }
            }
            else{
                printf("data is %s and username is %s\n", recievebuf, threadarg1->client.username);
                //Now send to every client except your own.            
                broadcasttoclients(threadarg, recievebuf);
            } 
        }
    }
}
void broadcasttoclients(struct thread_args *threadarg, char *recievebuf){ //Take in list of clients, synchronize the data. 
    struct thread_args *info = threadarg;
    for(int i=0; i<NUMCLIENT; i++){
        if(info->clientNum != i){ //send to all but the passed threadarg client.
            //Send both the username and text.
            char username[RCVBUFSIZE];
            strcpy(username, info->username);
            sendto(threadarg1[i].sock, recievebuf, RCVBUFSIZE, MSG_CONFIRM, (struct sockaddr*)&threadarg1[i].client.clntAddr, sizeof(threadarg1[i].client.clntAddr)); //using sendto() for UDP messages.
            sendto(threadarg1[i].sock, username, RCVBUFSIZE, MSG_CONFIRM, (struct sockaddr*)&threadarg1[i].client.clntAddr, sizeof(threadarg1[i].client.clntAddr)); //using sendto() for UDP messages.
        }
    }
}