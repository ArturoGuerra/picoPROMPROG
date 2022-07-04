#pragma once

#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/pio.h>

typedef uint16_t address_t;
typedef uint8_t data_t;

enum OP_MODE {
    READ = 0,
    WRITE = 1,
    OP_MODE_SIZE = 2,
};

struct eeprom_info_t {
    const char* name;  // Name of the EEPROM
    uint address_mask; // Address bus mask usually 15 bits so 0x7FFF
    uint data_mask;    // Databus mask usually 8 bits so 0xFF
    int size;          // EEPROM Size in bytes, usually 1 more than address mask
    int page_size;     // EEPROM Size in bytes, usually 64 bytes
    int page_delay_ms; // EEPROM Page Delay in milliseconds
    int write_delay_ns; // Delay between states for write operations
    int read_delay_ns; // Delay for data to be available for read operations
    bool write_protect;
    bool write_protect_disable;
};


class Programmer {
private:
    eeprom_info_t eeprom_info;    
    OP_MODE op_mode;
    
    // State Machine info
    PIO pio;
    int addressbus_sm, databus_sm;
    uint addressbus_offset, databus_offset;
    pio_sm_config databus_config;
    // Databus SM Programs, one for each OP_MODE
    const pio_program_t* db_programs[OP_MODE_SIZE];
    
    void set_mode(const OP_MODE mode);
    void init_mode(const OP_MODE mode);
    pio_sm_config databus_get_default_config(uint offset);

public:
    // Inits the programmer in read mode
    Programmer(const eeprom_info_t& info);
    // Needs to stop all state machines and reset them
    ~Programmer();

    // Byte operations
    void wbyte(address_t address, data_t data);
    data_t rbyte(address_t address);

    // Whole EEPROM Operations
    void wimage(data_t data[], int size, int offset);
    void rimage(data_t *data, int size, int offset);
    void chip_erase();
    void enable_data_protection();
    void disable_data_protection();
};