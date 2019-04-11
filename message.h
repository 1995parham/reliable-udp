#ifndef MESSAGE_H
#define MESSAGE_H

#define MAX_LINE 80
#define WINDOW_SIZE 10

#pragma pack(1)
struct message {
    int sequence_number;
    char payload[MAX_LINE];
};
#pragma pack(0)

#define MESSAGE_FIN 0x02
#define MESSAGE_ACK 0x03
#define MESSAGE_EMPTY 0x04

#endif
