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

#include "myftp.c"

// #define IPADDR "127.0.0.1"
// #define PORT 12345


int main(int argc, char **argv) {
  if (argc < 4 || argc > 5)
  {
    printf("Usage: ./myftpclient <server ip addr> <server port> <list|get|put> <file>\n");
    exit(0);
  }

  char *ipaddr = argv[1];
  int port = atoi(argv[2]);

  int sd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr;
  pthread_t worker;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ipaddr);
  server_addr.sin_port = htons(port);
  if (connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    printf("connection error: %s (Errno:%d)\n", strerror(errno), errno);
    exit(0);
  }

  char *cmd = argv[3];

    /* LIST */
    if (strcmp(cmd, "list") == 0)
    {
      printf("List files.\n");
      sendMsg(sd, 0xA1, sizeof(struct message_s));

      struct message_s *msg = (struct message_s *)malloc(sizeof(struct message_s));
      if (recvMsg(sd, (struct message_s *)msg) == 1) {
        fprintf(stderr, "error receiving header, exit!\n");
        return 0;
      }
      char *files = (char *)malloc(msg->length - sizeof(struct message_s));
      if (recvData(sd, files, msg->length - sizeof(struct message_s)) == 1)
      {
        fprintf(stderr, "error receiving payload, exit!\n");
        return 0;
      }

      printf("%s", files);
    }
    /* GET */
    else if (strcmp(cmd, "get") == 0)
    {
      char *file = argv[4];
      //scanf("%s", file);
      printf("Get %s. strlen: %d\n", file, strlen(file));
      sendMsg(sd, 0xB1, sizeof(struct message_s) + strlen(file));
      sendData(sd, file, strlen(file));

      struct message_s *msg = (struct message_s *)malloc(sizeof(struct message_s));
      if (recvMsg(sd, (struct message_s *)msg) == 1) {
        fprintf(stderr, "error receiving, exit!\n");
      }
      else if(msg->type == 0xB2)
      {
        struct message_s *new_msg = (struct message_s *)malloc(sizeof(struct message_s));
        if (recvMsg(sd, (struct message_s *)new_msg) == 1) {
          fprintf(stderr, "error receiving, exit!\n");
        }

        if (new_msg->type == 0xFF)
        {
          char *contents = (char *)malloc(new_msg->length - sizeof(struct message_s));
          if (recvData(sd, contents, new_msg->length - sizeof(struct message_s)) == 1)
          {
            fprintf(stderr, "error receiving payload, exit!\n");
          }
          else{
            recvFile(file, contents, new_msg->length - sizeof(struct message_s));
          }
        }
      }

      else if(msg->type == 0xB3){
        printf("File %s does not exist!\n", file);
      }
    }

    /* PUT */
    else if (strcmp(cmd, "put") == 0)
    {
      char *file = argv[4];
      //scanf("%s", file);
      printf("Put %s.\n", file);
      FILE* fp = fopen(file, "r");
      if (fp)
      {
        sendMsg(sd, 0xC1, sizeof(struct message_s) + strlen(file));
        sendData(sd, file, strlen(file));

        struct message_s *msg = (struct message_s *)malloc(sizeof(struct message_s));
        if (recvMsg(sd, (struct message_s *)msg) == 1) {
          fprintf(stderr, "error receiving header, exit!\n");
          return 0;
        }
        sendFile(sd, fp);
        fclose(fp);

      }
      else{
        printf("File not exists!\n");
      }
    }
    else
    {
      printf("%s: Command not found. \nAvailable commands: list / get FILE / put FILE\n", cmd);
    }

  return 0;
}

