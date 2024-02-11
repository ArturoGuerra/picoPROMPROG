#pragma once

#include <pico/stdlib.h>

// TODO: Predifine know eeprom chips with correct sizes and timings

struct eeprom_info_t {
  const char *name;   // Name of the EEPROM
  uint address_mask;  // Address bus mask usually 15 bits so 0x7FFF
  uint data_mask;     // Databus mask usually 8 bits so 0xFF
  int size;           // EEPROM Size in bytes, usually 1 more than address mask
  int page_size;      // EEPROM Size in bytes, usually 64 bytes
  int page_delay_ms;  // EEPROM Page Delay in milliseconds
  int write_delay_ns; // Delay between states for write operations
  int read_delay_ns;  // Delay for data to be available for read operations
  bool write_protect;
  bool write_protect_disable;
};

const eeprom_info_t eeproms[] = {
    {
        "AT28C256",
        0x7FFF,
        0xFF,
        0x8000,
        64,
        10,
        100,
        350,
        false,
        false,
    },
};
