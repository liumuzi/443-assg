#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#include "myftp.c"

#define PORT 12345


void list_reply(int sd);
void put_reply(int sd, char *filename);
void get_reply(int sd, char *filename);

/*
 * Worker thread performing receiving and outputing messages
 */
void *pthread_prog(void *sDescriptor) {
  int sd = *(int *)sDescriptor;
  
  //while (1) {
    struct message_s *msg = (struct message_s *)malloc(sizeof(struct message_s));
    /* receive header message */
    if (recvMsg(sd, (struct message_s *)msg) == 1) {
      fprintf(stderr, "error receiving, exit!\n");
      return NULL;
    }
    /* Execute file transport functions */
    switch(msg->type)
    {
      case 0xA1:        /* LIST_REQUEST */
      printf("Received LIST_REQUEST.\n");
      list_reply(sd);
      printf("Sent file list.\n");
      break;
      case 0xB1:        /* GET_REQUEST */
      printf("Received GET_REQUEST.\n");
      char *filename = (char *)malloc(msg->length - sizeof(struct message_s) + 1);
      memset(filename, 0, msg->length - sizeof(struct message_s) + 1);
      if (recvData(sd, filename, msg->length - sizeof(struct message_s)) == 1)
      {
        fprintf(stderr, "error receiving payload, exit!\n");
        return 0;
      }
      get_reply(sd, filename);

      break;
      case 0xC1:        /* PUT_REQUEST */
      printf("Received PUT_REQUEST.\n");
      char *name = (char *)malloc(msg->length - sizeof(struct message_s) + 1);
      memset(name, 0, msg->length - sizeof(struct message_s) + 1);
      if (recvData(sd, name, msg->length - sizeof(struct message_s)) == 1)
      {
        fprintf(stderr, "error receiving payload, exit!\n");
        return 0;
      }
      put_reply(sd, name);
      break;
      default:
      fprintf(stderr, "wrong header type!\n");
    }
    free(msg);
  //}

}

int main(int argc, char **argv) {
  if (argc != 2)
  {
    printf("Usage: ./myftpserver PORT_NUMBER\n");
    exit(0);
  }

  int port = atoi(argv[1]);

  int sd = socket(AF_INET, SOCK_STREAM, 0);
//reusable ip
    long val=1;
  if(setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(long))==-1){
                perror("setsocketopt");
                exit(1);
  }

  int client_sd;
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);
  if (bind(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    printf("bind error: %s (Errno:%d)\n", strerror(errno), errno);
    exit(0);
  }
  if (listen(sd, 3) < 0) {
    printf("listen error: %s (Errno:%d)\n", strerror(errno), errno);
    exit(0);
  }
  
  while(1){
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    if ((client_sd = accept(sd, (struct sockaddr *)&client_addr, &addr_len)) <
        0) {
      printf("accept erro: %s (Errno:%d)\n", strerror(errno), errno);
      exit(0);
    } else {
      printf("receive connection from %s\n", inet_ntoa(client_addr.sin_addr));
    }
    pthread_t worker;
    pthread_create(&worker, NULL, pthread_prog, &client_sd);
  }
  
  close(sd);
  return 0;
}


void list_reply(int sd)
{
  char *files = (char *)malloc(sizeof(char));
  strcpy(files, "");
  int list_len = 0;

  DIR *dir;
  struct dirent *ptr;
  dir = opendir("data/");
  int count = 0;
  while((ptr = readdir(dir)) != NULL)
  {
    list_len = list_len + strlen(ptr->d_name) + 1;
    files = realloc(files, list_len + 1);
    strcat(files, ptr->d_name);
    strcat(files, "\n");
    count++;
  }
  closedir(dir);

  sendMsg(sd, 0xA2, sizeof(struct message_s) + list_len);
  sendData(sd, files, list_len);

  free(files);
}


void put_reply(int sd, char *filename){
  sendMsg(sd, 0xC2, sizeof(struct message_s) );
  struct message_s *msg = (struct message_s *)malloc(sizeof(struct message_s));
  if (recvMsg(sd, (struct message_s *)msg) == 1) {
    fprintf(stderr, "error receiving, exit!\n");
  }
  else{
    if (msg->type == 0xFF)
    {
      char *contents = (char *)malloc(msg->length - sizeof(struct message_s));
      if (recvData(sd, contents, msg->length - sizeof(struct message_s)) == 1)
      {
        fprintf(stderr, "error receiving payload, exit!\n");
      }
      else{
        char *dir = (char *)malloc(sizeof(char)*(6 + strlen(filename)));
        memset(dir, 0, 6 + strlen(filename));
        strcpy(dir, "data/");
        strcat(dir, filename);
        strcat(dir, "\0");
        recvFile(dir, contents, msg->length - sizeof(struct message_s));
        free(dir);
      }
    }
  }
}


void get_reply(int sd, char *filename){
  char *dir = (char *)malloc(sizeof(char)*(6 + strlen(filename)));
  memset(dir, 0, 6 + strlen(filename));
  strcpy(dir, "data/");
  strcat(dir, filename);
  strcat(dir, "\0");
  printf("filename: %s dir: %s strlen: %d\n", filename, dir, strlen(dir));
  FILE* fp = fopen(dir, "r");
  if (fp)
  {
    sendMsg(sd, 0xB2, sizeof(struct message_s));
    sendFile(sd, fp);
  }
  else
    sendMsg(sd, 0xB3, sizeof(struct message_s));
  free(dir);
}