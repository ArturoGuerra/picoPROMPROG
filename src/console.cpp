#include <stdio.h>
#include <stdlib.h>
#include <pico/stdlib.h>
#include <string.h>
#include <map>
#include <sstream>

#include "console.h"

#define MAX_STRING_LENGTH 256

// Maps menu option to redable names
const std::map<std::string, menu_option_t> menu_options_map = {
    {"w", MENU_WRITE_IMAGE},
    {"r", MENU_READ_IMAGE},
    {"v", MENU_VERIFY_IMAGE},
    {"rb", MENU_READ_BYTE},
    {"wb", MENU_WRITE_BYTE},
    {"e", MENU_ERASE_CHIP},
    {"ep", MENU_ENABLE_DATA_PROTECTION},
    {"dp", MENU_DISABLE_DATA_PROTECTION},
    {"cs", MENU_SELECT_CHIP},
    {"s", MENU_SHOW_SETTINGS},
    {"q", MENU_QUIT},
};

// Handles string input
static inline const char* input_handler() {
    char* data = new char[MAX_STRING_LENGTH];
    printf("> ");
    int c = getchar();
    int size = 0;
    while((c != 13 || size == 0) && size != (MAX_STRING_LENGTH - 2)) {
        if (126 >= c) {
            putchar(c);
            data[size] = (char)c;
            size++;
        }

        c = getchar();
    }

    putchar('\n');
    data[size] = '\0';

    return data;
};

std::string Console::gets() {
    const char* cstr = input_handler();
    std::string str(cstr);
    delete cstr;
    return str;
};

int Console::getint() {
    int value;


    while(true) {
        std::string s = gets();
        std::stringstream ss = std::stringstream(s);
        if (ss >> value) {
            break;
        }
    }
    
    return value;
};

void Console::banner() {
    printf("--------------------------------\n");
    printf("  Pipico EEPROM Programmer v1.0\n");
    printf("  By Arturo Guerra\n");
    printf("--------------------------------\n");
    printf("\n\n");
};

eeprom_info_t Console::configure() {
    int eeproms_size = sizeof(eeproms) / sizeof(eeproms[0]);
    eeprom_info_t eeprom_info;

    printf("Select EEPROM:\n");
    for(int i = 0; i < eeproms_size; i++) {
        printf("%d: %s\n", i, eeproms[i].name);
    }
    printf("%d: Custom\n", eeproms_size);
    
    int selection;
    while(true) {
        selection = getint();
        if (eeproms_size >= selection) {
            break;
        } else {
            printf("Invalid selection\n");

        }
        
    }

    if (selection == eeproms_size) {
        printf("Enter EEPROM name: ");
        eeprom_info.name = gets().c_str();
        printf("Enter EEPROM address mask: ");
        eeprom_info.address_mask = getint();
        printf("Enter EEPROM data mask: ");
        eeprom_info.data_mask = getint();
        printf("Enter EEPROM size: ");
        eeprom_info.size = getint();
        printf("Enter EEPROM page size: ");
        eeprom_info.page_size = getint();
        printf("Enter EEPROM page delay (ms): ");
        eeprom_info.page_delay_ms = getint();
        printf("Enter EEPROM write delay (ns): ");
        eeprom_info.write_delay_ns = getint();
        printf("Enter EEPROM read delay (ns): ");
        eeprom_info.read_delay_ns = getint();
        eeprom_info.write_protect = false;
        eeprom_info.write_protect_disable = false;
    } else {
        eeprom_info = eeproms[selection];
    }

    return eeprom_info;
};


menu_option_t Console::menu() {
    printf("\n");
    printf("w: Write Image\n");
    printf("r: Read Image\n");
    printf("v: Verify Image\n");
    printf("rb: Read Byte\n");
    printf("wb: Write Byte\n");
    printf("e: Erase Chip\n");
    printf("ep: Enable Data Protection\n");
    printf("dp: Disable Data Protection\n");
    printf("s: Show Settings\n");
    printf("q: Quit\n");
    
    menu_option_t menu_option = MENU_QUIT;

    bool valid_option = false;
    while(!valid_option) {
        std::string data = gets();
        
        auto it = menu_options_map.find(data);
        if (it != menu_options_map.end()) {
            valid_option = true;
            menu_option = it->second;
        } else {
            printf("\nInvalid option\n");
        }
        
    }    

    return menu_option;
};


void Console::show_settings(const eeprom_info_t &eeprom_info) {
    printf("\n\n");
    printf("EEPROM Settings:\n");
    printf("\n");
    printf("Chip: %s\n", eeprom_info.name);
    printf("Size: %d bytes\n", eeprom_info.size);
    printf("Page Size: %d bytes\n", eeprom_info.page_size);
    printf("Page Delay: %d ms\n", eeprom_info.page_delay_ms);
    printf("Write Delay: %d ns\n", eeprom_info.write_delay_ns);
    printf("Read Delay: %d ns\n", eeprom_info.read_delay_ns);
    printf("\n");
 };