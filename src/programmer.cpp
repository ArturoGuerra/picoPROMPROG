#include "programmer.h"
#include "config.h"
#include <hardware/clocks.h>

// PIO Headers
#include "addressbus.pio.h"
#include "databus.pio.h"

#define DATABUS_SPEED_MHZ 100
// 1000000000 / (DATABUS_SPEED_MHZ * 1000000) = Period in us
#define DATABUS_PERIOD_NS (1000 / DATABUS_SPEED_MHZ)

// Addressbus pins, we only need three since we are using 2 shift registers
#define ADDRESSBUS_BASE_PIN 16
#define ADDRESSBUS_PIN_COUNT 3

// Databus data pins and control pins (WE, OE, CS)
#define DATABUS_BASE_PIN 6
#define DATABUS_PIN_COUNT 8
#define DATABUS_SIDESET_BASE_PIN 19
#define DATABUS_SIDESET_PIN_COUNT 3

static inline void addressbus_init(PIO, int, uint, int pin_base, int pin_count);
static inline void databus_init(PIO, int, uint, pio_sm_config*, int pin_base, int pin_count, int sideset_pin_base, int sideset_pin_count, bool output);

Programmer::Programmer(const eeprom_info_t& info) {
    eeprom_info = info; // Gives programmer into about the eeprom
    op_mode = OP_MODE::READ; // Tell the programmer to start in read mode
    
    // We select pio slice to use, we migth want to dynamically select one in the future instead of hardcoding it.
    pio = pio0;
    
    // Initializes and starts addressbus state machine
    addressbus_sm = pio_claim_unused_sm(pio, true);
    addressbus_offset = pio_add_program(pio, &addressbus_program);
    addressbus_init(pio, addressbus_sm, addressbus_offset, ADDRESSBUS_BASE_PIN, ADDRESSBUS_PIN_COUNT);
    pio_sm_set_enabled(pio, addressbus_sm, true);
    
    // Stores databus SM Programs in an array for easy access
    db_programs[OP_MODE::READ] = &db_read_program;
    db_programs[OP_MODE::WRITE] = &db_write_program;
    
    // Initializes and starts databus state machine with required configs for selected op_mode
    databus_sm = pio_claim_unused_sm(pio, true);
    databus_offset = pio_add_program(pio, db_programs[op_mode]);
    databus_config = databus_get_default_config(databus_offset);
    databus_init(pio, databus_sm, databus_offset, &databus_config, DATABUS_BASE_PIN, DATABUS_PIN_COUNT, DATABUS_SIDESET_BASE_PIN, DATABUS_SIDESET_PIN_COUNT, false);
    pio_sm_set_enabled(pio, databus_sm, true);
    init_mode(op_mode);
};


Programmer::~Programmer() {
    // Stops state machines
    pio_sm_set_enabled(pio, addressbus_sm, false);
    pio_sm_set_enabled(pio, databus_sm, false);

    // Removes State Machine program from memory
    pio_remove_program(pio, &addressbus_program, addressbus_offset);
    pio_remove_program(pio, db_programs[op_mode], databus_offset);

    // Frees state machines for further use
    pio_sm_unclaim(pio, addressbus_sm);
    pio_sm_unclaim(pio, databus_sm);
}


void Programmer::set_mode(const OP_MODE mode) {
    // Return if state machine is already running the correct program
    if (mode == op_mode) return;

    // Stops state machine, and remove program from memory
    pio_sm_set_enabled(pio, databus_sm, false);
    pio_remove_program(pio, db_programs[op_mode], databus_offset);
    
    // Sets new mode and loads new program into memory
    op_mode = mode;
    databus_offset = pio_add_program(pio, db_programs[op_mode]);
    databus_config = databus_get_default_config(databus_offset);

    // Loads new program into state machine, and puts it in a known state
    databus_init(pio, databus_sm, databus_offset, &databus_config, DATABUS_BASE_PIN, DATABUS_PIN_COUNT, DATABUS_SIDESET_BASE_PIN, DATABUS_SIDESET_PIN_COUNT, (op_mode == OP_MODE::READ) ? false : true);
    
    // Enables state machine
    pio_sm_set_enabled(pio, databus_sm, true);
    init_mode(op_mode);
}


// Initializes the state machine by setting the correct timings for each EEPROM
void Programmer::init_mode(const OP_MODE mode) {
    switch(mode) {
        case OP_MODE::READ:
            pio_sm_put_blocking(pio, databus_sm, eeprom_info.read_delay_ns / DATABUS_PERIOD_NS);
            break;
        case OP_MODE::WRITE:
            pio_sm_put_blocking(pio, databus_sm, eeprom_info.write_delay_ns / DATABUS_PERIOD_NS);
            break;
        case OP_MODE::OP_MODE_SIZE:
            // Required to suppress compiler warning
            break;
    }
}

// Gets default State Machine config depending on which mode is selected
pio_sm_config Programmer::databus_get_default_config(uint offset) {
    pio_sm_config config;
    switch(op_mode) {
        case OP_MODE::READ:
            config = db_read_program_get_default_config(offset);
            break;
        case OP_MODE::WRITE:
            config = db_write_program_get_default_config(offset);
            break;
        case OP_MODE::OP_MODE_SIZE:
            // Required to suppress compiler warning
            break;
    }

    return config;
}

