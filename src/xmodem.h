#pragma once

#include <stdio.h>

#define XMODEM_BUFFER_SIZE 0x10000

// Max packet size
#define XMODEM_PACKET_SIZE 128
// 10 Second timeout
#define XMODEM_TIMEOUT_US 10000

// Xmodem packet symbols
#define XMODEM_SOH 0x01
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_ETB 0x17
#define XMODEM_CAN 0x18
#define XMODEM_EOF 0x1A

enum XMODEM_STATUS {
    OK = 0,
    BAD_PACKET_NUMBER = 1,
    RETRIES = 2,
    CANCELED = 3,
    BUFFER_TOO_SMALL = 4,
    UNKOWN_ERROR = 5,
};

//enum XMODEM_PACKET {
//    XMODEM_PACKET_SOH = 0x01,
//    XMODEM_PACKET_EOT = 0x04,
//    XMODEM_PACKET_ACK = 0x06,
//    XMODEM_PACKET_NAK = 0x15,
//    XMODEM_PACKET_ETB = 0x17,
//    XMODEM_PACKET_CAN = 0x18,
//};
//
//struct  XmodemPacket {
//    XMODEM_PACKET Start;
//    uint8_t       PacketNumber;
//    uint8_t       PacketNumberMask;
//    uint8_t       Data[128];
//    uint8_t       Checksum;
//};

void xmodem_send(const char *buffer, const int size);
XMODEM_STATUS xmodem_receive(char* buffer, int &size);