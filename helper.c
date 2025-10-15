#include "helper.h"

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

/*Error defines*/
#define ERROR_400 "%s 400 Bad Request\r\n\r\n" /*The request could not be parsed or is malformed*/
#define ERROR_403 "%s 403 Forbidden\r\n\r\n" /*The requested file can not be accessed due to a file permission issue*/
#define ERROR_404 "%s 404 Not Found\r\n\r\n" /*The requested file can not be found in the document tree*/
#define ERROR_405 "%s 405 Method Not Allowed\r\n\r\n" /*A method other than GET was requested*/
#define ERROR_505 "%s 505 HTTP Version Not Supported\r\n\r\n" /*An HTTP version other than 1.0 or 1.1 was requested*/

/*Valid define*/
#define VALID_200 "%s 200 OK\r\n"

/*Content-type defines*/
#define CONTENT_HTML "Content-Type: text/html\r\n"
#define CONTENT_HTM "Content-Type: text/htm\r\n"
#define CONTENT_TXT "Content-Type: text/plain\r\n"
#define CONTENT_PNG "Content-Type: image/png\r\n"
#define CONTENT_GIF "Content-Type: image/gif\r\n"
#define CONTENT_JPG "Content-Type: image/jpg\r\n"
#define CONTENT_ICO "Content-Type: image/x-icon\r\n"
#define CONTENT_CSS "Content-Type: text/css\r\n"
#define CONTENT_JS "Content-Type: application/javascript\r\n"


/*Content-Length define*/
#define CONTENT_LENGTH "Content-Length: %ld\r\n\r\n"

#define BUFSIZE 4096

void error(char *msg) {
    perror(msg);
    //return(-1);
}

void trim(char *str) {
    size_t length = strlen(str);
    if(length > 0 && str[length-1] == '\n') {
        str[length-1] = '\0';
    }
}

/*Modify to take path from request and see if it exists and check permissions.*/
/*Return -1 if non-existant, return 1 if existant but permission issue and return 0 if it exists and no permission issue*/
int file_search(const char *fileName) {
    char temp[100];
   
    snprintf(temp, sizeof(temp), "./www%s", fileName);

    if(access(temp, F_OK) == 0) {
        if(access(temp, R_OK) == 0) {
            return 0; // exists and can be read
        }
        else {
            return 1; // exists but permission issue
        }
    }

    else {
        return -1; // file doesn't exist
    }
}

long file_length(const char *fileName) {
    char temp[100];
    snprintf(temp, sizeof(temp), "./www%s", fileName);

    FILE *transfer;
    transfer = fopen(temp, "r");
    if(transfer == NULL) {
        error("ERROR opening transfer file");
        return -1;
    }

    fseek(transfer, 0, SEEK_END);
    long transferSize = ftell(transfer); // get the size of file
    fseek(transfer, 0, SEEK_SET);

    fclose(transfer);
    return transferSize;
}

const char *content_type(const char *extension) {
    if(strcmp(extension, "html") == 0) {
        return CONTENT_HTML;
    }
    else if(strcmp(extension, "txt") == 0) {
        return CONTENT_TXT;
    }
    else if(strcmp(extension, "png") == 0) {
        return CONTENT_PNG;
    }
    else if(strcmp(extension, "gif") == 0) {
        return CONTENT_GIF;
    }
    else if(strcmp(extension, "jpg") == 0) {
        return CONTENT_JPG;
    }
    else if(strcmp(extension, "ico") == 0) {
        return CONTENT_ICO;
    }
    else if(strcmp(extension, "css") == 0) {
        return CONTENT_CSS;
    }
    else if(strcmp(extension, "js") == 0) {
        return CONTENT_JS;
    }

}

