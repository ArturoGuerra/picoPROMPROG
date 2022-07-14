#include "xmodem.h"
#include <string.h>
#include <pico/stdlib.h>

void xmodem_send(const char* buffer, const int size) {
    int packets = size / XMODEM_PACKET_SIZE;
    // If the size is not a multiple of the packet size, add 1 packet
    if (size % XMODEM_PACKET_SIZE != 0) packets++;

    bool sending = true;
    while(sending) {
        // Wait for NAK
        char c = getchar();
        if (c != XMODEM_NAK) continue;


        // Send the packet
        bool resend = false;
        for (int i = 0; packets > i; i++) {
            // Check if previous packet needs to be resent
            if (resend) i--;
            // Sends packet header
            putchar(XMODEM_SOH);
            // Packet number
            putchar(i + 1);
            // Packet number mask
            putchar(0xFF - (i + 1));

            // Packet data
            for (int y = 0; XMODEM_PACKET_SIZE > y; y++) {
                if (i + y == size - 1) {
                    putchar(EOF);
                } else {
                    putchar(buffer[i + y]);
                }
            }

            // Checksum
            char checksum = 0;
            putchar(checksum);

            // Wait for ACK
            char c = getchar();
            if (c != XMODEM_ACK) {
                // If NAK, retry
                if (c == XMODEM_NAK) {
                    resend = true;
                    continue;
                }
                // If CAN, cancel
                if (c == XMODEM_CAN) {
                    sending = false;
                    break;
                }
            }

            resend = false;
        }

        // Send EOT
        putchar(XMODEM_EOT);
        getchar();
        putchar(XMODEM_EOT);
        getchar();
    }
    
 
};

//// TODO: Dynamic max size support
XMODEM_STATUS xmodem_receive(char* buffer, int &size) {
    memset(buffer, 0, XMODEM_BUFFER_SIZE);
    
    XMODEM_STATUS error = OK;
    int packet_number = 1;
    int timeouts = 0;

    putchar(XMODEM_NAK);

    while(true) {
        // Handler header packet
        bool eot, can = false;
        while(true) {
            if (timeouts > 8) {
                break;
            }

            // Wait for packet header
            int soh = getchar_timeout_us(XMODEM_TIMEOUT_US);
            if (soh == XMODEM_SOH || soh == XMODEM_EOT || soh == XMODEM_CAN) {
                can = (soh == XMODEM_CAN);
                eot = (soh == XMODEM_EOT);
                break;
            } else if (soh == PICO_ERROR_TIMEOUT) {
                putchar(XMODEM_NAK);
                timeouts++;
            }
        }
        
        if (timeouts > 8) {
            error = RETRIES;
            break;
        }

        // Exit if EOT or CAN
        if (can || eot) {
            error = (can ? CANCELED : OK);
            putchar(XMODEM_ACK);
            break;
        }

        // Handlers packet number and packet number mask
        int packetnumber = getchar_timeout_us(XMODEM_TIMEOUT_US);
        if (packetnumber == PICO_ERROR_TIMEOUT) {
            putchar(XMODEM_NAK);
            timeouts++;
            continue;
        }

        int packetnumbermask = getchar_timeout_us(XMODEM_TIMEOUT_US);
        if (packetnumbermask == PICO_ERROR_TIMEOUT) {
            putchar(XMODEM_NAK);
            timeouts++;
            continue;
        }

        if ((char)packetnumber != (char)packet_number) {
            error = BAD_PACKET_NUMBER;
            for (int i = 0; 8 > i; i++) putchar(XMODEM_CAN);
            break;
        }

        // Packet number and packet number mask don't add up to 255
        if (packetnumber + packetnumbermask != 0xFF) {
            error = BAD_PACKET_NUMBER;
            for (int i = 0; 8 > i; i++) putchar(XMODEM_CAN);
            break;
        }

        if (size + XMODEM_PACKET_SIZE > XMODEM_BUFFER_SIZE) {
            error = BUFFER_TOO_SMALL;
            for (int i = 0; 8 > i; i++) putchar(XMODEM_CAN);
            break;
        }

        // Handles packet data
        int psize = 0;
        int checksum = 0;
        bool timeout = false;
        uint8_t packetbuff[XMODEM_PACKET_SIZE];
        for (int i = 0; i < XMODEM_PACKET_SIZE; i++) {
            int data = getchar_timeout_us(XMODEM_TIMEOUT_US);
            if (data == PICO_ERROR_TIMEOUT) {
                timeout = true;
                break;
            }
            
            packetbuff[i] = data;
            checksum += data;
            if (data != XMODEM_EOF) {
                psize++;
            }
        }

        if (timeout) {
            putchar(XMODEM_NAK);
            timeouts++;
            continue;
        }
        

        // Gets checksum hash from sender
        int checksum_hash = getchar_timeout_us(XMODEM_TIMEOUT_US);
        if (checksum_hash == PICO_ERROR_TIMEOUT) {
            putchar(XMODEM_NAK);
            timeouts++;
            continue;
        }

        // Send NAK if checksum is wrong
        if ((uint8_t)checksum != checksum_hash) {
            putchar(XMODEM_NAK);
            timeouts++;
            continue;
        }

        // Copies packet data to buffer
        memcpy(&buffer[size], packetbuff, psize);
        size += psize;

        // Acknowledge packet received
        putchar(XMODEM_ACK);
        // Increment packet number
        packet_number++;
        timeouts = 0;
        error = OK;
    }

    return error;
};