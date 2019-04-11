#ifndef MESSAGE_H
#define MESSAGE_H

#define MAX_LINE 80

#pragma pack(1)
struct message {
    int sequence_number;
    char payload[MAX_LINE];
};
#pragma pack(0)

#endif
