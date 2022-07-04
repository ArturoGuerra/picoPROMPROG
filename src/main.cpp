#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <tusb.h>

#include "programmer.h"

#define ADDRESS_MASK 0x7FFF
#define DATA_MASK 70
#define MAX_MEM 0x8000
#define PAGE_SIZE 64

int main() {
    stdio_init_all();
    
    const eeprom_info_t eeprom_info = {
        "AT28C256",
        ADDRESS_MASK,
        DATA_MASK,
        MAX_MEM,
        PAGE_SIZE,
        10, // 10ms Page write delay
        100, // 100us write state delay
        350, // 350us max delay between address input and data output
        false,
        true,
    };

    Programmer *p = new Programmer(eeprom_info);
    
    sleep_ms(10000);

    uint8_t data = 0xF8;

    for (uint16_t address = 0; MAX_MEM > address; address++) {
        if (address % PAGE_SIZE == 0) {
           // printf("0x%04x\n", address);
            sleep_ms(10);
        }
        p->wbyte(address, data);
        //p->wbyte(address, address);
        //databus_write(pio, databus_sm, addrbus_sm, address, (uint8_t)address);
    }

    sleep_ms(100);


    for (uint16_t address = 0; MAX_MEM > address; address++) {
        //uint8_t ndata = databus_read(pio, databus_sm, addrbus_sm, address);
        data_t ndata = p->rbyte(address);
        //if (ndata != (uint8_t)address) {
        if (ndata != data) {
           printf("%04X: %02X\n", address, ndata);
        }
    }

    printf("-----\n");

    while(true) {
        tight_loop_contents();
    }

    return 0;
}