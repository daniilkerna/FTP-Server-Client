//  CS 360
//  Final Project 
//  Server Code
//
//  Created by Daniil V Kernazhytski on 4/11/18.
//

#include "server.h"


char* hello = "Hello From Server \n";
int DEBUG = 0;
struct sockaddr_in servAddr;
struct sockaddr_in clientAddr;
int listenfd;
int dataSocket;
int controlConnectionFD;
int dataConnectionFD;
bool dataConnectionOn = false;

char* getHostName(struct sockaddr_in* client){      //return hostname of the client 
    struct hostent* hostEntry;
    char* hostName;
    hostEntry = gethostbyaddr(&(client->sin_addr) , sizeof(struct in_addr), AF_INET);
    if (hostEntry == NULL) return "Unknown CLient Name";
    hostName = hostEntry -> h_name;
    return hostName;
}


void printGreetingMessage(){
    int pidNumber = (int) getpid();
    char* clientName = getHostName(&clientAddr);
   
    printf("Child %d : Host IP address is: %s \n",pidNumber, inet_ntoa(clientAddr.sin_addr));
    printf("Child %d : Client Hostname: %s \n",pidNumber, clientName);
    printf("Child %d : Client IP address is: %s \n",pidNumber, inet_ntoa(clientAddr.sin_addr));
    
    
}

void serverAckowledge(){
    char *message = malloc(sizeof(char) * 25);
    strcpy(message, "A\n");
    writeToClient(controlConnectionFD, message, strlen(message) * sizeof(char));
    free(message);

}

void serverReportError(){
    char *message = malloc(sizeof(char) * 25);
    strcpy(message, "E");
    strcat(message, strerror(errno) );
    strcat(message, "\n" );
    writeToClient(controlConnectionFD, message, strlen(message) * sizeof(char));
    free(message);
}

void setTimeOut(int sockfd, int time){
    int status;
    struct timeval tv;
    tv.tv_sec = time; 
    tv.tv_usec = 0;
    status = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    if (status < 0 ){
        perror("set timeout error ");
    }
}

