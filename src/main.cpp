#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <tusb.h>

#include "programmer.h"

#define ADDRESS_MASK 0x7FFF
#define DATA_MASK 0xFF
#define MEM_SIZE 0x8000
#define PAGE_SIZE 64
#define PAGE_WRITE_DELAY_MS 10
#define WRITE_DELAY_NS 100
#define READ_DELAY_NS 100
#define WRITE_PROTECT 0
#define WRITE_PROTECT_DISABLE 0

#define DATA 0xFF

int main() {
    stdio_init_all();
    
    const eeprom_info_t eeprom_info = {
        "AT28C256",
        ADDRESS_MASK,
        DATA_MASK,
        MEM_SIZE,
        PAGE_SIZE,
        PAGE_WRITE_DELAY_MS, // 10ms Page write delay
        WRITE_DELAY_NS, // 100us write state delay
        READ_DELAY_NS, // 350us max delay between address input and data output
        WRITE_PROTECT,
        WRITE_PROTECT_DISABLE,
    };

    Programmer *p = new Programmer(eeprom_info);
    
    sleep_ms(10000);


    data_t wdata[(address_t)MEM_SIZE];
    for (int a = 0; MEM_SIZE > a; a++) wdata[a] = DATA;
    p->wimage(wdata, MEM_SIZE, 0);


    //for (uint16_t address = 0; MAX_MEM > address; address++) {
    //    if (address % PAGE_SIZE == 0) {
    //       // printf("0x%04x\n", address);
    //       sleep_ms(10);
    //    }
    //    p->wbyte(address, data);
        //p->wbyte(address, address);
        //databus_write(pio, databus_sm, addrbus_sm, address, (uint8_t)address);
    //}

    sleep_ms(100);

    data_t rdata[MEM_SIZE];
    p->rimage(rdata, MEM_SIZE, 0);

    for (uint16_t address = 0; MEM_SIZE > address; address++) {
        data_t data = rdata[address];
        if (data != DATA) {
            printf("0x%04x 0x%02x\n", address, data);
        }
    
        //uint8_t ndata = databus_read(pio, databus_sm, addrbus_sm, address);
        //data_t ndata = p->rbyte(address);
        //if (ndata != (uint8_t)address) {
        //if (ndata != data) {
        //   printf("%04X: %02X\n", address, ndata);
        //}
    }

    delete p;

    printf("-----\n");

    while(true) {
        tight_loop_contents();
    }

    return 0;
}