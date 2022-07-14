#include <tusb.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/binary_info.h>
#include <hardware/pio.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>

#include "programmer.h"
#include "console.h"
#include "xmodem.h"
#include "config.h"

void menu_write_image(Programmer *p, Console &c);
void menu_read_image(Programmer *p, Console &c);
void menu_verify_image(Programmer *p, Console &c);
void menu_read_byte(Programmer *p, Console &c);
void menu_write_byte(Programmer *p, Console &c);

int main() {
    bi_decl(bi_program_description("EEPROM Programmer"));

    stdio_init_all();

    // Waits for usb serial to be connected
    while(!tud_cdc_connected()) sleep_ms(1000);
    
    printf("EEPROM Programmer\n");


//    printf("Press enter to start transfer.\n");
//    char c = getchar();
//    while(c != '\r') {
//        c = getchar();
//        sleep_ms(100);
//    }
//
//    printf("Reading..\n");
//
//    int size = 0;
//    char buffer[XMODEM_BUFFER_SIZE];
//    XMODEM_STATUS status = xmodem_receive(buffer, size);
//
//    c = getchar();
//    while(c != '\r') {
//        c = getchar();
//        sleep_ms(100);
//    }
//
//    if (status != OK) {
//        printf("Error: %d\n", status);
//        return -1;
//    }
//
//    printf("Received %d bytes\n", size);
//
//    bool ok = true;
//    for (int i = 0; size > i; i++) {
//        if (buffer[i] != 0xEA) {
//            printf("%d\n", buffer[i]);
//            ok = false;
//            break;
//        }
//    }
//
//
//    printf("%s\n", ok ? "OK" : "FAIL");


    Console console;

    console.banner();

    while(true) {
        const eeprom_info_t eeprom_info = console.configure();
        Programmer *p = new Programmer(eeprom_info);  

        bool chip_select = true;
        while(chip_select) {
            menu_option_t option = console.menu();
            switch(option) {
                case MENU_WRITE_IMAGE:
                    menu_write_image(p, console);
                    break;
                case MENU_READ_IMAGE:
                    menu_read_image(p, console);
                    break;
                case MENU_VERIFY_IMAGE:
                    menu_verify_image(p, console);
                    break;
                case MENU_READ_BYTE:
                    menu_read_byte(p, console);
                    break;
                case MENU_WRITE_BYTE:
                    menu_write_byte(p, console);
                    break;
                case MENU_ERASE_CHIP:
                    p->chip_erase();
                    break;
                case MENU_ENABLE_DATA_PROTECTION:
                    p->enable_data_protection();
                    break;
                case MENU_DISABLE_DATA_PROTECTION:
                    p->disable_data_protection();
                    break;
                case MENU_SELECT_CHIP:
                    chip_select = true;
                    break;
                case MENU_SHOW_SETTINGS:
                    console.show_settings(eeprom_info);
                    break;
                case MENU_QUIT:
                    break;
            }
            
        }

        delete p;
    }

    return 0;
}

void menu_write_image(Programmer *p, Console &c) {
    int size = 0;
    char buffer[XMODEM_BUFFER_SIZE];
    
    printf("Press enter to start reading from xmodem.\n");
    char o = getchar();
    while(o != '\r') {    
        sleep_ms(100);
        o = getchar();
    }

    printf("Reading..\n");

    XMODEM_STATUS s = xmodem_receive(buffer, size);
    
    o = getchar();
    while(o != '\r') {    
        sleep_ms(100);
        o = getchar();
    }

    if (s != OK) {
        printf("Error: %d\n", s);
        return;
    }

    printf("Received %d bytes\n", size);

    printf("Writing to EEPROM...\n");
    p->wimage((const data_t*)buffer, size);

    int vsize = 0;
    char vbuffer[EEPROM_BUFFER_SIZE];
    printf("Reading EEPROM..\n");
    p->rimage((data_t*)vbuffer, vsize);

    if (vsize != size) {
        printf("Error: Buffers are not the same size!\n");
        printf("%d != %d\n", vsize, size);
        return;
    }
    
    for (int i = 0; size > i; i++) {
        if (buffer[i] != vbuffer[i]) {
            printf("Error: data is not the same!\n");
            return;
        }
    }

    printf("Done writing!\n");
}

void menu_read_image(Programmer *p, Console &c) {
    printf("Reading EEPROM...\n");
    int rsize = 0;
    data_t data[EEPROM_BUFFER_SIZE];
    p->rimage(data, rsize);
    printf("Image read, Size: %d\n", rsize);
    printf("Sending image...\n");
    xmodem_send((const char*)data, rsize);
    printf("Image sent\n");
}

void menu_verify_image(Programmer *p, Console &c) {
    printf("Verifing EEPROM...\n");
    int xsize, vsize = 0;
    char xbuffer[XMODEM_BUFFER_SIZE], vbuffer[EEPROM_BUFFER_SIZE];

    xmodem_receive(xbuffer, xsize);
    p->rimage((data_t*)vbuffer, vsize);

    if (vsize != xsize) {
        printf("EEPROM size is not the same as Xmodem buffer size!\n");
    }

    for (int i = 0; xsize > i; i++) {
        if (xbuffer[i] != vbuffer[i]) {
            printf("Error: Failed to verify EEPROM content, checksum error!\n");
            return;
        }
    }

    printf("Verified EEPROM!\n");
}

void menu_write_byte(Programmer *p, Console &c) {
    printf("Enter address: ");
    uint address = c.getint();
    printf("Enter data: ");
    uint data = c.getint();
    p->wbyte(address, data);
}

void menu_read_byte(Programmer *p, Console &c) {
    printf("Enter address: ");
    uint address = c.getint();
    uint data = p->rbyte(address);
    printf("Data: %d\n", data);
}