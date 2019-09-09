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

#include "myftp.h"


int sendMsg(int sd, char type, int len) {
  struct message_s *msg = (struct message_s *)malloc(sizeof(struct message_s));
  strcpy(msg->protocol, "myftp");
  msg->type = type;
  msg->length = len;

  int recvLen = 0;
  int header_len = sizeof(struct message_s);
  while (recvLen != header_len) {
    int rLen = send(sd, msg, header_len - recvLen, 0);
    if (rLen <= 0) {
      fprintf(stderr, "error sending msg\n");
      return 1;
    }
    recvLen += rLen;
  }
  free(msg);
  return 0;
}

int recvMsg(int sd, struct message_s *msg) {
  int recvLen = 0;
  int len = sizeof(struct message_s);
  while (recvLen != len) {
    int rLen = recv(sd, msg, len - recvLen, 0);
    if (rLen <= 0) {
      fprintf(stderr, "error recving msg\n");
      return 1;
    }
    recvLen += rLen;
  }

  printf("Received header: %s, %c, %d\n", msg->protocol, msg->type, msg->length);

  return 0;
}

int recvData(int sd, void *buff, int len) {
  int recvLen = 0;
  while (recvLen != len) {
    int rLen = recv(sd, buff + recvLen, len - recvLen, 0);
    if (rLen <= 0) {
      fprintf(stderr, "error recving msg\n");
      return 1;
    }
    recvLen += rLen;
  }
  return 0;
}

int sendData(int sd, void *buff, int len) {
  int recvLen = 0;
  while (recvLen != len) {
    int rLen = send(sd, buff + recvLen, len - recvLen, 0);
    if (rLen <= 0) {
      fprintf(stderr, "error sending msg, error code: %d\n", rLen);
      return 1;
    }
    recvLen += rLen;
  }
  return 0;
}
void recvFile (char *filename, char *contents, int length){
  FILE *fp;
  fp = fopen(filename, "w" );
  fwrite(contents, 1, length, fp );
  fclose(fp);
}

void sendFile (int sd, FILE *fp){
  fseek(fp, 0L, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char *contents = (char *)malloc(fsize+1);
  int count = fread(contents, fsize, 1, fp);
  sendMsg(sd, 0xFF, sizeof(struct message_s)+fsize);
  sendData(sd, contents, fsize);
}