// Writes a single byte to the EEPROM
void Programmer::wbyte(address_t address, data_t data) {
    set_mode(WRITE);
    pio_sm_put_blocking(pio, addressbus_sm, address & eeprom_info.address_mask);
    pio_sm_put_blocking(pio, databus_sm, data & eeprom_info.data_mask);
    sleep_ms(eeprom_info.page_delay_ms);
};

// Reads a single byte from the EEPROM
data_t Programmer::rbyte(address_t address) {
    set_mode(READ);
    pio_sm_put_blocking(pio, addressbus_sm, address & eeprom_info.address_mask);
    return (data_t)pio_sm_get_blocking(pio, databus_sm) & eeprom_info.data_mask;
};

//// TODO: Migrate over to DMA Channel for better throughput
// Writes whole image or file to EEPROM starting at offset using fast write
void Programmer::wimage(const data_t *data, int size) {
    set_mode(WRITE);

    for (int address = 0; size > address; address++) {
        // Sleeps at page boudaries to allow eeprom to write data from latches
        if (address % eeprom_info.page_size == 0) sleep_ms(eeprom_info.page_delay_ms);
        
        pio_sm_put_blocking(pio, addressbus_sm, address & eeprom_info.address_mask);
        pio_sm_put_blocking(pio, databus_sm, data[address] & eeprom_info.data_mask);
    }
    
    // Ensures that the last page is written to the EEPROMq
    sleep_ms(eeprom_info.page_delay_ms);
};

//// TODO: Migrate over to DMA Channel for better throughput
// Reads whole image from EEPROM starting at offset and returns it in data[]
void Programmer::rimage(data_t *data, int &size) {
    set_mode(READ);
    size = eeprom_info.size;
    for (int address = 0; size > address; address++) {
        pio_sm_put_blocking(pio, addressbus_sm, address & eeprom_info.address_mask);
        data[address] = (data_t)pio_sm_get_blocking(pio, databus_sm) & eeprom_info.data_mask;
    }
};


void Programmer::chip_erase() {};
void Programmer::disable_data_protection() {};
void Programmer::enable_data_protection() {};

// Initializes addressbus pins and state machine
static inline void addressbus_init(PIO pio, int sm, uint offset, int pin_base, int pin_count) {
    // Setup pin mapping
    uint ser_pin = pin_base;
    uint rclk_pin = pin_base + 1;
    uint srclk_pin = pin_base + 2;

    pio_gpio_init(pio, ser_pin);
    pio_gpio_init(pio, rclk_pin);
    pio_gpio_init(pio, srclk_pin);
    
    gpio_pull_down(ser_pin);
    gpio_pull_down(rclk_pin);
    gpio_pull_down(srclk_pin);

    // Sets pins to output
    pio_sm_set_consecutive_pindirs(pio, sm, pin_base, pin_count, true);

    // Gets default config for state machine
    pio_sm_config c = addressbus_program_get_default_config(offset);

    // Sets up data pin
    sm_config_set_out_pins(&c, ser_pin, 1);
    // Sets up sideset pins or control pins for 74HC595N
    sm_config_set_sideset_pins(&c, rclk_pin);

    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    pio_sm_init(pio, sm, offset, &c);
}

// Initializes databus pins and state machine
static inline void databus_init(PIO pio, int sm, uint offset, pio_sm_config *c, int pin_base, int pin_count, int sideset_pin_base, int sideset_pin_count, bool output) {
    uint pin_mask, pin_dirs = 0x00;
    
    // Sets all data pins for EEPROM
    for (int i = 0; pin_count > i; i++) {
        uint pin = pin_base + i;
        pio_gpio_init(pio, pin);
        gpio_pull_down(pin);
        pin_mask |= (1u << pin);
        if (output) pin_dirs |= (1u << pin);
    }

    // Sets all the control pins for EEPROM
    for (int i = 0; sideset_pin_count > i; i++) {
        uint pin = sideset_pin_base + i;
        pio_gpio_init(pio, pin);
        gpio_pull_down(pin);
        pin_mask |= (1u << pin);
        if (output) pin_dirs |= (1u << pin);
    }

    // Applies pin mask to state machine
    pio_sm_set_pindirs_with_mask(pio, sm, pin_dirs, pin_mask);

    // Sets up state machine configs for each mode
    sm_config_set_out_pins(c, pin_base, pin_count);
    sm_config_set_in_pins(c, pin_base);
    sm_config_set_sideset_pins(c, sideset_pin_base);
    
    sm_config_set_in_shift(c, false, false, 8);
    sm_config_set_out_shift(c, true, false, 32);

    // Sets State Machine clock divider, normally runs at 100Mhz which gives a 10ns clock period
    // making every instruction in the state machine complete in that ammount of time
    float div = clock_get_hz(clk_sys) / (DATABUS_SPEED_MHZ * 1000000);
    sm_config_set_clkdiv(c, div);

    pio_sm_init(pio, sm, offset, c);
}