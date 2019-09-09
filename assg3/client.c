# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <arpa/inet.h>

# define PORT 10000

int main(int argc, char** argv){
	char* ip = argv[1];
	int sd=socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	inet_pton(AF_INET, ip, &(server_addr.sin_addr));
	printf("server_addr: %s\n", inet_ntoa(server_addr.sin_addr));
	server_addr.sin_port=htons(PORT);
	if(connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
		printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	int le=0x12345678;
	le=htonl(le);
	while(1){
		char buff[100];
		memset(buff,0,100);
		scanf("%s",buff);
		printf("%s\n",buff);
		int len;
		if((len=send(sd,buff,strlen(buff),0))<0){
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			exit(0);
		}
		if(strcmp(buff,"exit")==0){
			close(sd);
			break;
		}
	}
	return 0;
}