void *connection_handler(void *socket_desc) {
    int n;
    char buf[BUFSIZE];
    int client_sock;
    int filePerm;

    client_sock = *(int*)socket_desc;

    n = recv(client_sock, buf, BUFSIZE - 1, 0);
        if(n < 0) {
            error("ERROR in recv");
            return (void *)-1;
        }

        /*Variables for extracting request information*/
        char reqMethod[10];
        char reqURI[100];
        char reqVersion[10];
        char reqContentType[5];

        /*Set variables with sscanf*/
        int interpret = sscanf(buf, "%s %s %s", reqMethod, reqURI, reqVersion);

        /*Log to check*/
        printf("Request method: %s\n", reqMethod);
        printf("Request URI: %s\n", reqURI);
        printf("Request Version: %s\n", reqVersion);

        /*Check if the request is a get method*/
        if(strncmp(reqMethod, "GET", 3) != 0) {
            memset(buf, 0, BUFSIZE);
            snprintf(buf, BUFSIZE, ERROR_405, reqVersion);
            n = send(client_sock, buf, n, 0);
            if(n < 0) {
                error("ERROR on sending data GET");
                return (void *)-1;
            }
            close(client_sock);
            return (void *)-1;
        }

        /*Check if the request is using one of the valid HTTP protocols*/
        if((strncmp(reqVersion, "HTTP/1.0", 8) != 0) && (strncmp(reqVersion, "HTTP/1.1", 8) != 0)) {
            memset(buf, 0, BUFSIZE);
            snprintf(buf, BUFSIZE, ERROR_505, reqVersion);
            n = send(client_sock, buf, n, 0);
            if(n < 0) {
                error("ERROR on sending data HTTP");
                return (void *)-1;
            }
            close(client_sock);
            return (void *)-1;
        }

        /*Check for file existance and permission*/
        filePerm = file_search(reqURI);

        if(filePerm == -1) { //missing file
            memset(buf, 0, BUFSIZE);
            snprintf(buf, BUFSIZE, ERROR_404, reqVersion);
            n = send(client_sock, buf, n, 0);
            if(n < 0) {
                error("ERROR on sending data MISSING FILE");
                return (void *)-1;
            }
            close(client_sock);
            return (void *)-1;
        } 

        if(filePerm == 1) { //permission issue
            memset(buf, 0, BUFSIZE);
            snprintf(buf, BUFSIZE, ERROR_403, reqVersion);
            n = send(client_sock, buf, n, 0);
            if(n < 0) {
                error("ERROR on sending data PERMISSION ISSUE");
                return (void *)-1;
            }
            close(client_sock);
            return (void *)-1;
        }

        /*Check for bad request, i.e., a field left blank*/
        if(strncmp(reqMethod, "GET", 3) != 0 || reqURI[0] != '/' || ((strncmp(reqVersion, "HTTP/1.0", 8) != 0) 
        && (strncmp(reqVersion, "HTTP/1.1", 8) != 0))) {
            memset(buf, 0, BUFSIZE);
            snprintf(buf, BUFSIZE, ERROR_400, reqVersion);
            n = send(client_sock, buf, n, 0);
            if(n < 0) {
                error("ERROR on sending data 400");
                return (void *)-1;
            }
            close(client_sock);
            return (void *)-1;
        }

         /*Extract file extension and trim last character to null*/
         char *extension;
         if(reqURI[strlen(reqURI)-1] != '/') {
            extension = strrchr(reqURI, '.');
            strncpy(reqContentType, extension+1, sizeof(reqContentType)-1);
            trim(reqContentType);
         }
         const char *content_type_extension_message = content_type(reqContentType);
         long fileLength;

         if(reqURI[strlen(reqURI)-1] == '/') {
            char reqURICheckHTML[200];
            char reqURICheckHTM[200];
            snprintf(reqURICheckHTML, sizeof(reqURICheckHTML), "./www%sindex.html", reqURI);
            snprintf(reqURICheckHTM, sizeof(reqURICheckHTM), "./www%sindex.htm", reqURI);
            if((fopen(reqURICheckHTML, "r") == NULL)&&(fopen(reqURICheckHTML, "r") == NULL)) {
                memset(buf, 0, BUFSIZE);
                snprintf(buf, BUFSIZE, ERROR_404, reqVersion);
                n = send(client_sock, buf, n, 0);
                if(n < 0) {
                    error("ERROR on sending data MISSING FILE");
                    return (void *)-1;
                }
                close(client_sock);
                return (void *)-1;
            }
            if(fopen(reqURICheckHTML, "r") != NULL) {
                content_type_extension_message = CONTENT_HTML;
                char fileLengthHTMLcheck[200];
                snprintf(fileLengthHTMLcheck, sizeof(fileLengthHTMLcheck), "%sindex.html", reqURI);
                fileLength = file_length(fileLengthHTMLcheck);
            }
            if(fopen(reqURICheckHTM, "r") != NULL) {
                content_type_extension_message = CONTENT_HTM;
                char fileLengthHTMcheck[200];
                snprintf(fileLengthHTMcheck, sizeof(fileLengthHTMcheck), "%sindex.htm", reqURI);
                fileLength = file_length(fileLengthHTMcheck);
            }
            
         }

         if(reqURI[strlen(reqURI)-1] != '/') {
            fileLength = file_length(reqURI);
         }
        

        char responseHeader[BUFSIZE];
        memset(responseHeader, 0, BUFSIZE);

        /*Send valid header, content type and size*/
        snprintf(responseHeader, BUFSIZE, VALID_200, reqVersion);
        strncat(responseHeader, content_type_extension_message, BUFSIZE - strlen(responseHeader) - 1);

        char tempFileLength[100];
        snprintf(tempFileLength, 100, CONTENT_LENGTH, fileLength);
        strncat(responseHeader, tempFileLength, BUFSIZE - strlen(responseHeader) - 1);

        memset(buf, 0, BUFSIZE);

        snprintf(buf, BUFSIZE, "%s", responseHeader);
        n = send(client_sock, buf, strlen(buf), 0);
        if(n < 0) {
            error("ERROR on sending data ONE PACKET");
            return (void *)-1;
        }

        FILE *transfer;
        char tempFile[200];

        if(reqURI[strlen(reqURI)-1] == '/') {
            transfer = fopen("./www/index.html", "r");
        }

        else{
            snprintf(tempFile, sizeof(tempFile), "./www%s", reqURI); 
            transfer = fopen(tempFile, "r");
        }

        memset(buf, 0, BUFSIZE);

        if(fileLength < (BUFSIZE - strlen(buf))) { // can send in one packet

            size_t readFile = fread(buf, sizeof(char), fileLength, transfer);
            if(readFile != fileLength) {
                error("ERROR reading file");
                return (void *)-1;
            }

            n = send(client_sock, buf, BUFSIZE, 0);
            if(n < 0) {
                error("ERROR on sending data ONE PACKET");
                return (void *)-1;
            }

            fclose(transfer);
            //free(socket_desc);
            close(client_sock);
        }
    
        else { // too big, must send in multiple, FIX/WORK ON THIS AND HOW TO SEND MULTIKPLE PACKETS, HEADER EACH TIME??? 
            size_t sent = 0;
            
            while(sent < fileLength) {
                
                size_t chunk = fread(buf, sizeof(char), BUFSIZE, transfer);
                n = send(client_sock, buf, chunk, 0); 
                if(n < 0) {
                    error("ERROR sending chunk");
                    return (void *)-1;
                }
                sent += chunk;
                memset(buf, 0, BUFSIZE);
            }
            //printf("busted out\n");
            fclose(transfer);
            //free(socket_desc);
            close(client_sock);
        }
        //return 0;
}


