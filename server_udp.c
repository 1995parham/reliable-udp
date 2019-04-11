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
    char buf[MAX_LINE]; // recieve buffer
    struct sockaddr_in sin;
    int len;
    int s, i;
    struct timeval tv;
    char seq_num = 1;
    FILE *fp;

    struct message msg;

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
        if (recvfrom(s, &msg, sizeof(struct message), 0, (struct sockaddr *)&sin, &sock_len) == 0) {
                perror("recvfrom");
        }

        strcpy(buf, msg.payload);
        len = strlen(buf);
        printf("Message SN: %d (len: %d)\n", msg.sequence_number, len);
        if(len == 1){
            if (buf[0] == 0x02){
                printf("Transmission Complete\n");
                break;
            }
            else{
                perror("Error: Short packet\n");
            }
        }
        else if(len > 1){
            if(fputs((char *) buf, fp) < 1){
                printf("fputs() error\n");
            }
        }
    }
    fclose(fp);
    close(s);
}
