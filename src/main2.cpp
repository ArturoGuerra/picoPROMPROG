#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <tusb.h>


#include "addressbus.pio.h"
#include "databus.pio.h"

void addressbus_init(PIO, int, uint, int);
void databus_init(PIO, int, uint, int, int);

uint8_t databus_read(PIO, int, int, uint16_t);
void databus_write(PIO, int, int, uint16_t, uint8_t);


#define ADDRESSBUS_BASE_PIN 16
#define ADDRESSBUS_PIN_COUNT 3

#define DATABUS_BASE_PIN 6
#define DATABUS_PIN_COUNT 8

#define DATABUS_SIDESET_BASE_PIN 19
#define DATABUS_SIDESET_PIN_COUNT 3

#define DATABUS_INPUT_MASK 0x000
#define DATABUS_OUTOUT_MASK 0x1FF
                          
#define ADDRESS_MASK 0x7FFF
#define DATA_MASK 70


// EEPROM Info 
#define MAX_MEM 0x10
#define PAGE_SIZE 64

int main() {
    stdio_init_all();
    
    PIO pio = pio0;
    
    
    int addrbus_sm = pio_claim_unused_sm(pio, true);
    uint addrbus_offset = pio_add_program(pio, &addressbus_program);
    
    int databus_sm = pio_claim_unused_sm(pio, true);
    uint databus_offset = pio_add_program(pio, &databus_program);
   
    addressbus_init(pio, addrbus_sm, addrbus_offset, ADDRESSBUS_BASE_PIN);
    databus_init(pio, databus_sm, databus_offset, DATABUS_BASE_PIN, DATABUS_SIDESET_BASE_PIN);

    sleep_ms(10000);

    uint16_t data = 0xEA;

    //for (uint16_t address = 0; MAX_MEM > address; address++) {
    //    databus_write(pio, databus_sm, addrbus_sm, address, data);
    //}



    for (uint16_t address = 0; MAX_MEM > address; address++) {
        uint8_t ndata = databus_read(pio, databus_sm, addrbus_sm, address);
        //if (ndata != data) {
            printf("%04X: %02X\n", address, ndata);
        //}
    }

    printf("-----\n");


    while(true) {
        tight_loop_contents();
    }

//    uint16_t address = 0;
//    while(true) {
//        data = databus_read(pio, databus_sm, addrbus_sm, address % 10);
//        printf("Address: 0x%04X Data: 0x%02X RWB: %s\n", address % 10, data, "R");
//            
//        data++;
//        databus_write(pio, databus_sm, addrbus_sm, address % 10, data);
//        printf("Address: 0x%04X Data: 0x%02X RWB: %s\n", address % 10, data, "W");
//        data = databus_read(pio, databus_sm, addrbus_sm, address % 10);
//        printf("Address: 0x%04X Data: 0x%02X RWB: %s\n", address % 10, data, "R");
//        data = databus_read(pio, databus_sm, addrbus_sm, address % 10);
//        printf("Address: 0x%04X Data: 0x%02X RWB: %s\n", address % 10, data, "R");
//        printf("------------------\n");
//        sleep_ms(500);
//        address++;
//    }

    return 0;
}



/*
Sends instruction to State Machine to set databus to output and jump to write routine,
then it writes a byte to the databus
*/

void databus_write(PIO pio, int d_sm, int a_sm, uint16_t address, uint8_t data) {
    pio_sm_put_blocking(pio, a_sm, address & ADDRESS_MASK);
    // First 8 bits are for pindirs, 1 bit to tell state-machine how to branch, last 8 bits are for pinvalues.
    uint payload = (data << 9) | DATABUS_OUTOUT_MASK;
    pio_sm_put_blocking(pio, d_sm, payload);
    sleep_ms(10);
}


/*
Sends instruction to State Machine to set databus to input and jump to read routine,
then it reads a byte from the databus
*/

uint8_t databus_read(PIO pio, int d_sm, int a_sm, uint16_t address) {
    pio_sm_put_blocking(pio, a_sm, address);
    // First 8 bits are for pindirs, 1 bit to tell state-machine how to branch.
    uint data = DATABUS_INPUT_MASK;
    pio_sm_put_blocking(pio, d_sm, data);
    return (uint8_t)pio_sm_get_blocking(pio, d_sm);
}

void addressbus_init(PIO pio, int sm, uint offset, int base_pin) {
    int ser_pin = base_pin; // Shift register data pin
    int rclk_pin = base_pin + 1; // Shift-register data is moved to storage register
    int srclk_pin = base_pin + 2; // Shifts bit to shift-register
    pio_gpio_init(pio, ser_pin);
    pio_gpio_init(pio, rclk_pin);
    pio_gpio_init(pio, srclk_pin);
    gpio_pull_down(ser_pin);
    gpio_pull_down(rclk_pin);
    gpio_pull_down(srclk_pin);

    pio_sm_set_consecutive_pindirs(pio, sm, base_pin, 3, true);

    pio_sm_config c = addressbus_program_get_default_config(offset);

    sm_config_set_out_pins(&c, base_pin, 1);
    sm_config_set_sideset_pins(&c, rclk_pin);

    //sm_config_set_out_shift(&c, true, false, 16);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

void databus_init(PIO pio, int sm, uint offset, int base_pin, int base_sideset_pin) {
    uint pin_mask = 0x00;
    uint pin_dirs = 0x00;
    
    // Sets databus pins and sideset pins
    for (int i = 0; DATABUS_PIN_COUNT > i; i++) {
        uint pin = base_pin + i;
        pio_gpio_init(pio, pin);
        pin_mask |= (1u << pin);
        pin_dirs |= (1u << pin);
        gpio_pull_down(pin);
    }

    for (int i = 0; DATABUS_SIDESET_BASE_PIN > i; i++) {
        uint pin = base_sideset_pin + i;
        pio_gpio_init(pio, pin);
        pin_mask |= (1u << pin);
        pin_dirs |= (1u << pin);
        gpio_pull_down(pin);
    }

    pio_sm_set_pindirs_with_mask(pio, sm, pin_dirs, pin_mask);

    pio_sm_config c = databus_program_get_default_config(offset);

    sm_config_set_out_pins(&c, base_pin, 8);
    sm_config_set_in_pins(&c, base_pin);
    sm_config_set_sideset_pins(&c, base_sideset_pin);

    sm_config_set_in_shift(&c, false, false, 8);   
    sm_config_set_out_shift(&c, true, false, 17);

    float div = clock_get_hz(clk_sys) / (20 * 1000000);
    sm_config_set_clkdiv(&c, div);
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}