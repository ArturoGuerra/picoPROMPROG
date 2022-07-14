#pragma once

#include "config.h"
#include <string>

enum menu_option_t {
    MENU_WRITE_IMAGE,
    MENU_READ_IMAGE,
    MENU_VERIFY_IMAGE,
    MENU_READ_BYTE,
    MENU_WRITE_BYTE,
    MENU_ERASE_CHIP,
    MENU_ENABLE_DATA_PROTECTION,
    MENU_DISABLE_DATA_PROTECTION,
    MENU_SELECT_CHIP,
    MENU_SHOW_SETTINGS,
    MENU_QUIT,
};

class Console {
public:
    std::string gets();
    int getint();

    void banner();
    void show_settings(const eeprom_info_t &eeprom_info);
    eeprom_info_t configure();
    menu_option_t menu();


};