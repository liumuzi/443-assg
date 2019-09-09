
/* structure of protocol messages specified in assg1.pdf */
struct message_s {
  unsigned char protocol[5];   /* protocol string (5 bytes) */
  unsigned char type;          /* type (1 byte) */
  unsigned int length;         /* length (header + payload) (4 bytes) */
}__attribute__((packed));


/* send header message */
int sendMsg(int sd, char type, int len);
/* recv and print header message (for debug) */
int recvMsg(int sd, struct message_s *msg);
/* send payload data */
int sendData(int sd, void *buff, int len);
/* recv payload data */
int recvData(int sd, void *buff, int len);
/* write contents to file*/
void recvFile (char *filename, char *contents, int length);
/* send file */
void sendFile (int sd, FILE *fp);