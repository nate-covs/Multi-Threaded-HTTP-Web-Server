#include "helper.h"
// #include "helper.c"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>
#include <time.h>


#define BUFSIZE 4096

#define MAX_THREAD 24

int sockQueue[256];
int sockCount = 0;

pthread_mutex_t mutexQueue;
pthread_cond_t condQueue;

void* acceptThread() {
    while(1) {
        int sockNumber;

        pthread_mutex_lock(&mutexQueue);
        while(sockCount == 0) {
            pthread_cond_wait(&condQueue, &mutexQueue);
        }

        sockNumber = sockQueue[0];
        for(int i = 0; i < sockCount-1; i++) {
            sockQueue[i] = sockQueue[i+1];
        }
        sockCount--;
        pthread_mutex_unlock(&mutexQueue);

        connection_handler(&sockNumber);
    }
}

void addSock(int clientSock) {
    pthread_mutex_lock(&mutexQueue);
    sockQueue[sockCount] = clientSock;
    sockCount++;
    pthread_mutex_unlock(&mutexQueue);
    pthread_cond_signal(&condQueue);
}

static int server_socket;

void sigint_handler(int sig) {
    printf("Gracefully exiting...\n");
    sleep(3);
    if(server_socket != -1) {
        close(server_socket);
    }
    exit(0);
}

int main(int argc, char **argv) {

    signal(SIGINT, sigint_handler);

    int sockfd; /* socket */
    int portno; /* port to listen on */
    int clientlen; /* byte size of client's address */
    int client_sock;
    int *new_sock;

    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp; /* client host info */

    char buf[BUFSIZE]; /* message buf */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */
    int n; /* message byte size */
    int filePerm; /*used to determine if file exists and permissions*/

    if(argc != 2) {
        fprintf(stderr, "usuage: %s <port>\n", argv[0]);
        exit(1);
    }

    portno = atoi(argv[1]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        error("ERROR opening socket");
    }

    server_socket = sockfd;

    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    bzero((char*) &serveraddr, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) portno);

    if(bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        error("ERROR binding socket");
    }

    clientlen = sizeof(clientaddr);

    if(listen(sockfd, 3) < 0) {
        error("ERROR on listen");
    }

    /*create threads*/
    pthread_t sniffer[MAX_THREAD];
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);
    for(int i = 0; i < MAX_THREAD; i++) {
        pthread_create(&sniffer[i], NULL, &acceptThread, NULL);
    }


    while (1)
    {
        client_sock = accept(sockfd, (struct sockaddr *)&clientaddr, &clientlen);
        if(client_sock < 0) {
            error("accept failed");
        }
        addSock(client_sock);
        //printf("Connected to client\n");
    }



    close(sockfd);
    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);
    return 0;
}

