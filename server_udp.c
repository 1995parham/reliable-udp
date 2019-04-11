#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "message.h"

#define SERVER_PORT 5432

int main(int argc, char * argv[])
{
    char *fname;
    char buf[WINDOW_SIZE][MAX_LINE]; // recieve buffer
    struct sockaddr_in sin;
    int len;
    int s, i;
    struct timeval tv;
    int seq_num = 1;
    int recv_seq;
    FILE *fp;

    struct message msg;
    struct message ack;

    if (argc==2) {
        fname = argv[1];
    }
    else {
        fprintf(stderr, "usage: ./server_udp filename\n");
        exit(1);
    }

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);

    /* setup passive open */
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }
    if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
        perror("simplex-talk: bind");
        exit(1);
    }

    socklen_t sock_len = sizeof sin;

    /* open the given file to recieve into */
    fp = fopen(fname, "w");
    if (fp==NULL){
        printf("Can't open file\n");
        exit(1);
    }

    while(1){
retry:
        recv_seq = seq_num;

        for (int i = 0; i < WINDOW_SIZE; i++) {
            if (recvfrom(s, &msg, sizeof(struct message), 0, (struct sockaddr *)&sin, &sock_len) == 0) {
                    perror("recvfrom");
                    break;
            }

            strcpy(buf[i], msg.payload);
            len = strlen(buf[i]);
            printf("message recieved sn: %d - len: %d\n", msg.sequence_number, len);
            if (msg.sequence_number != recv_seq) {
                printf("unexpected message! sn: %d != %d\n", msg.sequence_number, recv_seq);
                goto retry;
            }
            recv_seq++;
            if(len == 1) {
                if (buf[i][0] == MESSAGE_FIN) {
                    printf("Transmission Complete\n");
                    goto fin;
                }
                if (buf[i][0] == MESSAGE_EMPTY) {
                    buf[i][0] = '\0';
                }
                else {
                    perror("Error: Short packet\n");
                }
            }
        }
        seq_num = recv_seq;
        ack.sequence_number = seq_num;
        ack.payload[0] = MESSAGE_ACK;
        ack.payload[1] = 0x0;
        if(sendto(s, &ack, sizeof(struct message), 0, (struct sockaddr *)&sin, sock_len) < 0){
            perror("SendTo Error\n");
            exit(1);
        }
        for (int i = 0; i < WINDOW_SIZE; i++) {
            if(strlen(buf[i]) > 0 && fputs((char *) buf[i], fp) < 1) {
                printf("fputs() error\n");
            }
        }
    }

fin:
    fclose(fp);
    close(s);
}