int main(int argc, char** argv){
        if (argc > 2){
            printf("Wrong useage. Use with no flags or with -d \n" );
            return 0;
        }
        if (argc == 2){
            if (strcmp(argv[1],"-d") == 0){
                DEBUG = 1;
            }
        }


    char hostname[1024];
	int status; 
    status = gethostname(hostname, 1024);
	if ( status == -1){
		//perror("no hostname");
	}
        printf("Server Hostname: %s \n", hostname);
        printf("%s",  getTime() );
    
        makeSocket(&listenfd);
        bindSocket(listenfd ,&servAddr, MY_PORT_NUMBER);
        listenSocket(listenfd);
       
    
    while (1){
        int childID = 0;
        int length = sizeof(struct sockaddr_in);
        controlConnectionFD = accept(listenfd, (struct sockaddr*) &clientAddr,(socklen_t*) &length );
        waitpid(childID, NULL, WNOHANG);     //checks for zombies

        if ((childID = fork())) {        //parent
            if (DEBUG) printf("created child :%d \n" ,childID);
            close(controlConnectionFD);
            
        }
        else{               //child
            if (controlConnectionFD < 0) perror("Accept Error \n"), exit(1);
            else{
                int pidNumber = (int) getpid();
                int bytesRead = 0;
                printGreetingMessage();
                char* bufferRead = malloc(sizeof(char) * 4096);
                char* bufferWrite = malloc(sizeof(char) * 1028);
                char* pathName = malloc(sizeof(char) * 512);
                char* fileName = malloc(sizeof(char) * 100);
                setTimeOut(controlConnectionFD, 150 );  //set up timeout
                
                while (1){    //keep reading from the client
                    bytesRead = readFromClient(controlConnectionFD, bufferRead);
                    if (bytesRead == -1){
                        printf("Server Child %d About to Die \n" , pidNumber);
                        myExit();
                    }

                    if (DEBUG) printf("%s" , bufferRead);

                    if (strcmp(bufferRead, "Q\n") == 0  ){        //Got command Q
                        serverAckowledge();
                        free(bufferRead);
                        free(bufferWrite);
                        free(pathName);
                        free(fileName);
                        printf("Server Child %d About to Die \n" , pidNumber);
                        myExit();
                    }
                    
                    else if (strcmp(bufferRead, "D\n") == 0  ){        //Got command D
                        int dataPortNumber = makeDataConnection(&dataSocket, &servAddr);
                        if (DEBUG) printf("established new connection, port: %d\n" ,dataPortNumber);
                        strcpy(bufferWrite, "");
                        strcpy(bufferWrite, "A");
                        sprintf(bufferRead,"%d\n", dataPortNumber);
                        strcat(bufferWrite, bufferRead);        //prepare Dportnumber
                        if (DEBUG) printf("%s\n" , bufferWrite);
                        writeToClient(controlConnectionFD, bufferWrite , strlen(bufferWrite));  //write the port number
                        dataConnectionFD = accept(dataSocket, (struct sockaddr*) &clientAddr,(socklen_t*) &length );
                        dataConnectionOn = true;
                        printf("Child: %d  | Established Data connection, port: %d \n" , pidNumber , dataPortNumber);
                    }
                    
                    else if (strcmp(bufferRead, "L\n") == 0  ){        //Got command L
                        if (DEBUG) printf("about to List directory\n");
                        if (dataConnectionOn == false){
                            perror("connection not established ");
                            continue;
                        }
                        serverAckowledge();
                        
                       if (fork()){
                           
                           wait(NULL);
                           close(dataConnectionFD); 
                           dataConnectionOn = false;
                           printf("Child: %d  | finished listing directories\n" , pidNumber);
                       }
                       else{
                           dup2(dataConnectionFD, 1);
                           execlp("ls", "ls", "-l", NULL);
                           perror("failed executing ls on the server");
                           myExit();
                       }
                        
                    }

                    else if ( bufferRead[0] == 'C'){             // got C<pathname> 
                        if (DEBUG) printf("about to change directory\n");
                        strcpy(pathName, bufferRead+1);
                        removeNewLineCharacter(pathName);
                        if(DEBUG) printf("received path: %s \n" ,pathName);
                        int status = chdir(pathName);
                        if ( status == 0){
                            strcpy(bufferWrite, "A\n");
                            writeToClient(controlConnectionFD, bufferWrite, strlen(bufferWrite) * sizeof(char));
                            printf("Child: %d  |Server changed directory to %s \n" , pidNumber , pathName);
                        }
                        else{
                            strcpy(bufferWrite, "Edirectory doesn't exist or not accessible\n");
                            writeToClient(controlConnectionFD, bufferWrite, strlen(bufferWrite) * sizeof(char));
                            if (DEBUG) printf("Failed to change the directory \n");
                        }
                        
                    }

                    else if (bufferRead[0] == 'G'){          //got G<pathname>
                        if (dataConnectionOn == false){
                            perror("connection not established ");
                            continue;
                        }

                        if (DEBUG) printf("about to retreive a file\n");
                        strcpy(pathName, bufferRead+1);
                        removeNewLineCharacter(pathName);
                        if(DEBUG) printf("received path: %s \n" ,pathName);
                        
                        int fileFD = open(pathName, O_RDONLY);
                            if (fileFD < 0 ){
                                serverReportError();
                                close(dataConnectionFD);
                                continue;
                            }

                        if (isRegularFile(pathName) == 0){
                            serverReportError();
                            close(dataConnectionFD);
                            close(fileFD);
                            continue;
                        }
                        serverAckowledge();

                        if (fork()){    //parent
                            close(dataConnectionFD);
                            close(fileFD);
                            dataConnectionOn = false;
                            wait(NULL);
                            printf("Child: %d  |Server sent the file: %s \n" , pidNumber , pathName);
                        }
                        else{              // child
                            dup2(dataConnectionFD, 1);
                            readFileFromClient(fileFD, 1);
                            myExit();

                        }
                    }

                    else if (bufferRead[0] == 'P'){             //got P<pathname>
                        if (dataConnectionOn == false){
                            perror("connection not established ");
                            continue;
                        }

                        if (DEBUG) printf("about to put a file on the server\n");
                        strcpy(pathName, bufferRead+1);
                        parseFileName(pathName, fileName);
                        removeNewLineCharacter(fileName);

                        if (DEBUG) printf("got file name: %s \n" , fileName);

                        if (DEBUG) printf("about to create the file for reading \n");

                        int fileFD= open(fileName, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0666);    //open for writing
                        if (fileFD < 0 ){
                                serverReportError();
                                close(dataConnectionFD);
                                continue;
                            }

                        serverAckowledge();

                        readFileFromClient(dataConnectionFD, fileFD);
                        close(fileFD);
                        close(dataConnectionFD);
                        dataConnectionOn = false;
                        printf("Child: %d  |Server received the file:  %s \n" , pidNumber , pathName);
                    }

                    else{
                        printf("Uknown Command \n");
                    }


        
                }
            
            }
        }
    }
    
    printf("Server is Shutting down \n");
    return 0;
}
