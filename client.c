//  CS 360
//  Final Project 
//  Client Code 
//
//  Created by Daniil V Kernazhytski on 4/11/18.
//

#include "server.h"

struct sockaddr_in servAddr;
struct sockaddr_in servAddrData;
struct hostent* hostEntry;
struct in_addr** pptr;
int DEBUG = 0;

int socketFD;
int dataPortNumber;
int dataConnectFD;

int readPortNumberFromServer(char* buf){
    strcpy(buf , "D\n");
    writeToClient(socketFD,buf, strlen(buf) * sizeof(char));
    if ( (readFromClient(socketFD, buf)) < 0 ){
        perror("Server Connection Lost. Shutting Down");
        myExit();
    }
    if (DEBUG) printf ("received port number from, server: %s" , buf);

    dataPortNumber = atoi(buf+1);
    return dataPortNumber;
}

void makeDataConnectionFromClient(){
    if (DEBUG) printf("port number is :%d \n" , dataPortNumber);

    makeSocket(&dataConnectFD);
    memset( &servAddrData, 0 , sizeof(servAddr));
    servAddrData.sin_family = AF_INET;
    servAddrData.sin_port = htons(dataPortNumber);
    
    if (hostEntry){
        pptr = (struct in_addr**) hostEntry ->h_addr_list;
        memcpy(&servAddrData.sin_addr, *pptr, sizeof(struct in_addr));
    }
    else{
        perror(" Error get host by name ");
        exit(1);
    }
    int connectStatus = connect(dataConnectFD, (struct sockaddr*)&servAddrData, sizeof(servAddr));
    if (connectStatus < 0) perror("Connection Error"), exit(1);

    if (DEBUG) printf("connection success :%d \n" , connectStatus);
}

