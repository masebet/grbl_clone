#include "grbl.h"

#define CS1_PIN PC0
#define CS2_PIN PC1

#define DEBUG_SPI

#ifdef DEBUG_SPI
void debug_print_read(uint8_t addr, uint8_t data) {
    serial_write('R');
    serial_write('[');
    if (addr < 16) serial_write('0');
    serial_write((addr < 10) ? ('0' + addr) : ('A' + addr - 10));
    serial_write(']');
    serial_write('=');
    if (data < 16) serial_write('0');
    serial_write((data >> 4) < 10 ? ('0' + (data >> 4)) : ('A' + (data >> 4) - 10));
    serial_write((data & 0xF) < 10 ? ('0' + (data & 0xF)) : ('A' + (data & 0xF) - 10));
    serial_write('\n');
}
#endif

void SPI_init() {
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);
    DDRC |= (1 << CS1_PIN) | (1 << CS2_PIN);
    PORTC |= (1 << CS1_PIN) | (1 << CS2_PIN);
    // Slower SPI clock: SPR1=1, SPR0=1 = fosc/128
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0) | (1 << CPHA);
}

void SPI_write(uint8_t cs_pin, uint8_t addr, uint8_t data) {
    uint16_t frame = ((addr & 0x3F) << 8) | data;
    
    PORTC &= ~(1 << cs_pin);
    
    SPDR = frame >> 8;
    while(!(SPSR & (1 << SPIF)));
    
    SPDR = frame & 0xFF;
    while(!(SPSR & (1 << SPIF)));
    
    PORTC |= (1 << cs_pin);
}

uint8_t SPI_read(uint8_t cs_pin, uint8_t addr) {
    uint16_t frame = (1 << 14) | ((addr & 0x3F) << 8);
    
    PORTC &= ~(1 << cs_pin);
    _delay_us(1);  // CS setup time
    
    SPDR = frame >> 8;
    while(!(SPSR & (1 << SPIF)));
    
    SPDR = frame & 0xFF;
    while(!(SPSR & (1 << SPIF)));
    
    _delay_us(1);  // Small delay before reading
    
    SPDR = 0;
    while(!(SPSR & (1 << SPIF)));
    uint8_t status = SPDR;  // Status byte
    
    SPDR = 0;
    while(!(SPSR & (1 << SPIF)));
    uint8_t data = SPDR;    // Data byte
    
    _delay_us(1);  // CS hold time
    PORTC |= (1 << cs_pin);
    
    #ifdef DEBUG_SPI
    debug_print_read(addr, data);
    // Also print status for debugging
    serial_write('S'); serial_write('=');
    serial_write((status >> 4) < 10 ? ('0' + (status >> 4)) : ('A' + (status >> 4) - 10));
    serial_write((status & 0xF) < 10 ? ('0' + (status & 0xF)) : ('A' + (status & 0xF) - 10));
    serial_write(' ');
    #endif
    
    return data;
}

void motor_spi_init() {
    if (MACHINE_TYPE != BAMBOO) return;
    
    _delay_ms(100);
    SPI_init();
    
    for (uint8_t cs = CS1_PIN; cs <= CS2_PIN; cs++) {
        // Write configuration first
        SPI_write(cs, 0x04, 0xCF);
        _delay_ms(1);
        SPI_write(cs, 0x05, 0x06);
        _delay_ms(5);  // Longer delay after write
        
        // Then read back to verify
        SPI_read(cs, 0x04);
        SPI_read(cs, 0x05);
    }
}