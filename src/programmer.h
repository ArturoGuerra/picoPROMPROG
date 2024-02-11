#pragma once

#include <hardware/pio.h>
#include <pico/stdlib.h>

#include "config.h"

// Maximum number of bytes any eeprom can have
#define EEPROM_BUFFER_SIZE 0x10000

typedef uint16_t address_t;
typedef uint8_t data_t;

enum OP_MODE {
  READ = 0,
  WRITE = 1,
  OP_MODE_SIZE = 2,
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
  const pio_program_t *db_programs[OP_MODE_SIZE];

  void set_mode(const OP_MODE mode);
  void init_mode(const OP_MODE mode);
  pio_sm_config databus_get_default_config(uint offset);

public:
  // Inits the programmer in read mode
  Programmer(const eeprom_info_t &info);
  // Needs to stop all state machines and reset them
  ~Programmer();

  const eeprom_info_t &get_eeprom_info() const { return eeprom_info; }

  // Byte operations
  void wbyte(const address_t address, const data_t data);
  data_t rbyte(const address_t address);

  // Whole EEPROM Operations
  void wimage(const data_t *data, int size);
  void rimage(data_t *data, int &size);
  void chip_erase();
  void enable_data_protection();
  void disable_data_protection();
};
