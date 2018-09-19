//
//  server.h
//  
//
//  Created by Daniil V Kernazhytski on 4/21/18.
//


#ifndef server_h
#define server_h

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>


#define MY_PORT_NUMBER 49999
#define SIZE_STRING 4096


char* exitCommand = "exit\n";
char* lsRead = "ls\n";
char* rlsRead = "rls\n";
char* cdRead = "cd\n";
char* rcdRead = "rcd\n";
char* getRead = "get\n";
char* showRead = "show\n";
char* putRead = "put\n";

void myExit(){
    exit(0);
}

char* getTime(){
    time_t seconds = time(NULL);
    char* timeString =  ctime(&seconds);
    return timeString;
}

//send a message to the client with the given size
void writeToClient(int fileDescriptor,char* message, int size){
    if (write(fileDescriptor, message, size) < 0){
        perror("Did not write to the client");
    }
}

// read from given descriptor, until a new line character 
int readFromClient(int clientFD, char* buf){
    memset(buf,0,SIZE_STRING);
    int n;
    while ( 1 ){
        n = read(clientFD,buf,1);
        if ( n < 0){ 
            perror ("failed reading, source error");
            return -1;
        }
        if ( n == 0) return -1;
        if (*buf == '\n') break;
        buf++;
    }
    strcat(buf, "\0");
    return n;
}

void readFromClientBreakOnEOF(int clientFD, char* buf){
    memset(buf,0,SIZE_STRING);
    int n;
    while ( (n = read(clientFD,buf,1)) ){
        if ( n < 0) perror ("failed reading, source error");
        if ( n == 0) break;
        //printf("%c" , *buf);
        buf++;
    }
    strcat(buf, "\0");
}

void makeSocket(int* socketFD){ //creates socket with the given socket descriptor
    *socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (*socketFD < 0) perror("Socket Error"), exit(1);
}


void bindSocket(int listenFD, struct sockaddr_in* serv, int portNumber){      //bind socket to the portnumber
    memset(serv, 0 , sizeof(* serv));
    serv -> sin_family = AF_INET;
    serv -> sin_port = htons(portNumber);
    serv -> sin_addr.s_addr = htonl(INADDR_ANY);
    if ( bind(listenFD,(struct sockaddr*) serv, sizeof(* serv)) < 0 ){
        perror("Port Binding Error");
        exit(1);
    }
    //printf("success binding \n");
}

void listenSocket(int listenFD){        //establish a connection queue
    if (listen( listenFD , 4) < 0){
        perror("Error listening");
        exit(1);
    }
}

int makeDataConnection(int* socketDescriptor,struct sockaddr_in* serv){
    makeSocket(socketDescriptor);
    bindSocket(*socketDescriptor,serv, 0);
    listenSocket(*socketDescriptor);
    
    int length = sizeof(struct sockaddr_in);
    struct sockaddr_in emptyAddr;
    int newPort = getsockname(*socketDescriptor,(struct sockaddr*) &emptyAddr,(socklen_t*) &length);
    if (newPort < 0 ){
        perror("Getsockname error");
    }
    newPort = ntohs(emptyAddr.sin_port);
    //printf("new data connection port: %d \n" , newPort);
    
    return newPort;     //return the new port number
    
}

//check if the pathName is a directory using lstat AND whether access is available 
int isDirectory(char* pathName){    
    struct stat area, *s = &area;

    if ((stat(pathName, s) == 0) && (access(pathName, R_OK) == 0))
        return S_ISDIR (s->st_mode);

    else 
        return 0;
}

//check if pathName is a regular file 
int isRegularFile(char* pathName){
    struct stat area, *s = &area;

    if ((stat(pathName, s) == 0) && (access(pathName, R_OK) == 0))
       return S_ISREG (s->st_mode);
    
    else
        return 0;
}

int parseArgument(char* buffer,char* command, char* pathName){      //breaks string into two parts
    int result;
    result = sscanf(buffer,"%s %[^\n]" ,command, pathName );
    if (result > 0){
        strcat(command, "\n");
    }
    return result;
}

void parseFileName(char* path, char* name){     //gets a pathname and puts the file name into *name
    int lastIndexFound = 0;
    int i = 0;
    while(path[i]){
        if (path[i] == '/') lastIndexFound = i;
        i++;
    }
    if (lastIndexFound == 0){
        strcpy(name, path);
        return;
     }

    strcpy(name, path+lastIndexFound+1);
    
}

void readFileFromClient(int clientFD, int fileFD){      //get file from Client
    char* buffer = (char*) malloc(sizeof(char) * 128);
    int n;
    while(1){
        n = read(clientFD, buffer, 128);
        if (n == 0) break;
        write(fileFD, buffer, n);
    }
    free(buffer);
}

void removeNewLineCharacter(char* buf){
    int i = 0;
    while(buf[i]){
        if (buf[i] == '\n'){
            buf[i] = '\0';
            break;
        }
        i++;
    }
}


#endif /* server_h */


