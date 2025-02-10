
#include "grbl.h"
#define CS1_PORT PORTC  // Chip Select for Driver 1
#define CS1_PIN  PC0

#define CS2_PORT PORTC  // Chip Select for Driver 2
#define CS2_PIN  PC1

void debug_print(const char *msg)
{
    while (*msg)
    {
        serial_write(*msg++); // Send each character over serial
    }
}

// Chip Select Pin Definitions
#define CS1_PIN PC0 // Chip Select for Driver 1
#define CS2_PIN PC1 // Chip Select for Driver 2

// SPI Timing Parameters (in ns)
#define SPI_SCLK_PERIOD 100
#define SPI_SETUP_TIME 20
#define SPI_HOLD_TIME 30

// Function to initialize SPI communication
void SPI_init() {
    // Set MOSI, SCK, and SS as output
    DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);
    
    // Set Chip Select pins as outputs
    DDRC |= (1 << CS1_PIN) | (1 << CS2_PIN);
    
    // Deselect all chips
    PORTC |= (1 << CS1_PIN) | (1 << CS2_PIN);
    
    // Enable SPI, Set as Master, Clock rate fck/16
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

// Function to send 16-bit SPI frame
void SPI_send_frame(uint8_t cs_pin, uint16_t address, uint16_t data) {
    debug_print("Sending SPI frame...\n");

    // Construct 16-bit SDI frame
    // MSB format: Command Byte (8-bit)
    //   [B15:B14] = 00 (standard frame)
    //   [B13:B8]  = Address (6 bits)
    //   [B7:B0]   = Write Command (W0 = 0)
    uint16_t command_byte = (0 << 14) | ((address & 0x3F) << 8) | (0 << 7);
    
    // Full 16-bit frame
    uint16_t sdi_frame = command_byte | (data & 0xFF);

    debug_print("Pulling CS low...\n");

    delay_ms(50);

    // Select the chip
    PORTC &= ~(1 << cs_pin);
    
    // Delay to meet nSCS setup time
    _delay_us(0.05);
    
    // Send high byte first (MSB)
    SPDR = (sdi_frame >> 8) & 0xFF;
    while(!(SPSR & (1 << SPIF)));
    
    // Send low byte
    SPDR = sdi_frame & 0xFF;
    while(!(SPSR & (1 << SPIF)));
    
    // Deselect the chip
    PORTC |= (1 << cs_pin);
}

void SPI_read_frame(uint8_t cs_pin, uint16_t address) {

    uint16_t *data;

    // Construct 16-bit SDI frame
    // MSB format: Command Byte (8-bit)
    //   [B15:B14] = 00 (standard frame)
    //   [B13:B8]  = Address (6 bits)
    //   [B7:B0]   = Read Command (R0 = 1)
    uint16_t command_byte = (0 << 14) | ((address & 0x3F) << 8) | (1 << 7);
    
    // Full 16-bit frame
    uint16_t sdi_frame = command_byte;

    // Select the chip
    PORTC &= ~(1 << cs_pin);
    
    // Delay to meet nSCS setup time
    _delay_us(0.05);
    
    // Send high byte first (MSB)
    SPDR = (sdi_frame >> 8) & 0xFF;
    while(!(SPSR & (1 << SPIF)));
    
    // Send low byte
    SPDR = sdi_frame & 0xFF;
    while(!(SPSR & (1 << SPIF)));
    
    // Read the data
    *data = SPDR;
    
    // Deselect the chip
    PORTC |= (1 << cs_pin);

    char debug_msg[50];

    debug_print("Data read from SPI frame...\n");
    debug_print("Data: ");
    sprintf(debug_msg, "0x%04X", *data);
    debug_print(debug_msg);
    debug_print("\n");
}

void motor_spi_init() {
    if (MACHINE_TYPE == BAMBOO) {
        SPI_init();
        debug_print("Initializing SPI for stepper drivers...\n");

        // Configure Chip Select pins as outputs
        DDRC |= (1 << CS1_PIN) | (1 << CS2_PIN);
        CS1_PORT |= (1 << CS1_PIN);  // Set CS high (deselect)
        CS2_PORT |= (1 << CS2_PIN);  // Set CS high (deselect)

        // Send configuration to Driver 1
        SPI_send_frame(CS1_PIN, 0x04, 0x07);
        // Send configuration to Driver 2
        SPI_send_frame(CS2_PIN, 0x04, 0x07);

        SPI_read_frame(CS1_PIN, 0x04);

        debug_print("SPI initialization complete.\n");
    }
}