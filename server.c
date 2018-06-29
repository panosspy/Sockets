#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <fcntl.h>

#define READ 0 		//Read end of pipe
#define WRITE 1 	//Write end of pipe

int alarmflag=0;  	//Global alarm flag

void alarmHandler(); //Forward declaration

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256], *exec, *command;
    struct sockaddr_in serv_addr, cli_addr;
    int n, fd[2], file, k, pid, status, bytes;
	int ck=0;
	int childId=-100;
	
	if(pipe(fd)==-1){			//create a pipe
		perror("pipe");
		exit(1); 
	}
	
    if (argc < 2)
    {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
		
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)				//bind socket to server
        error("ERROR on binding");
	
	while(1){
		
		listen(sockfd,5);																	//listen for the client
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);					//accept connection with client
		if (newsockfd < 0)
			error("ERROR on accept");
		
		char clntName[INET_ADDRSTRLEN];
		
		if(inet_ntop(AF_INET,&cli_addr.sin_addr.s_addr,clntName,sizeof(clntName))!='\0'){  
			printf("Client ip/port:%s/%d\n",clntName,ntohs(cli_addr.sin_port));  
		} else {
			printf("Unable to get address\n");
		}
	
		do{
			printf("Waiting for command...\n");
			bzero(buffer,256);
			n = read(newsockfd,buffer,255);							//read from client
			if (n < 0) 
				error("ERROR reading from socket");
		
			if (strncmp(buffer,"stop",4)==0){						//checking command from client if it is stop
				ck=2;
                if(childId>0)										//if there is a child then kill it
		            kill(childId,SIGKILL);
				n = write(newsockfd,"Terminating",11);				//send message back
				if (n < 0) 
					error("ERROR writing to socket");
			}
			
			else if (strncmp(buffer,"kill_child",10)==0){			//checking command
				if (childId<=0){									//check if child exists
					n = write(newsockfd,"Child does not exist",20);
					if (n < 0) 
						error("ERROR writing to socket");
				}
				else{
					k = kill(childId,SIGKILL);						//send signal to kill child
					wait(&status);
					if (k==-1)
						error("ERROR could not kill child");
					n = write(newsockfd,"Child killed",12);
					if (n < 0) 
						error("ERROR writing to socket");
				}
			}
			
			else if(strncmp(buffer,"create_child",12)==0){			//checking what command has the client give
				pid = fork();
				if(pid==-1){								
					error("ERROR forking");
				}
				else if(pid!=0){
					childId = pid;
					n = write(newsockfd,"Child created",13);
					if (n < 0) 
						error("ERROR writing to socket");
				}
				else{
					childId=-100;
					ck=2;
					signal( SIGALRM, alarmHandler ); 				//Install signal handler
					while (!alarmflag){ 							//Loop until flag set
						sleep(2);
						printf("Child is running\n");
					}
					close(fd[WRITE]);							/*close unused end */
					bytes=read(fd[READ], command, sizeof(command));
					
					printf("lalalala\n");
                    file = open("executed_command",O_RDWR|O_CREAT,0600);
					if(file==-1){
						error("ERROR opening file");
						exit(1);
					}
                    printf("xaxaxa\n");
					dup2(file,1);				//change output stream to file
					execlp(command,command,NULL);
					
				}
			}
			
			else if(strncmp(buffer,"exec ",5)==0){
				if (childId<0){									//check if child exists
					n = write(newsockfd,"Child does not exist",20);
					if (n < 0) 
						error("ERROR writing to socket");
				}
				else{
					exec = strtok(buffer," ");
					command = strtok( NULL," ");
					printf("%s/%s\n",exec,command);
					close(fd[READ]);							//close unused end
					write(fd[WRITE], command, strlen(command)+1);

					kill(childId,SIGALRM);
					n = write(newsockfd,"Child did the command",21);
					if (n < 0) 
						error("ERROR writing to socket");
					wait(&status);
					childId=0;
				}	
			}
			
			else{											//if command is unknown then print to client the message  
				n = write(newsockfd,"Unknown command",15);
				if (n < 0) 
							error("ERROR writing to socket");
			}
		}while(ck!=2);
	}
    close(newsockfd);
	
    close(sockfd);
    return 0;
}

void alarmHandler(){
	printf("Child received alarm signal\n");							//confirmation message
	alarmflag = 1;											//change flag's value
} //Set flag to stop looping
