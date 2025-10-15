#ifndef HELPER_H
#define HELPER_H

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
#include <signal.h>
#include <pthread.h>

/*Error printing function*/
void error(char *msg);

/*Function to trim newline characters if needed*/
void trim(char *str);

/*Search www directory for files in request*/
int file_search(const char *fileName); 

/*Find length of a file*/
long file_length(const char *fileName);

/*Return content type def based on extracted extension*/
const char *content_type(const char *extension);

/*Thread function*/
void *connection_handler(void *socket_desc);

/**/
#endif