// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// -------- //
// counter1 //
// -------- //

#define counter1_wrap_target 0
#define counter1_wrap 10

static const uint16_t counter1_program_instructions[] = {
            //     .wrap_target
    0xa027, //  0: mov    x, osr                     
    0x00c3, //  1: jmp    pin, 3                     
    0x0001, //  2: jmp    1                          
    0x00c3, //  3: jmp    pin, 3                     
    0x2020, //  4: wait   0 pin, 0                   
    0x20a0, //  5: wait   1 pin, 0                   
    0x00c8, //  6: jmp    pin, 8                     
    0x0044, //  7: jmp    x--, 4                     
    0xa0c1, //  8: mov    isr, x                     
    0x8020, //  9: push   block                      
    0xc000, // 10: irq    nowait 0                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program counter1_program = {
    .instructions = counter1_program_instructions,
    .length = 11,
    .origin = -1,
};

static inline pio_sm_config counter1_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + counter1_wrap_target, offset + counter1_wrap);
    return c;
}

static inline void counter1_program_init(PIO pio, uint sm, uint offset, uint input_pin, uint gate_pin) {
   pio_gpio_init(pio, input_pin);
   pio_sm_set_consecutive_pindirs(pio, sm, input_pin, 1, false);
   pio_sm_config c = counter1_program_get_default_config(offset);
   sm_config_set_in_pins(&c, input_pin); // for WAIT
   sm_config_set_jmp_pin(&c, gate_pin);  // for JMP
   pio_sm_init(pio, sm, offset, &c);
}

#endif
