#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>

#include "message.h"

#define SERVER_PORT 5432

int main(int argc, char * argv[])
{
    FILE *fp;
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    char *fname;
    char buf[MAX_LINE];
    int s;
    int slen;
    char seq_num = 1;

    struct message send_window[WINDOW_SIZE];
    int wnd_ptr = 0;
    struct message fin;
    struct message ack;

    time_t now;
    double timeout_interval = 1;
    double estimated_rtt = 1;
    double dev_rtt = 0;

    if (argc==3) {
        host = argv[1];
        fname= argv[2];
    }
    else {
        fprintf(stderr, "Usage: ./client_udp host filename\n");
        exit(1);
    }

    /* translate host name into peer’s IP address */
    hp = gethostbyname(host);
    if (!hp) {
        fprintf(stderr, "Unknown host: %s\n", host);
        exit(1);
    }

    fp = fopen(fname, "r");
    if (fp==NULL){
        fprintf(stderr, "Can't open file: %s\n", fname);
        exit(1);
    }

    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);

    /* active open */
    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket");
        exit(1);
    }

    socklen_t sock_len = sizeof sin;

    /* main loop: get and send lines of text */
    while(fgets(buf, 80, fp) != NULL){
        // replaces \n with nil
        slen = strlen(buf);
        buf[slen] ='\0';

        if (wnd_ptr < WINDOW_SIZE) {
            send_window[wnd_ptr].sequence_number = seq_num++;
            strcpy(send_window[wnd_ptr].payload, buf);
            wnd_ptr++;
        }
        if (wnd_ptr == WINDOW_SIZE) {
retry:
            now = time(NULL);
            for (int i = 0; i < WINDOW_SIZE; i++) {
                if (sendto(s, &send_window[i], sizeof(struct message), 0, (struct sockaddr *) &sin, sock_len) < 0){
                    perror("SendTo Error\n");
                    exit(1);
                }
            }
            // wait for ack and retry if needed
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = (int)(timeout_interval * 1000);
            if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                perror("setsockopt");
                exit(1);
            }
            if (recvfrom(s, &ack, sizeof(struct message), 0, (struct sockaddr *)&sin, &sock_len) == 0) {
                perror("recvfrom");
                goto retry;
            }
            if (ack.payload[0] != MESSAGE_ACK || ack.sequence_number != seq_num) {
                goto retry;
            }
            int rtt = time(NULL) - now;
            dev_rtt = 0.75 * dev_rtt + 0.25 * fabs(rtt - estimated_rtt);
            estimated_rtt = 0.9 * estimated_rtt + 0.1 * rtt;
            timeout_interval = estimated_rtt + 4 * dev_rtt;
            wnd_ptr = 0;
        }
    }

    int empty_messages = wnd_ptr == 0 ? wnd_ptr : WINDOW_SIZE - wnd_ptr;
    for (int i = 0; i < empty_messages; i++) {
        send_window[wnd_ptr].sequence_number = seq_num++;
        send_window[wnd_ptr].payload[0] = MESSAGE_EMPTY;
        send_window[wnd_ptr].payload[1] = '\0';
        wnd_ptr++;
    }

retry_last:
    for (int i = 0; i < WINDOW_SIZE; i++) {
        if (sendto(s, &send_window[i], sizeof(struct message), 0, (struct sockaddr *) &sin, sock_len) < 0){
            perror("SendTo Error\n");
            exit(1);
        }
    }
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        exit(1);
    }
    if (recvfrom(s, &ack, sizeof(struct message), 0, (struct sockaddr *)&sin, &sock_len) == 0) {
          perror("recvfrom");
          goto retry_last;
    }
    if (ack.payload[0] != MESSAGE_ACK || ack.sequence_number != seq_num) {
          goto retry_last;
    }

    fin.sequence_number = seq_num++;
    fin.payload[0] = MESSAGE_FIN;
    fin.payload[1] = 0x0;
    if(sendto(s, &fin, sizeof(struct message), 0, (struct sockaddr *)&sin, sock_len) < 0){
        perror("SendTo Error\n");
        exit(1);
    }
    // wait for ack and retry if needed

    fclose(fp);
}

