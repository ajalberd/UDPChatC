#include <stdio.h> /*for printf() and fprintf()*/
#include <sys/socket.h> /*for socket(), connect(), send(), and recv()*/
#include <arpa/inet.h> /*for sockaddr_in and inet_addr()*/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

#define RCVBUFSIZE 1024 /*Size of receive recievebuf*/

void DieWithError(char *errorMessage);
struct thread_args{
    struct sockaddr_in server_addr;
    int sock;
    char *username;
};
void *recvfromserver(void *args);
void *sendtoserver(void *args);
struct thread_args threadarg1;
int main(int argc, char *argv[]) 
{
    int port = atoi(argv[1]); //port number first
    char *address = argv[2];
    char *username = argv[3];

    int sock; //the socket descriptor used to initilize socket()
    struct sockaddr_in server_addr; //sockaddr_in includes sin_port and sin_addr, choose that one.
    
    //Taken from the included TCP Programming PDF
    //1. Create a TCP socket using socket()
    if((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0){ //sockdgram allows for UDP
        DieWithError("Error creating the socket()");
    } 
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET; //the chosen address family
    server_addr.sin_port = htons(port); //Server port
    server_addr.sin_addr.s_addr = inet_addr(address);
    

    //Send the string to the server
    char buffer[RCVBUFSIZE];
    strcpy(buffer, username);
    int s = sendto(sock, username, RCVBUFSIZE, MSG_CONFIRM, (struct sockaddr*)&server_addr, sizeof(server_addr)); //using sendto() for UDP messages.
    printf("Sent %d bytes\n",s);
    
    //Now that the username is sent to the server, then you can start sending text.
    pthread_t thread1, thread2;
    int t1,t2;
    threadarg1.server_addr = server_addr;
    threadarg1.sock = sock;
    threadarg1.username = username;
    t1 = pthread_create(&thread1, NULL, recvfromserver, (void *) &threadarg1);
    t2 = pthread_create(&thread2, NULL, sendtoserver, (void *) &threadarg1);
    pthread_join( thread1, NULL);
    pthread_join( thread2, NULL); 
    return 0;
} 

void *recvfromserver(void *args){
    struct thread_args *info = args;
    char recievebuf[RCVBUFSIZE]; //get text from server later
    while(1){
        recievebuf[0] = '\0'; //blank the recieve every time
        recvfrom(info->sock, recievebuf, 1024, 0, (struct sockaddr*)&info->server_addr, sizeof(info->server_addr));
        if(recievebuf[0] != '\0'){
            printf("%s\n",recievebuf);
        }
    }
}
void *sendtoserver(void *args){
    struct thread_args *info = args;
    char sendbuf[RCVBUFSIZE];
    while(1){
        printf("input text:\n");
        fgets(sendbuf,RCVBUFSIZE,stdin);
        sendbuf[strlen(sendbuf) - 1] = 0;
        if(sendto(info->sock, sendbuf, RCVBUFSIZE, MSG_CONFIRM, (struct sockaddr*)&info->server_addr, sizeof(info->server_addr)) != sizeof(sendbuf)){
            DieWithError("Send failed\n");
        }
    }
}