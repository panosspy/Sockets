#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>



void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	
    char buffer[256];
    if (argc < 3)
    {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    
	printf("Connected.\n");								//displays menu
	while(1){
                printf("----------------------\n");
		printf("Acceptable commands:\ncreate_child\nexec command(command to be executed by child)\nkill_child\nstop\n");
		printf("Give command\n");
                printf("----------------------\n");							
		bzero(buffer,256);
		fgets(buffer,255,stdin);							//reads first command
		n = write(sockfd,buffer,strlen(buffer));			//write to server the command
		if (n < 0)											//checking if message send
			error("ERROR writing to socket");
    
		bzero(buffer,256);
		n = read(sockfd,buffer,255);						//reads from server the response
		if (n < 0){											//checking if read was right
			error("ERROR reading from socket");
			break;
		}
		if(n == 0){
			printf("Server closed connection\n");
			break;
		}
		
		if(strncmp(buffer, "Terminating", 11)==0){			//checking if response was Terminate
			printf("%s\n",buffer);
			break;
		}
		printf("%s\n",buffer);								//shows response from server
		
		
	}
	
    close(sockfd);											//close connection and programm
    return 0;
	
}