int main(int argc, char** argv){
    int numberOfArguments;

    if (argc != 2){
        printf("Wrong Number of Arguments \n");
        exit(1);
    }
    
    
    
    makeSocket(&socketFD);
    makeSocket(&dataConnectFD);
    
    memset( &servAddr, 0 , sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(MY_PORT_NUMBER);
    
    hostEntry = gethostbyname(argv[1]);
    if (hostEntry){
        pptr = (struct in_addr**) hostEntry ->h_addr_list;
        memcpy(&servAddr.sin_addr, *pptr, sizeof(struct in_addr));
    }
    else{
        perror(" Error get host by name ");
        exit(1);
    }
    int connectStatus = connect(socketFD, (struct sockaddr*)&servAddr, sizeof(servAddr));
    if (connectStatus < 0) perror("Connection Error"), exit(1);
    
    char* writeBuffer = malloc(sizeof(char) * SIZE_STRING);
    char* readBuffer = malloc(sizeof(char) * SIZE_STRING);
    char* pathName = malloc(sizeof(char) * SIZE_STRING);
    char* command = malloc(sizeof(char) * SIZE_STRING);
    char* fileName = malloc(sizeof(char) * 100);

    while(1){
        printf(">>");
        fgets(writeBuffer, SIZE_STRING , stdin);        //get command from User 
        numberOfArguments = parseArgument(writeBuffer, command, pathName);      //if 0 commands Error
        if (numberOfArguments < 1){
            perror("invalid number of arguments ");
            continue;
        }

        if (DEBUG) printf("writeBuffer: %s" , writeBuffer);
        if (DEBUG) printf("command: %s" , command);
        if (DEBUG) printf("pathName: %s\n" , pathName);

        if (strcmp(command,exitCommand) == 0){      // exit command
            if (DEBUG) printf("About to tell server to die \n");
            strcpy(writeBuffer, "Q\n");
            writeToClient(socketFD,writeBuffer , strlen(writeBuffer) * sizeof(char));
            readFromClient(socketFD , readBuffer);          //read Aknowledgement
            if ( readBuffer[0]  == 'E'){
                printf("Error : %s" , readBuffer+1);
            } 
            free(writeBuffer);
            free(readBuffer);
            free(pathName);
            free(command);
            free(fileName);
            close(socketFD);
            printf("Client Exiting \n");
            myExit();
        }

        else if (strcmp(command, cdRead) == 0){     // local change directory 
            if (DEBUG) printf ("inside local change directory\n");
             if (numberOfArguments != 2){
                perror("can't change directory, no directory specified");
                continue;
            }
            
            if (isDirectory(pathName) == 0){
                perror("Not a valid directory path ");
            }
            else{
                chdir(pathName);
                printf("Changed local directory \n");
            }

            
        }

        else if (strcmp(command, rcdRead) == 0){        //remote change directory
            if (DEBUG) printf ("inside remote change directory\n");
            if (numberOfArguments != 2){
                perror("can't change directory, no directory specified");
                continue;
            }
            memset(writeBuffer, 0 , SIZE_STRING);
            strcpy(writeBuffer, "C");
            strcat(writeBuffer, pathName);
            strcat(writeBuffer, "\n");
            if (DEBUG) printf("have command: %s \n" , writeBuffer);
            writeToClient(socketFD, writeBuffer, strlen(writeBuffer) * sizeof(char));

            readFromClient(socketFD , readBuffer);          //read aknowledgement 
            if (readBuffer[0] == 'E'){
                perror("change directory failed");
                printf("%s\n" , readBuffer+1);
                continue;
            }

            printf("Finished remote change directory \n");
        }
        
        else if (strcmp(command,lsRead) == 0){         // local ls
            if (DEBUG) printf ("about to do ls\n");
            if (fork()){    //parent
                wait(NULL);
                printf("finished local ls \n");
            }
            else{           //child
                int pipeFD[2];
                pipe (pipeFD);
                int reader, writer;
                reader = pipeFD[0];
                writer = pipeFD[1];

                if(fork()){     //parent 2
                    close(writer);
                    dup2(reader, 0);
                    execlp("more", "more", "-20", NULL);
                    perror("more failed ");
                }
                else{           //child 2
                    close(reader);
                    dup2(writer, 1);
                    execlp("ls", "ls", "-l", NULL);
                    perror("ls failed ");
                }
            
            }
        }
        
        else if (strcmp(command,rlsRead) == 0){         //remote ls
            if (DEBUG) printf ("about to do rls\n");
            if (DEBUG) printf ("about to establish data connection\n");
            if (readPortNumberFromServer(command) == 0){
                printf("failed getting a port number \n");
                continue;
            }
                
            makeDataConnectionFromClient();
           
            strcpy(command, "L\n");
            writeToClient(socketFD,command, strlen(command) * sizeof(char));
            if (DEBUG) printf ("about to read from data coonection \n");
            //read aknowledgement 
            readFromClient(socketFD, readBuffer);
            if (DEBUG) printf("server aknowledges: %s \n" , readBuffer);
            if (readBuffer[0] == 'E'){              //check if positive aknowledgement
                perror("server returned error");
                printf("%s \n" , readBuffer+1);
            }

            if (fork()){    //parent
                close(dataConnectFD);
                wait(NULL);
                printf("finished remote ls \n");
                
            }
            else{           //child
                    dup2(dataConnectFD, 0);         //read from the socket 
                    execlp("more", "more", "-20", NULL);
                    perror("more failed ");
                       
            }
            
        }

        else if (strcmp(command,getRead) == 0){         // get file from server
            if (DEBUG) printf("entered get <pathname> \n ");
            parseFileName(pathName, fileName);
            if (DEBUG) printf("path name: %s  fileName: %s \n" , pathName, fileName);
            
            if (readPortNumberFromServer(command) == 0){
                perror("failed getting a port number \n");
                continue;
            }
            makeDataConnectionFromClient();

            memset(writeBuffer, 0 , SIZE_STRING);
            strcpy(writeBuffer, "G");
            strcat(writeBuffer, pathName);
            strcat(writeBuffer, "\n");
            if (DEBUG) printf("have command: %s \n" , writeBuffer);
            writeToClient(socketFD, writeBuffer, strlen(writeBuffer) * sizeof(char));


            readFromClient(socketFD , readBuffer);          //read aknowledgement 
            if (DEBUG) printf("server aknowledges: %s \n" , readBuffer);

            if (readBuffer[0] == 'E'){
                printf("%s\n" , readBuffer+1);
                close(dataConnectFD);
                continue;
            }


            int fileDescriptor = open(fileName, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0666);       //check if file can be opened for writing
            if (fileDescriptor < 0 ){
                perror("could not create file locally");
                close(dataConnectFD);
                close(fileDescriptor);
                continue; 
            }

           

            readFileFromClient(dataConnectFD, fileDescriptor);
            close(dataConnectFD);
            close(fileDescriptor);

            printf("finished getting file from the server \n");

        }

        else if (strcmp(command,showRead) == 0 ){       //show file from the server 
            if (DEBUG) printf("entered show <pathname> \n ");
            parseFileName(pathName, fileName);
            if (DEBUG) printf("path name: %s  fileName: %s \n" , pathName, fileName);

            if (readPortNumberFromServer(command) == 0){
                printf("failed getting a port number \n");
                continue;
            }
            makeDataConnectionFromClient();

            memset(writeBuffer, 0 , SIZE_STRING);
            strcpy(writeBuffer, "G");
            strcat(writeBuffer, pathName);
            strcat(writeBuffer, "\n");
            if (DEBUG) printf("have command: %s \n" , writeBuffer);
            writeToClient(socketFD, writeBuffer, strlen(writeBuffer) * sizeof(char));


            readFromClient(socketFD , readBuffer);          //read aknowledgement 
            if (DEBUG) printf("server aknowledges: %s \n" , readBuffer);

            if (readBuffer[0] == 'E'){
                printf("%s\n" , readBuffer+1);
                close(dataConnectFD);
                continue;
            }

            if (fork()){
                close(dataConnectFD);
                wait(NULL);
            }
            else{
                    dup2(dataConnectFD, 0);
                    execlp("more" , "more" , "-20", NULL);
                    perror("execlp more failed ");
            }
            
            close(dataConnectFD);
            printf("finished showing file from the server \n");
        }

        else if (strcmp(command, putRead) == 0){        //put a file on the server
            if (DEBUG) printf("entered put <pathname> \n ");
            parseFileName(pathName, fileName);
            if (DEBUG) printf("path name: %s  fileName: %s \n" , pathName, fileName);
            if( isRegularFile(pathName) == 0){
                perror("the pathname passed is not a regular file");
                continue;
            }

            int fileDescriptor = open(fileName, O_RDONLY);
            if (fileDescriptor < 0 ){
                perror("could not open a file for reading");
                continue; 
            }

            if (readPortNumberFromServer(command) == 0){
                printf("failed getting a port number \n");
                continue;
            }
            makeDataConnectionFromClient();

            memset(writeBuffer, 0 , SIZE_STRING);
            strcpy(writeBuffer, "P");
            strcat(writeBuffer, pathName);
            strcat(writeBuffer, "\n");
            if (DEBUG) printf("have command: %s \n" , writeBuffer);
            writeToClient(socketFD, writeBuffer, strlen(writeBuffer) * sizeof(char));


            readFromClient(socketFD , readBuffer);          //read aknowledgement 
            if (DEBUG) printf("server aknowledges: %s \n" , readBuffer);

            if (readBuffer[0] == 'E'){
                perror("put file error");
                printf("%s\n" , readBuffer+1);
                close(fileDescriptor);
                close(dataConnectFD);
                continue;
            }

            readFileFromClient(fileDescriptor, dataConnectFD);
            close(fileDescriptor);
            close(dataConnectFD);
    
            
            printf("finished putting file on the server \n");

        }

        else{
            printf("Uknown Command \n");
        }
    }

    return 0;
}
   
